// 
// Temperature, Humidity and Air Quality 
// with WeMOS D1 mini lite 
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

#include <ArduinoJson.h>

// ************************************
// Config, save and load functions
//
// save and load configuration from config file in SPIFFS. JSON format (need ArduinoJson library)
// ************************************
bool loadConfigFile() {
  DynamicJsonDocument root(512);
  DEBUG("[DEBUG] loadConfigFile()");
  configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    DEBUG("[ERROR] Config file not available");
    return false;
  } else {
    DeserializationError err = deserializeJson(root, configFile);
    if (err) {
      DEBUG("[CONFIG] Failed to read config file:"+String(err.c_str()));
      return false;
    } else {
      strlcpy(config.wifi_essid, root["wifi_essid"], sizeof(config.wifi_essid));
      strlcpy(config.wifi_password, root["wifi_password"], sizeof(config.wifi_password));
      strlcpy(config.hostname, root["hostname"] | "thaq-sensor", sizeof(config.hostname));
      strlcpy(config.broker_host, root["broker_host"] | "", sizeof(config.broker_host));
      config.broker_port = root["broker_port"] | 1883;
      strlcpy(config.client_id, root["client_id"] | "thaq-sensor", sizeof(config.client_id));
      strlcpy(config.syslog_server, root["syslog_server"] | "", sizeof(config.syslog_server));
      config.syslog_port = root["syslog_port"] | 514;
      config.ota_enable = root["ota_enable"] | true; // OTA updates enabled by default
      
      strlcpy(config.ntp_server, root["ntp_server"] | "time.ien.it", sizeof(config.ntp_server));
      config.ntp_timezone = root["ntp_timezone"] | 1;
      //
      config.alarm_t = root["alarm_temp"] | 0;
      config.alarm_h = root["alarm_hum"] | 0;
      config.alarm_aq = root["alarm_aq"] | 0;
      config.alarm_hdex = root["alarm_hdex"] | 0;
      //
      DEBUG("[INIT] Configuration loaded");
    }
  }
  configFile.close();
  return true;
}

bool saveConfigFile() {
  DynamicJsonDocument root(512);
  DEBUG("[DEBUG] saveConfigFile()\n");

  root["wifi_essid"] = config.wifi_essid;
  root["wifi_password"] = config.wifi_password;
  root["hostname"] = config.hostname;
  root["broker_host"] = config.broker_host;
  root["broker_port"] = config.broker_port;
  root["client_id"] = config.client_id;
  root["ntp_server"] = config.ntp_server;
  root["ntp_timezone"] = config.ntp_timezone;
  root["syslog_server"] = config.syslog_server;
  root["syslog_port"] = config.syslog_port;
  root["ota_enable"] = config.ota_enable;

  root["alarm_temp"] = config.alarm_t;
  root["alarm_hum"] = config.alarm_h;
  root["alarm_aq"] = config.alarm_aq;
  root["alarm_hdex"] = config.alarm_hdex;
  
  configFile = SPIFFS.open(CONFIG_FILE, "w");
  if(!configFile) {
    DEBUG("[ERROR] Failed to create config file !");
    return false;
  }
  serializeJson(root,configFile);
  configFile.close();
  return true;
}
