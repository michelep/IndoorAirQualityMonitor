// THAQ Station 
// Temperature, Humidity and Pressure
// with Lolin ESP8266
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty


String templateProcessor(const String& var)
{
  //
  // System values
  //
  if(var == "hostname") {
    return String(config.hostname);
  }
  if(var == "fw_name") {
    return String(FW_NAME);
  }
  if(var=="fw_version") {
    return String(FW_VERSION);
  }
  if(var=="uptime") {
    return String(millis()/1000);
  }
  if(var=="timedate") {
    return NTP.getTimeDateString();
  }
  //
  // Config values
  //
  if(var=="wifi_essid") {
    return String(config.wifi_essid);
  }
  if(var=="wifi_password") {
    return String(config.wifi_password);
  }
  if(var=="wifi_rssi") {
    return String(WiFi.RSSI());
  }
  if(var=="ntp_server") {
    return String(config.ntp_server);
  }
  if(var=="ntp_timezone") {
    return String(config.ntp_timezone);
  }
  if(var=="syslog_server") {
    return String(config.syslog_server);
  }
  if(var=="syslog_port") {
    return String(config.syslog_port);
  }
  if(var=="broker_host") {
    return String(config.broker_host);
  }
  if(var=="broker_port") {
    return String(config.broker_port);
  }
  if(var=="client_id") {
    return String(config.client_id);
  }  
  // Alarm values
  if(var=="alarm_temp") {
    return String(config.alarm_t);
  }
   if(var=="alarm_hum") {
    return String(config.alarm_h);
  }
  if(var=="alarm_aq") {
    return String(config.alarm_aq);
  }
  if(var=="alarm_hdex") {
    return String(config.alarm_hdex);
  }
  //
  return String();
}

// ************************************
// initWebServer
//
// initialize web server
// ************************************
void initWebServer() {
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setTemplateProcessor(templateProcessor);

  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
    String message;
    if(request->hasParam("wifi_essid", true)) {
        strcpy(config.wifi_essid,request->getParam("wifi_essid", true)->value().c_str());
    }
    if(request->hasParam("wifi_password", true)) {
        strcpy(config.wifi_password,request->getParam("wifi_password", true)->value().c_str());
    }
    if(request->hasParam("ntp_server", true)) {
        strcpy(config.ntp_server, request->getParam("ntp_server", true)->value().c_str());
    }
    if(request->hasParam("ntp_timezone", true)) {
        config.ntp_timezone = atoi(request->getParam("ntp_timezone", true)->value().c_str());
    }
    if(request->hasParam("syslog_server", true)) {
        strcpy(config.syslog_server,request->getParam("syslog_server", true)->value().c_str());
    }
    if(request->hasParam("syslog_port", true)) {
        config.syslog_port = atoi(request->getParam("syslog_port", true)->value().c_str());
    }
    if(request->hasParam("broker_host", true)) {
        strcpy(config.broker_host,request->getParam("broker_host", true)->value().c_str());
    }
    if(request->hasParam("broker_port", true)) {
        config.broker_port = atoi(request->getParam("broker_port", true)->value().c_str());
    }
    if(request->hasParam("client_id", true)) {
        strcpy(config.client_id, request->getParam("client_id", true)->value().c_str());
    }    
    //
    if(request->hasParam("alarm_temp", true)) {
        config.alarm_t = atoi(request->getParam("alarm_temp", true)->value().c_str());
    }     
    if(request->hasParam("alarm_hum", true)) {
        config.alarm_h = atoi(request->getParam("alarm_hum", true)->value().c_str());
    }     
    if(request->hasParam("alarm_aq", true)) {
        config.alarm_aq = atoi(request->getParam("alarm_aq", true)->value().c_str());
    }     
    if(request->hasParam("alarm_hdex", true)) {
        config.alarm_hdex = atoi(request->getParam("alarm_hdex", true)->value().c_str());
    } 
    //
    saveConfigFile();
    request->redirect("/?result=ok");
  });

  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
    ESP.restart();
  });
  
  server.on("/ajax", HTTP_POST, [] (AsyncWebServerRequest *request) {
    String action,value,response="";

    if (request->hasParam("action", true)) {
      action = request->getParam("action", true)->value();
      Serial.print("ACTION: ");
      if(action.equals("get")) {
        value = request->getParam("value", true)->value();
        Serial.println(value);
        if(value.equals("temp")) {
          response = String(temperature);
        }
        if(value.equals("humidity")) {
          response = String(humidity);
        }
        if(value.equals("pressure")) {
          response = String(pressure);
        }
        if(value.equals("co2")) {
          response = String(aq_ppm);
        }
        if(value.equals("humidex")) {
          response = String(humidex);
        }
        if(value.equals("altitude")) {
          response = String(altitude);
        }
        if(value.equals("status")) {
          response = system_status;
        }
      }
    }
    request->send(200, "text/plain", response);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server.begin();
}
