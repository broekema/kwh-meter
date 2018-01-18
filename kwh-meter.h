/*  
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

    kwh-meter  is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    kwh-meter is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define VERSION "0.3"

//#define DEBUG_NTPClient
#define DEBUG

#define sensorPin 12 // pin connecting to the TCRT5000 ..  D6==GPIO12 on NodeMCU
#define C 600.0      // 600 revolutions = 1 kWh 
#define threshold 1  // threshold value of the IR sensor, set to 1 for digital output

#define WIFI_SSID     ""
#define WIFI_PASSWORD ""
#include "local_wifi.h"

#define SERIAL_BAUD 115200

#define OTA_HOSTNAME "KWHMETER"
#define OTA_PASSWORD "OTAPASSWORD"

/// NTP configuration
#define NTP_HOST "10.5.1.1"
#define NTP_OFFSET 0
#define NTP_UPDATE_INTERVAL 60000

/// MQTT configuration
#define MQTT_SERVER "10.5.1.5"
#define MQTT_PORT 1883
#define MQTT_ROOT "Peizerweg217/"
#define MQTT_GATEWAY_NAME "kWhMeter/"
#define MQTT_WILL_TOPIC MQTT_ROOT MQTT_GATEWAY_NAME "LWT"
#define MQTT_WILL_RETAIN true
#define MQTT_WILL_QOS 0
#define MQTT_WILL_MESSAGE "Offline"
#define MQTT_GATEWAY_ANNOUNCEMENT "Online"
#define MQTT_VERSION_TOPIC MQTT_ROOT MQTT_GATEWAY_NAME "version"

/// define the MQTT subjects to publish energy consumption to
#define ONEMINSUBJECT MQTT_ROOT MQTT_GATEWAY_NAME "1MinuteWatt"
#define TENMINSUBJECT MQTT_ROOT MQTT_GATEWAY_NAME "10MinuteWatt"
#define ONEHRSUBJECT MQTT_ROOT MQTT_GATEWAY_NAME "1HourWatt"
#define SIXHRSUBJECT MQTT_ROOT  MQTT_GATEWAY_NAME "6HourWatt"
#define TWENTYFOURHRSUBJECT MQTT_ROOT MQTT_GATEWAY_NAME "24HourWatt"
#define DAILYTOPIC MQTT_ROOT MQTT_GATEWAY_NAME "DailyKWH"
#define TIMETOPIC MQTT_ROOT MQTT_GATEWAY_NAME "Time"

// struct to hold values for time based measurements
struct EnergyMeasures {
  unsigned long begin_time;
  unsigned long begin_count;
};
