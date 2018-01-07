# kwh-meter

kwh-meter: A simple application to monitor full-house energy
consumption by detecting the rotation of the spinning disk in the
meter. Consumption is averaged over a time and published to MQTT.
    
Hardware overview:
 - IR sensor and emitter, such as a TCRT5000 module
 - some form of Arduino or similar microcontroler, code assumes ESP8266
 - Ferraris meter with rotating disk with a non-reflective area

 Dependencies:
  - NTPClient (https://github.com/arduino-libraries/NTPClient)
  - ESP8266WiFi (https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi)
  - ArduinoOTA (https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA)
  - PubSubClient (https://github.com/knolleary/pubsubclient)

Copyright 2018 Chris Broekema
