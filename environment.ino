// Indoor Air Quality monitor
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

// ************************************
// calculateHumidex()
//
// calculate Humidex value:
// if ((humidex >= 21 )&&(humidex < 27)) ==> OK
// if ((humidex >= 27 )&&(humidex < 35)) ==> not so bad...
// if ((humidex >= 35 )&&(humidex < 40)) ==> bad
// if ((humidex >= 40 )&&(humidex < 46)) ==> health risks
// if ((humidex >= 46 )&&(humidex < 54)) ==> severe health risks
// if ((humidex >= 54 )) ==> heat stroke danger
// ************************************
float calculateHumidex(float temperature, float humidity) {
  float e;

  e = (6.112 * pow(10,(7.5 * temperature/(237.7 + temperature))) * humidity/100); //vapor pressure

  float humidex = temperature + 0.55555555 * (e - 10.0); //humidex
  return humidex;
}

// ************************************
// envCallback()
//
// ************************************    
void envCallback() {   
  char temp[10];
  unsigned int rzero;

  DEBUG("envCallback()");
  bme.takeForcedMeasurement();

  temperature = bme.readTemperature();
  dtostrf(temperature, 5, 2, temp);
  env["temperature"] = temperature;
  DEBUG("BME280 Temperature: "+String(temp)+"C");

  humidity = bme.readHumidity();
  dtostrf(humidity, 5, 2, temp);
  env["humidity"] = humidity;  
  DEBUG("BME280 Humidity: "+String(temp)+"%");
    
  pressure = bme.readPressure() / 100.0F;
  dtostrf(pressure, 5, 2, temp);
  env["pressure"] = pressure;
  DEBUG("BME280 Pressure: "+String(temp)+"hPA");
  
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  dtostrf(altitude, 5, 2, temp);
  env["altitude"] = altitude;
  DEBUG("BME280 Altitude: "+String(temp)+"mslm");

  // Calculate humidex
  humidex = calculateHumidex(temperature, humidity);
    
  if ((humidex >= 21 )&&(humidex < 27)) {
  // Good
    setLed(0,100,100);      
  } // dark green
  if ((humidex >= 27 )&&(humidex < 35)) {
    // Quite good
    setLed(50,100,0);
  }
  if ((humidex >= 35 )&&(humidex < 40)) {
    // Not good
    setLed(100,100,5);
  } 
  if ((humidex >= 40 )&&(humidex < 46)) {
    // Health risk
    setLed(100,40,0);
  }
  if ((humidex >= 46 )&&(humidex < 54)) {
    setLed(100,20,0);
  } 
  if ((humidex >= 54 )) {
    setLed(100,0,0);
  }
  
  dtostrf(humidex, 5, 2, temp);
  env["humidex"] = humidex;
  DEBUG("Humidex index: "+String(temp));

  /* if MQ135 is warmed up... */
  if(mq135ready) {
    rzero = mq135.getCorrectedRZero(temperature,humidity);
    DEBUG("[DEBUG] CO2 RZero: "+String(rzero));

    vocs = mq135.getCorrectedPPM(temperature,humidity);

    if(isnan(vocs)) {
      Serial.println("[ERROR] Failed to read from MQ135 sensor!");
    } else {
      dtostrf(vocs, 5, 2, temp);
      env["vocs"] = vocs;
      DEBUG("[DEBUG] MQ135 VOCs PPM: "+String(temp));
    }
  }

  env["timestamp"] = timeClient.getFormattedTime();

  isDataReady=true;
}
