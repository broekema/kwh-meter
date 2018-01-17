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

#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>

#include "kwh-meter.h"

int  sensor_value;
boolean state = false;  // state == 1 -> sensor == high
unsigned long count = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_HOST, NTP_OFFSET, NTP_UPDATE_INTERVAL);

WiFiClient eClient;
PubSubClient client(MQTT_SERVER, MQTT_PORT, NULL, eClient);
 
// We want to measure consumption over various time ranges
EnergyMeasures onemin;
EnergyMeasures tenmin;
EnergyMeasures onehr;
EnergyMeasures sixhr;
EnergyMeasures twentyfourhr;
EnergyMeasures daily;

void setup_wifi() {
  //  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
#ifdef DEBUG
    Serial.println(".");
#endif
    delay(500);
  }

#ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address: "); Serial.println(WiFi.localIP());
#endif
}

void setup_OTA() {
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() { Serial.println("Start"); });
  ArduinoOTA.onEnd([]() { Serial.println("End");});
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
#ifdef DEBUG
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
#endif
    });
#ifdef DEBUG
  ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
      else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
      else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
      else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
      else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
    });
#endif
  ArduinoOTA.begin();
}

void setupMeasures() {
  timeClient.update();
  
  onemin.begin_time = timeClient.getMinutes();
  tenmin.begin_time = timeClient.getMinutes();
  onehr.begin_time = timeClient.getHours();
  sixhr.begin_time = timeClient.getHours();
  twentyfourhr.begin_time = timeClient.getDay();
  daily.begin_time = timeClient.getDay();
  
  onemin.begin_count = 0;
  tenmin.begin_count = 0;
  onehr.begin_count = 0;
  sixhr.begin_count = 0;
  twentyfourhr.begin_count = 0;
  daily.begin_count =  0;
}

void setup() {
  Serial.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY);
  //  pinMode(sensorPin, INPUT);
#ifdef DEBUG
  Serial.println("kWh meter initializing");
#endif
  delay(1500);
  
  setup_wifi();
  setup_OTA();
  timeClient.begin();
  delay(1500);
  setupMeasures();
#ifdef DEBUG
  Serial.println("Setup complete");

  Serial.println("subjects: ");
  Serial.println(ONEMINSUBJECT);
  Serial.println(TENMINSUBJECT);
  Serial.println(ONEHRSUBJECT);
  Serial.println(SIXHRSUBJECT);
  Serial.println(TWENTYFOURHRSUBJECT);
#endif
}

boolean reconnect_mqtt() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (client.connect(MQTT_GATEWAY_NAME, MQTT_WILL_TOPIC, MQTT_WILL_QOS, MQTT_WILL_RETAIN, MQTT_WILL_MESSAGE)) {
#ifdef DEBUG
	Serial.println("Connected to broker ");
#endif
	client.publish(MQTT_WILL_TOPIC, MQTT_GATEWAY_ANNOUNCEMENT, MQTT_WILL_RETAIN);
	client.publish(MQTT_VERSION_TOPIC, VERSION, MQTT_WILL_RETAIN);
      } else {
#ifdef DEBUG
	Serial.println("failed, rc=" + String(client.state()));
	Serial.println("try again in 5s");
#endif
	// Wait 5 seconds before retrying
	delay(5000);
    }
  }
  return client.connected();
}

void loop() {
  
  // house-keeping - MQTT, NTP and ArduinoOTA
  if (!client.connected()) {
    reconnect_mqtt() ;
  }
  client.loop();       // MQTT
  ArduinoOTA.handle(); // OTA
  timeClient.update(); // NTP

  // sensor stuff  
  sensor_value = digitalRead(sensorPin);

  if ((sensor_value < threshold) && state) {
    // sensor no longer detects disk, and previously did (state is true)
    state = false;
    count++;
#ifdef DEBUG
    Serial.println("Ping number " + String(count));
#endif
  } else if ((sensor_value >= threshold) && !state) {
    // sensor detects disk, and previously did not (state is false)
    state = true;
  }

  if (timeClient.getMinutes() != onemin.begin_time) {
    // at the top of every minute, generate per minute metric
    double kwh = (count - onemin.begin_count) * 60.0 * 1000.0 / C;
#ifdef DEBUG
    Serial.println(timeClient.getFormattedTime() + " average energy consumption / minute = " + String(kwh) + " Watt");
#endif
    client.publish(ONEMINSUBJECT, String(kwh).c_str());
    onemin.begin_time = timeClient.getMinutes();
    onemin.begin_count = count;

    
    if (timeClient.getMinutes() % 10 == 0) {
      // every ten minutes, generate per 10 minute metrics
      kwh = (count - tenmin.begin_count) * 6.0 * 1000.0 / C;
#ifdef DEBUG
      Serial.println(timeClient.getFormattedTime() + " average energy consumption / 10 minutes = " + String(kwh) + " Watt");
#endif
      client.publish(TENMINSUBJECT, String(kwh).c_str());
      tenmin.begin_count = count;
    }

    if (timeClient.getMinutes() == 0) {
      // at the top of every hour, generate per hour metric
      kwh = (count - onehr.begin_count) * 1000.0 / C; 
#ifdef DEBUG
      Serial.println(timeClient.getFormattedTime() + " Average energy consumption / hour = "  + String(kwh) + " Watt");
#endif
      client.publish(ONEHRSUBJECT, String(kwh).c_str());
      onehr.begin_count = count;


      if (timeClient.getHours() % 6 == 0) {
	// at the top of the hour,
	// when #hours is divisable by 6,
	// generate per six hour metric
	kwh = (count - sixhr.begin_count) * 1000.0 / 6.0;
#ifdef DEBUG
	Serial.println(timeClient.getFormattedTime() + " Average energy consumption / 6 hours = "  + String(kwh) + " Watt");
#endif
	client.publish(SIXHRSUBJECT, String(kwh).c_str());
	sixhr.begin_count = count;
      }


      if (timeClient.getHours() == 0) {
	// at midnight, generate per 24 hrs metric and per day kwh
	kwh = (count - twentyfourhr.begin_count) * 1000.0 / C / 24.0;
#ifdef DEBUG
	Serial.println(timeClient.getFormattedTime() + " Average energy consumption / 24 hours = "  + String(kwh) + " Watt");
#endif
	client.publish(TWENTYFOURHRSUBJECT, String(kwh).c_str());
	twentyfourhr.begin_count = count;

	kwh = (count - daily.begin_count) / C;
#ifdef DEBUG
	Serial.println(timeClient.getFormattedTime() + " Daily energy consumption = " + String(kwh) + " kWh");
#endif
	client.publish(DAILYTOPIC, String(kwh).c_str());
	daily.begin_count = count;
      }
    }
  }
}
