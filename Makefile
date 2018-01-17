
# kwh-meter: A simple application to monitor full-house energy
# consumption by detecting the rotation of the spinning disk in the
# meter. Consumption is averaged over a time and published to MQTT.

# Hardware overview:
#  - IR sensor and emitter, such as a TCRT5000 module
#  - some form of Arduino or similar microcontroler, code assumes ESP8266
#  - Ferraris meter with rotating disk with a non-reflective area

#  Dependencies:
#   - NTPClient (https://github.com/arduino-libraries/NTPClient)
#   - ESP8266WiFi (https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WiFi)
#   - ArduinoOTA (https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA)
#   - PubSubClient (https://github.com/knolleary/pubsubclient)

# Copyright 2018 Chris Broekema

# kwh-meter  is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# kwh-meter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


# Makefile to build the application and upload it to an ESP8266 based device,
# without relying on the Arduino IDE.
# depends on https://github.com/plerup/makeEspArduino

# Set some local parameters
SKETCH = kwh-meter.ino

# OTA parameters
ESP_ADDR = KWHMETER
ESP_PWD = OTAPASSWORD

BOARD = nodemcuv2

# include the main Makefile
include ../makeEspArduino/makeEspArduino.mk
