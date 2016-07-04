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


#include<FS.h>  // reuired for SPIFFS opertaions

ESP8266WebServer LocalServer(80);

extern String onStatus;

void initiateLocalServer() {
  SPIFFS.begin();

  // open local app
  LocalServer.on("/", []() {
    File page = SPIFFS.open("/index.html", "r");
    LocalServer.streamFile(page, "text/html");
    page.close();
  });

  // host is dynamically generated
  // local IP address is send as host
  LocalServer.on("/host.js", []() {
    String content = String("host = \"") + WiFi.localIP().toString() + "\";\r\n";
    LocalServer.send(200, "text/javascript", content);
  });

  // host is dynamically generated
  // a javascript array of options is send
  LocalServer.on("/wifilist.js", []() {
    String Options = "var Options = [";
    byte n = WiFi.scanNetworks();
    if (n > 0) {
      for (byte i = 0; i < n; ++i) {
        String nw = String((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "Open : " : "Protected : ") + WiFi.SSID(i);
        Options += "{\"value\":\"" + nw + "\"}";
        if (i != (n - 1))
          Options += ",\r\n";
        else
          Options += "\r\n";
      }
    }
    Options += "];";
    LocalServer.send(200, "text/javascript", Options);
  });

  // configuration page
  LocalServer.on("/config", []() {
    File page = SPIFFS.open("/config.html", "r");
    LocalServer.streamFile(page, "text/html");
    page.close();
  });

  // config update command
  LocalServer.on("/updateconfig", []() {
    String ssidType = "";
    String qsid = URLDecode(LocalServer.arg("ssid"));
    String qpass = URLDecode(LocalServer.arg("pass"));
    String qhost = URLDecode(LocalServer.arg("host"));

    // cut out description part
    if (qsid.indexOf(':') != -1) {
      ssidType = qsid.substring(0, qsid.indexOf(':') - 1);
      qsid = qsid.substring(qsid.indexOf(':') + 2);
    }
    // invalid
    if (!ssidType.equals("Protected") || !(qpass.length() < 8)) {
      // clearing EEPROM
      for (byte i = 0; i < 200; ++i) {
        EEPROM.write(i, 0);
      }

      // invalid qsid is received
      if (qsid.length() < 1) {
        qsid = "-none-";
      } else { // valid ssid
        // writing SSID
        for (byte i = 0; i < qsid.length(); ++i) {
          EEPROM.write(i, qsid[i]);
        }
      }

      // valid pass
      if (qpass.length() >= 8) {
        // writing PASSWORD
        for (byte i = 0; i < qpass.length(); ++i) {
          EEPROM.write(32 + i, qpass[i]);
        }
      }

      // invalid qhost is received
      if (qhost.length() < 1) {
        qhost = WiFi.localIP().toString();
      }
      // writing host
      for (byte i = 0; i < qhost.length(); ++i) {
        EEPROM.write(96 + i, qhost[i]);
      }

      EEPROM.commit();

      String content = String(F("<!DOCTYPE HTML>\r\n"
                                "<html>\n"
                                "  <body style=\"text-align: center;\">\n"
                                "    <h3>WiFi settings updated</h3>\n"
                                "    <h4>SSID: ")) + qsid + F("</h4>\n"
                                    "    <h4>host: ") + qhost + F("</h4>\n"
                                        "    <h3>Restart to Connect</h3>\n"
                                        "  </body>\n"
                                        "</html>");

      LocalServer.send(200, "text/html", content);

      delay(100);
      // restart ESP
      ESP.restart();
    } else {
      String content = F("<!DOCTYPE HTML>\r\n"
                         "<html>\n"
                         "  <body style=\"text-align: center;\">\n"
                         "    <h3>Please required credentials</h3>\n"
                         "  </body>\n"
                         "</html>");
      LocalServer.send(404, "text/html", content);
    }
  });

  // local services
  // Sending status
  LocalServer.on("/getStatus.php", []() {
    blinkIndicator();
    LocalServer.send(200, "text/html", onStatus);
  });

  // receiving remote data
  LocalServer.on("/sendRemoteData.php", []() {
    String data = LocalServer.arg("data");

    blinkIndicator();
    // first char is actual data
    Serial.print(data[0]);

    // return empty response
    LocalServer.send(200, "text/html", data);
  });

  // starting the local server with the defined handlers
  LocalServer.begin();
}

void InternalServerRun() {
  LocalServer.handleClient();
}

String URLDecode(String param) {
  param.replace("+", " ");
  param.replace("%21", "!");
  param.replace("%23", "#");
  param.replace("%24", "$");
  param.replace("%26", "&");
  param.replace("%27", "'");
  param.replace("%28", "(");
  param.replace("%29", ")");
  param.replace("%2A", "*");
  param.replace("%2B", "+");
  param.replace("%2C", ",");
  param.replace("%2F", "/");
  param.replace("%3A", ":");
  param.replace("%3B", ";");
  param.replace("%3D", "=");
  param.replace("%3F", "?");
  param.replace("%40", "@");
  param.replace("%5B", "[");
  param.replace("%5D", "]");
  return param;
}

