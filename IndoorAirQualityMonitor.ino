// Indoor Air Quality monitor 
//
// Temperature, Humidity and Air Quality for Indoor use
// with WeMOS D1 mini lite, BME280 and MQ135
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty
// 
// Tested with WeMos D1 mini Lite
// Flash size: 1MB (256KB SPIFFS)
//
// 11 GPIO pins, all except D0, support interrupt/PWM/I2C/one-wire
// 1 analog input (3.2V max input)
// 1 Micro USB connection
// 1MB Flash
// 500mA resettable fuse
// 
// Pin  Function  ESP-8266 Pin
// TX  TXD   TXD
// RX  RXD   RXD
// A0  Analog input, max 3.3V input  A0
// D0  IO  GPIO16
// D1  IO, SCL   GPIO5
// D2  IO, SDA   GPIO4
// D3  IO, 10k Pull-up   GPIO0
// D4  IO, 10k Pull-up, BUILTIN_LED  GPIO2
// D5  IO, SCK   GPIO14
// D6  IO, MISO  GPIO12
// D7  IO, MOSI  GPIO13
// D8  IO, 10k Pull-down, SS   GPIO15
// G   Ground  GND
// 5V  5V  -
// 3V3   3.3V  3.3V
// RST   Reset   RST
// All of the IO pins have interrupt/pwm/I2C/one-wire support except D0.
// All of the IO pins run at 3.3V.
//
// v0.2.0 - 12.07.2021
// - WEB GUI completely rewritten using purecss and self-hosted libs
// - MQTT re-engineered for better handling
// - Change NTP client library
// - More natural LED fading
//
// v0.1.2 - 12.11.2020
// - library updates
// - MQTT tasks
// - remove unused syslog related stuff
//
// v0.1.1 - 13.10.2019
// - added ota_enable to prevent accidental or unwanted OTA updates (true if OTA is enables, false otherwise)
// - change MQTT messages
//
// v0.1.0 - 03.07.2019
// - added alarm for temp, humidity, air quality and humixed indexes
// - some minor fixes
//
// v0.0.6
// - hardware improvements: BME280 
// - piezo buzzer
//
// v0.0.5
// - Update to ArduinoJson 6
// - move from REST to MQTT
//
// v0.0.4
// - Added reboot and WiFI RSSI config parameters
// - Fixed an error on datetime print
//
// v0.0.3
// - Fixed an issue with RestClient object: now will be instantiated only when needed. Removed global object.

#define __DEBUG__

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266httpUpdate.h>
#include <EEPROM.h>
#include <ESPAsyncTCP.h>

#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// TaskScheduler
// https://github.com/arkhipenko/TaskScheduler
#include <TaskScheduler.h>

Scheduler runner;

// ArduinoJson
// https://arduinojson.org/
#include <ArduinoJson.h>

// MQTT PubSub client
// https://github.com/knolleary/pubsubclient
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// MQ135 
// https://github.com/GeorgK/MQ135.git
#include <MQ135.h>

// I2C BME280
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

// NTP ClientLib 
// https://github.com/taranais/NTPClient
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// File System
#include <FS.h>   

// Format SPIFFS if mount failed
#define FORMAT_SPIFFS_IF_FAILED 1

// Firmware data
const char BUILD[] = __DATE__ " " __TIME__;
#define FW_NAME         "iaq-monitor"
#define FW_VERSION      "0.2.0"

#define ANALOGPIN  A0 
#define BUZZER 0 // D3

// RGB Led Pinout - Common Anode (light when LOW)
#define PWMRANGE 100

#define LED_B 13 // D7
#define LED_G 12 // D5
#define LED_R 14 // D6

// Define to which analog pin MQ135 was connected...
MQ135 mq135 = MQ135(ANALOGPIN);
bool mq135ready=false;

// Define tasks and callbacks
#define MQ135_INTERVAL 60000*15 // 15 mins delay before getting MQ135 values
void mq135Callback();
Task mq135Task(MQ135_INTERVAL, TASK_FOREVER, &mq135Callback);

#define ENV_INTERVAL 60000 // Every minute
void envCallback();
Task envTask(ENV_INTERVAL, TASK_FOREVER, &envCallback);

#define MQTT_INTERVAL 60000*5 // Every 5 minutes
void mqttCallback();
Task mqttTask(MQTT_INTERVAL, TASK_FOREVER, &mqttCallback);

// Web server
AsyncWebServer server(80);

// Config
struct Config {
  // WiFi config
  char wifi_essid[16];
  char wifi_password[16];
  // MQTT config
  char broker_host[32];
  unsigned int broker_port;
  unsigned int broker_timeout;
  char client_id[18];
  // Host config
  bool ota_enable;
  char hostname[16];
  //
  uint8_t alarm_temp;
  uint8_t alarm_hum;
  uint8_t alarm_vocs;
  uint8_t alarm_hdex;
};

#define CONFIG_FILE "/config.json"

File configFile;
Config config; // Global config object

static int last; // millis counter

float temperature=0, humidity=0, pressure=0, vocs=0, humidex=0, altitude=0;
DynamicJsonDocument env(256);

bool isSetupMode=false;
bool isDataReady=false; // True if environmental data is ready to be sent!
uint8_t isAlarm=0; // >0 if on alarm, and buzzer just beep until 

// https://diarmuid.ie/blog/pwm-exponential-led-fading-on-arduino-or-other-platforms
float R; // Brightness constant for led fading

// ************************************
// DEBUG()
//
// send message via RSyslog (if enabled) or fallback on Serial
// ************************************
void DEBUG(String message) {
#ifdef __DEBUG__
  Serial.println(message);
#endif
}

// ************************************
// md135Callback()
//
// after 24h, enable measurements for MD135
// ************************************
void mq135Callback() {
  if(!mq135Task.isFirstIteration()) {
    DEBUG("[DEBUG] Enable MQ135 after warm up...");
    mq135ready=true;
    mq135Task.disable();
  }
}

// ************************************
// mqttConnect()
//
//
// ************************************
void mqttConnect() {
  uint8_t timeout=10;
  if(strlen(config.broker_host) > 0) {
    DEBUG("[MQTT] Attempting connection to "+String(config.broker_host)+":"+String(config.broker_port));
    while((!mqttClient.connected())&&(timeout > 0)) {
      // Attempt to connect
      if (mqttClient.connect(config.client_id)) {
        // Once connected, publish an announcement...
        DEBUG("[MQTT] Connected as "+String(config.client_id));
      } else {
        timeout--;
        delay(500);
      }
    }
    if(!mqttClient.connected()) {
      DEBUG("[MQTT] Connection failed");    
    }
  }
}

void mqttInCallback(char* topic, byte* payload, unsigned int length) {
  DEBUG("Message arrived: "+String(topic));
}

bool mqttPublish(char *topic, char *payload) {
  if (mqttClient.connected()) {
  }
}

void mqttCallback() {
  char topic[32],value[16];
  if(isDataReady) {
    if(WiFi.status() != WL_CONNECTED) {
      connectToWifi();
    }
    // MQTT not connected? Connect!
    if (!mqttClient.connected()) {
      mqttConnect();
    }

    JsonObject root = env.as<JsonObject>();
 
    for (JsonPair keyValue : root) {
      sprintf(topic,"%s/%s",config.client_id,keyValue.key().c_str());
      if(keyValue.key().c_str()[0] != '_') {
        String value = keyValue.value().as<String>();
        if(mqttClient.publish(topic,value.c_str())) {
          DEBUG("[MQTT] Publish "+String(topic)+":"+value);
          isDataReady=false;
          env["status"] = "Data published via MQTT";
        } else {
          DEBUG("[MQTT] Publish failed!");
        }
      }
    }
  }  
}


// ************************************
// runWifiAP()
//
// run Wifi AccessPoint, to let user configure iHot without a WiFi
// ************************************

#define DEFAULT_WIFI_ESSID "IHOT_AP"
#define DEFAULT_WIFI_PASSWORD "12345678"  

void runWifiAP() {
  DEBUG("[DEBUG] runWifiAP() ");

  WiFi.mode(WIFI_AP); 
  WiFi.softAP(DEFAULT_WIFI_ESSID,DEFAULT_WIFI_PASSWORD);  
 
  IPAddress myIP = WiFi.softAPIP(); //Get IP address

  Serial.println("");
  Serial.print("WiFi Hotspot ESSID: ");
  Serial.println(DEFAULT_WIFI_ESSID);  
  Serial.print("WiFi password: ");
  Serial.println(DEFAULT_WIFI_PASSWORD);  
  Serial.print("Server IP: ");
  Serial.println(myIP);

  WiFi.printDiag(Serial);

  if (MDNS.begin(config.hostname)) {
    Serial.print("MDNS responder started for hostname ");
    Serial.println(config.hostname);
  }

  isSetupMode=true;
}

// ************************************
// connectToWifi()
//
// connect to configured WiFi network
// ************************************
bool connectToWifi() {
  uint8_t timeout=0;

  if(isSetupMode) {
    DEBUG("Setup mode: connect to AP and configure!");
    return false;
  }

  if(strlen(config.wifi_essid) > 0) {
    Serial.print("[INIT] Connecting to ");
    Serial.print(config.wifi_essid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi_essid, config.wifi_password);

    while((WiFi.status() != WL_CONNECTED)&&(timeout < 10)) {
      setLedRGB(PWMRANGE,0,0);
      delay(250);
      Serial.print(".");
      setLedRGB(0,0,0);
      delay(250);
      timeout++;
    }
    if(WiFi.status() == WL_CONNECTED) {
      DEBUG("OK. IP:"+WiFi.localIP().toString());

      if (MDNS.begin(config.hostname)) {
        Serial.println("[INIT] MDNS responder started");
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
      }
  
      // NTP    
      timeClient.begin();
  
      // Setup MQTT connection
      mqttClient.setServer(config.broker_host, config.broker_port);
      mqttClient.setCallback(mqttInCallback); 
      
      return true;  
    } else {
      Serial.println("[ERROR] Failed to connect to WiFi");
      return false;
    }
  } else {
    Serial.println("[ERROR] Please configure Wifi");
    runWifiAP();
    return false; 
  }
}


// ************************************
// setup()
//
// setup routine
// ************************************
void setup() {
  unsigned status;
  Serial.begin(115200);
  delay(1000);

  env["status"]="Booting...";

   // Define led ports as OUTPUT and turn it off 
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  setLedRGB(0,0,0);

  pinMode(BUZZER, OUTPUT);
  analogWrite(BUZZER, 205);
  delay(500);
  analogWrite(BUZZER, 0);

  // print firmware and build data
  Serial.println();
  Serial.println();
  Serial.print(FW_NAME);
  Serial.print("");
  Serial.print(FW_VERSION);
  Serial.print("");  
  Serial.println(BUILD);

  // Start scheduler
  runner.init();

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    DEBUG("[ERROR] SPIFFS Mount Failed. Try formatting...");
    if(SPIFFS.format()) {
      Serial.println("[INIT] SPIFFS initialized successfully");
    } else {
      Serial.println("[FATAL] SPIFFS error");
      ESP.restart();
    }
  } else {
    DEBUG("SPIFFS done");
  }

  // Load configuration
  loadConfigFile();

  // Setup I2C...
  DEBUG("[INIT] Initializing I2C bus...");
  Wire.begin(SDA, SCL);
  
  i2c_status();

  DEBUG("[INIT] Initializing BME280...");
  status=bme.begin(0x77);
  if(!status) {
     Serial.print("[ERROR] BME280 error!");
     delay(1000);
     ESP.restart();
  }
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF);
                      
  // Connect to WiFi network
  connectToWifi();

  // Setup OTA
  ArduinoOTA.onStart([]() { 
    Serial.println("[OTA] Update Start");
  });
  ArduinoOTA.onEnd([]() { 
    Serial.println("[OTA] Update End"); 
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "[OTA] Progress: %u%%\n", (progress/(total/100)));
    Serial.println(p);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) Serial.println("[OTA] Auth Failed");
    else if(error == OTA_BEGIN_ERROR) Serial.println("[OTA] Begin Failed");
    else if(error == OTA_CONNECT_ERROR) Serial.println("[OTA] Connect Failed");
    else if(error == OTA_RECEIVE_ERROR) Serial.println("[OTA] Recieve Failed");
    else if(error == OTA_END_ERROR) Serial.println("[OTA] End Failed");
  });
  ArduinoOTA.setHostname(config.hostname);
  ArduinoOTA.begin();

  // Enable runner tasks
  // MQ135 VOCs task
  runner.addTask(mq135Task);
  mq135Task.enable();

  // Environment task
  runner.addTask(envTask);
  envTask.enable();

  // MQTT task
  runner.addTask(mqttTask);
  mqttTask.setInterval(config.broker_timeout*1000);
  mqttTask.enable();

  // Initialize web server on port 80
  initWebServer();

  // Adjust analog write rage for LED PWM fading
  analogWriteRange(PWMRANGE);
  R = (PWMRANGE * log10(2))/(log10(255));
  
  // Go!
  env["uptime"] = 0;
  env["status"] = "Go!";
}

unsigned int led_R=0,led_G=0,led_B=0,led_max_R=PWMRANGE,led_max_G=PWMRANGE,led_max_B=PWMRANGE,led_sense=0;

unsigned int ledBrightness(unsigned int interval) {
  return PWMRANGE - (pow (2, (interval / R)) - 1);
}

void setLedRGB(unsigned int led_R,unsigned int led_G,unsigned int led_B) {  
  analogWrite(LED_R, ledBrightness(led_R));
  analogWrite(LED_G, ledBrightness(led_G));
  analogWrite(LED_B, ledBrightness(led_B)); 
}

void setLed(unsigned int led_R,unsigned int led_G,unsigned int led_B) {
  led_max_R = led_R;
  led_max_G = led_G;
  led_max_B = led_B;
}

void ledLoop() {
  if(led_sense == 0) {
      if((led_R >= led_max_R)&&(led_G >= led_max_G)&&(led_B >= led_max_B)) {
        led_sense = 1; 
      } else {
        if(led_R < led_max_R) led_R++;
        if(led_G < led_max_G) led_G++;
        if(led_B < led_max_B) led_B++;
      }
    } else {
      if((led_R < 1)&&(led_G < 1)&&(led_B < 1)) {
        led_sense = 0; 
      } else {
        if(led_R > 0) led_R--;
        if(led_G > 0) led_G--;
        if(led_B > 0) led_B--;
      }
  }
  setLedRGB(led_R,led_G,led_B);
}

// ************************************
// loop()
//
// loop routine
// ************************************
void loop() {
  if(config.ota_enable) {
    // Handle OTA
    ArduinoOTA.handle();
  }

  // Scheduler
  runner.execute();

  if((millis() - last) > 5100) {  
    if(isSetupMode) {
      Serial.printf("Stations connected to soft-AP = %d\n", WiFi.softAPgetStationNum());  
    }
    
    if(isAlarm) {
      analogWrite(BUZZER, 200);
      delay(100);
      analogWrite(BUZZER, 300);
      delay(100);
      analogWrite(BUZZER, 0);
      isAlarm--;
    }
    // NTP Sync
    timeClient.update();
    
    last = millis();
  }

  if(!isAlarm) {
    if((config.alarm_temp > 0)&&(temperature > config.alarm_temp)) {
      DEBUG("[ALARM] Alarm: temperature triggered!");
      env["status"]="Temperature ALARM triggered";
      isAlarm=10;
    } else if((config.alarm_hum > 0)&&(humidity > config.alarm_hum)) {
      DEBUG("[ALARM] Alarm: humidity triggered!");
      env["status"]="Humidity ALARM triggered";
      isAlarm=10;
    } else if((config.alarm_vocs > 0)&&(vocs > config.alarm_vocs)) {
      DEBUG("[ALARM] Alarm: AirQuality triggered!");
      env["status"]="Air Quality ALARM triggered";
      isAlarm=10;
    } else if((config.alarm_hdex > 0)&&(humidex > config.alarm_hdex)) {
      DEBUG("[ALARM] Alarm: Humidex triggered!");
      env["status"]="Humidex ALARM triggered";
      isAlarm=10;
    } else {
      env["status"]="System up and running";
    }
  }
  
  ledLoop();

  env["uptime"] = millis() / 1000; 

  delay(10);
}
