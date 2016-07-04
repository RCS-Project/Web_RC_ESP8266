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


#include<Ticker.h>  // timer operation

Ticker blinkShutter;
volatile boolean indicatorOn = false;
extern const byte INDICATOR;

Ticker autoReceiver;  // timer to receive data from internet

volatile boolean Ticked = false;

char host[40];
String webPath = ""; // web address to connect to

WiFiClient WebServer;
const byte httpPort = 80;

void initilizeWebConn() {
  // initiation the indicator
  pinMode(INDICATOR, OUTPUT);
  
  // initializing webPath and host
  String hst = readEEPROM(96, 199);
  byte i;
  for (i = 0; i < hst.length(); i++) {
    if (hst[i] == '/') {
      webPath = hst.substring(i);
      break;
    }
    host[i] = hst[i];
  }
  host[i] = '\0';

  // start reciever
  autoReceiver.attach(2, tick);
}

void WebServerRun() {
  if (Ticked) {
    recvRemoteData();
    Ticked = false;
  }
}

// send status code to web server
// automatically called by serial event handler
void sendWebServerStatus(String Status) {
  byte errCount = 0;
  do {
    if (!WebServer.connect(host, httpPort)) {
      errCount++;
      continue;
    }
    String url = webPath;
    url += "/svc/sendStatus.php?status=";
    url += Status;

    WebServer.print(String(F("GET ")) + url + F(" HTTP/1.1\r\n"
                    "Host: ") + host + F("\r\n"
                                         "Connection: close\r\n\r\n"));
    if (!ResponseReady(5000)) {
      errCount++;
      continue;
    }
    String line;
    while (WebServer.available()) {
      line = WebServer.readStringUntil('\r');
    }
    char c = line.charAt(line.length() - 1);
    if (c == 'S') {
      blinkIndicator();
      break;
    } else {
      errCount++;
    }
  } while (errCount != 3);
  yield(); // this is most important part of the code, as it tells the esp8266 keep running background wifi work,
  //without this, your code  will disconnect from wifi, after long run of code.
}

// receive any pending remote data
void recvRemoteData() {
  if (!WebServer.connect(host, httpPort)) {
    return;
  }

  String url = webPath;
  url += "/svc/recvRemoteData.php";

  // Requesting URL
  WebServer.print(String(F("GET ")) + url + F(" HTTP/1.1\r\n"
                  "Host: ") + host + F("\r\n"
                                       "Connection: close\r\n\r\n"));
  if (!ResponseReady(5000)) {
    return;
  }
  String line;
  while (WebServer.available()) {
    line = WebServer.readStringUntil('\r');
  }
  char c = line.charAt(line.length() - 1);
  if ((c >= '1' && c <= '9') || c == 'P' || c == 'S') {
    blinkIndicator();
    Serial.print(c);
  }
}

// waits till WebServer respone is ready upto timeout
boolean ResponseReady(unsigned int timeout) {
  unsigned long start = millis();
  while (!WebServer.available()) {
    if (millis() - start > timeout) {
      WebServer.stop();
      return false;
    }
  }
  return true;
}

void tick() {
  Ticked = true;
}

void blinkIndicator() {
  if (indicatorOn) {
    blinkShutter.detach();
  }
  digitalWrite(INDICATOR, LOW); // inverted o/p
  indicatorOn = true;
  
  blinkShutter.once_ms(300, []() {
    digitalWrite(INDICATOR, HIGH);
    indicatorOn = false;
  });
}

