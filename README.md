# Indoor Air Quality monitor
#
# A cheap and conveniente Indoor Air Quality Station
## Based on ESP8266 plus a bunch of cheap sensors!

This is an Arduino SDK project to create an Indoor Air Quality monitor, with MQTT support. With a WeMos D1 mini (ESP8266) and a Bosch BME280 sensor, plus a MQ135 VOCs air detector and a bunch of other components, you can create a fully-featured air quality monitor for your home.

## Configuration

All configurable items are in /data/config.json file, in JSON format. This directory need to be flashed to ESP8266's SPIFFS area.

## Changelog

v0.2.0 - 12.07.2021
- WEB GUI completely rewritten using purecss and self-hosted libs
- MQTT re-engineered for better handling
- Change NTP client library
- More natural LED fading

v0.1.1 - 13.10.2019
- added ota_enable to prevent accidental or unwanted OTA updates (true if OTA is enables, false otherwise)
- change MQTT messages

v0.1.0 - 03.07.2019
- added alarm for temp, humidity, air quality and humixed indexes
- some minor fixes

v0.0.6
- hardware improvements: BME280 
- piezo buzzer

v0.0.5
- Update to ArduinoJson 6
- move from REST to MQTT

v0.0.4
- Added reboot and WiFI RSSI config parameters
- Fixed an error on datetime print

v0.0.3
- Fixed an issue with RestClient object: now will be instantiated only when needed. Removed global object.

v0.0.2
v0.0.1
- First beta releases, not public yet!

## Support

There's no support nor any warranty! USE AT YOUR OWN RISK! But if you need help, try raise an Issue or sending a mail to o-zone@zerozone.it ;-) Of course, you are strongly encouraged to get the code and improve them!
