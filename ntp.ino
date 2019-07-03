// 
// Temperature, Humidity and Air Quality 
// with WeMOS D1 mini lite 
// DHT11/21/22
// MQ135
//
// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

#include <NtpClientLib.h>

// ************************************
// processSyncEvent()
//
// manage NTP sync events and warn in case of error
// ************************************
void processSyncEvent(NTPSyncEvent_t ntpEvent) {
  DEBUG("[DEBUG] processSyncEvent() ");
  if (ntpEvent) {
    Serial.print ("[ERROR] Time Sync error: ");
    if (ntpEvent == noResponse)
      Serial.println ("[ERROR] NTP server not reachable");
    else if (ntpEvent == invalidAddress)
      Serial.println ("[ERROR] Invalid NTP server address");
  } else {
#ifdef __DEBUG__    
    Serial.print("[DEBUG] Got NTP time: ");
    Serial.println(NTP.getTimeDateString (NTP.getLastNTPSync ()));
#endif
  }
}
