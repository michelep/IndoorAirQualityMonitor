// Indoor Air Quality monitor
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
    return String(timeClient.getFormattedTime());
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
  if(var=="broker_host") {
    return String(config.broker_host);
  }
  if(var=="broker_port") {
    return String(config.broker_port);
  }
  if(var=="broker_timeout") {
    return String(config.broker_timeout);
  }
  if(var=="client_id") {
    return String(config.client_id);
  }  
  if(var=="ota_enable") {
    if(config.ota_enable) {
      return String("checked");  
    } else {
      return String("");
    }
  }
  // Alarm values
  if(var=="alarm_temp") {
    return String(config.alarm_temp);
  }
   if(var=="alarm_hum") {
    return String(config.alarm_hum);
  }
  if(var=="alarm_vocs") {
    return String(config.alarm_vocs);
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
    if(request->hasParam("broker_host", true)) {
        strcpy(config.broker_host,request->getParam("broker_host", true)->value().c_str());
    }
    if(request->hasParam("broker_port", true)) {
        config.broker_port = atoi(request->getParam("broker_port", true)->value().c_str());
    }
    if(request->hasParam("broker_timeout", true)) {
        config.broker_timeout = atoi(request->getParam("broker_timeout", true)->value().c_str());
    }
    if(request->hasParam("client_id", true)) {
        strcpy(config.client_id, request->getParam("client_id", true)->value().c_str());
    }    
    if(request->hasParam("hostname", true)) {
        strcpy(config.hostname, request->getParam("hostname", true)->value().c_str());
    } 
    if(request->hasParam("ota_enable", true)) {
      config.ota_enable=true;        
    } else {
      config.ota_enable=false;
    } 
    //
    if(request->hasParam("alarm_temp", true)) {
        config.alarm_temp = atoi(request->getParam("alarm_temp", true)->value().c_str());
    }     
    if(request->hasParam("alarm_hum", true)) {
        config.alarm_hum = atoi(request->getParam("alarm_hum", true)->value().c_str());
    }     
    if(request->hasParam("alarm_vocs", true)) {
        config.alarm_vocs = atoi(request->getParam("alarm_vocs", true)->value().c_str());
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
    String action,response="";
    char outputJson[256];

    if (request->hasParam("action", true)) {
      action = request->getParam("action", true)->value();
      if(action.equals("env")) {
        serializeJson(env,outputJson);
        response = String(outputJson);
      }
    }
    request->send(200, "text/plain", response);
  });
  
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("NOT_FOUND: ");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    request->send(404);
  });

  server.begin();
}
