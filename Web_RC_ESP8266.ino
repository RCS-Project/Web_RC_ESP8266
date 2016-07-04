/*
  Web RC ESP8266
  Copyright (c) 2016 Subhajit Das

  Licence Disclaimer:

   This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <ESP8266WiFi.h>  // reuired for basic WiFi opertaions
#include <ESP8266WebServer.h>  // reuired for Server opertaions
#include<EEPROM.h>  // reuired for EEPROM opertaions

#define BAUD_RATE 115200
const byte INDICATOR = 2;

String onStatus = "0";

boolean WifiConnected = true;

void setup() {
  blinkIndicator();
  EEPROM.begin(512);
  setupWifi();
  Serial.begin(BAUD_RATE);
  initiateLocalServer();
  if (WifiConnected) {
    initilizeWebConn();
  }
}

void loop() {
  checkStatusUpdate();
  if (WifiConnected) {
    WebServerRun();
  }
  InternalServerRun();
  yield(); // this is most important part of the code, as it tells the esp8266 keep running background wifi work,
  //without this, your code  will disconnect from wifi, after long run of code.
}

// takes serial status data and stores in global status variable
void checkStatusUpdate() {
  if (Serial.available()) {
    delay(10);
    String data = "";
    do {
      // get the new byte:
      char inChar = (char)Serial.read();
      // add it to the inputString:
      data += inChar;
      // if the incoming character is a newline, set a flag
      // so the main loop can do something about it:
      if (inChar == '\n') {
        SerialFlush();
        onStatus = String(data);
        if (WifiConnected) {
          sendWebServerStatus(onStatus);
        }
      }
    } while (Serial.available());
  }
}

String readEEPROM(byte startAddr, byte endAddr) {
  String data;
  byte b;
  for (int i = startAddr; i <= endAddr; ++i) {
    b = EEPROM.read(i);
    data += char(b);
    if (b == 0) break;
  }
  return data;
}

void SerialFlush() {
  while (Serial.available()) {
    Serial.read();
  }
}

void setupWifi() {
  WiFi.mode(WIFI_STA);

  // starting wifi
  String esid = readEEPROM(0, 31);
  String epass = readEEPROM(32, 95);

  if ( esid.length() > 2 ) {
    if ( epass.length() > 2 ) {
      WiFi.begin(esid.c_str(), epass.c_str());
    } else {
      WiFi.begin(esid.c_str());
    }
    if (WiFiAvailable()) {
      return;
    }
  }

  WiFi.disconnect();
  WifiConnected = false;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Web Remote Receiver");
}

bool WiFiAvailable() {
  int c = 0;
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      blinkIndicator();
      return true;
    }
    delay(500);
    c++;
  }
  return false;
}

