#include <Arduino.h>
/*
   Copyright (c) 2024 by do8pgg. 
   Contact: aprs@do8pgg.de.
   
   All rights reserved.

          ***
   Please note that the following source code is licensed under the GNU Lesser General Public License
   as well as the OSL-EU License.
   Be sure to comply with the license terms specified at the end of this source code!
          ***

  If you find this software helpful and would like to express your gratitude,
  consider buying me a coffee... Or Beer. Your support is greatly appreciated! 😊
    via Paypal: paypal@nsa2go.de
*/

  #include <Wire.h>
  #include <AHT20.h>          // https://github.com/dvarrel/AHT20
  #include <BMP280.h>         // https://github.com/dvarrel/BMP280
  #include <WiFiManager.h>    // tzapu / tablatronix
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>

  #include <ESP8266WiFi.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266WebServer.h>
            ESP8266WebServer server(80);
            ESP8266WebServer httpServer(81);
  #include <ESP8266HTTPUpdateServer.h>
            ESP8266HTTPUpdateServer httpUpdater;
  const char* WIFI_SETUP_SSID = "aprs-esp8266";

float temperatureC;
int temperatureF;
int pressure;
int humidity;
int offset_pres = 0;    // Offset for pressure
int offset_temp = 0.0;  // Offset for temperature
int offset_humi = 4.4;  // Offset for humidity
// Important for the aht20, which is always slightly off the mark!

String IPaddress;

unsigned long previousMillis = 0;

AHT20 aht20;
BMP280 bmp280;
WiFiManager wm;

// Location
const char* lat = "5119.24N";
const char* lon = "00719.74E";

// APRS data
// Callsign and SSID (SSID 13 for WX stations)
const char* APRScall = "do8pgg-13";

// Your APRS-PassCODE e.g. 12345. NOT! your APRS-PassWORD
const char* APRSpasscode = "12345";

// APRS-Host
const char* host = "euro.aprs2.net";
//    Available servers:
// North America    noam.aprs2.net
// South America    soam.aprs2.net
// Europe & Africa  euro.aprs2.net
// Asia             asia.aprs2.net
// Oceania          aunz.aprs2.net


// APRS-Port
int port = 14580;

// Interval - Please set to 10 minutes, as shorter intervals are sometimes ignored.
//const long interval = 300000; // for Coding/Testing
const long interval = 600000;

// Build-Info
String buildversion = "v0.1b14";
String QTH = "Gevelsberg-Kipp";

/*
 // Planed for line 133 and 134
String greeter = "vy73 de Peter, do8pgg"; 
char stationname[] = "WX-Station-outdoor.";
*/

void aprs_sender() {
  digitalWrite(2, LOW); // Activate LED during transmission
  WiFiClient client;

  Serial.printf("\n[Connecting to %s ", host);
  Serial.print (":");
  Serial.print (port);
  Serial.println("....");

  if (client.connect(host, port))
  {
    Serial.println("[Connected]");
    Serial.println("Login to APRS server");
    char login[60];
    char sequence[150];
    // Calculate the temperature of both sensors to obtain an average value for both.
    float atemperature = aht20.getTemperature();  // Read temperature from aht20
    float btemperature = bmp280.getTemperature(); // Read temperature from bmp280
    float averageTemperature = (atemperature + btemperature) / 2 - offset_temp;

    // Conversion to Fahrenheit, aprs.fi apparently wants it that way
    temperatureF = (averageTemperature * 1.8) + 32;

    // Setting the air pressure value for the aprs.fi 'diva' :-) 
    pressure = (bmp280.getPressure() / 10);

    // Humidity Correction
    humidity = aht20.getHumidity() + offset_humi;

    delay(250);
    // Debuging-Stuff
    /*
          Serial.print("(averageTemperature) ");
          Serial.println((averageTemperature));

          Serial.print("pressure ");
          Serial.println(pressure);

          Serial.print("temperatureF ");
          Serial.println(temperatureF);

          Serial.print("humidity ");
          Serial.println(humidity);

          Serial.print("bmp280.getPressure() ");
          Serial.println(bmp280.getPressure());

          Serial.println();
          Serial.println();
          Serial.println();

    */
    //Send-Voodoo
    sprintf(login, "user %s pass %s vers WX_Station 0.1 filter m/1", APRScall, APRSpasscode);
    sprintf(sequence, "%s>APRS,TCPIP*:@090247z%s/%s_.../...t%03dh%02db%05dWX-Station-outdoor. vy73 de Peter, do8pgg", APRScall, lat, lon, temperatureF, humidity, pressure);

//    client.println(login);
    //    Serial.println(sequence);
    Serial.println("[Response:]");
    while (client.connected() || client.available())
    {
      if (client.available())
      {
        String line = client.readStringUntil('\n');
        Serial.println(line);
        delay(3000);
        Serial.println("Send: ");
        Serial.println(sequence);
//        client.println(sequence);
        delay(1000);
        client.stop();
      }
    }
    client.stop();
    Serial.println("\n[Disconnected]");
  }
  else
  {
    Serial.println("[Connection failed!]");
    client.stop();
  }
  digitalWrite(2, HIGH); // Deactivate LED after transmission
}

void OTA() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

}


void handleRoot() {
  float atemperature = aht20.getTemperature();
  float btemperature = bmp280.getTemperature();
  float averageTemperature = (atemperature + btemperature) / 2 - offset_temp;
  float humidity = aht20.getHumidity();
  uint32_t pressure = bmp280.getPressure();

  String html = "<html><head>";
  html += "<script>setTimeout(function(){ location.reload(); }, 60000);</script>";
  html += "</head><body>";
  html += "<h1>Weather data " + QTH + "</h1><br>";
  html += "<p>Air Temperaturr: " + String(averageTemperature, 2) + " &deg;C</p>";
  html += "<p>Air humidity: " + String(humidity + offset_humi, 1) + " % RH</p>";
  html += "<p>Air pressure: " + String(pressure / 100) + " hPa</p><br><br>";
  html += "<br>Version: " + buildversion;
  html += "<br><a href=http://" + IPaddress + ":81/update" + ">Update Firmware</a>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}


void loop() {
  //  delay(1000);
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    aprs_sender();
  }
  ArduinoOTA.handle();
  server.handleClient();
  httpServer.handleClient();
}

void connector() {
  bool res;
  delay(100);
  res = wm.autoConnect(WIFI_SETUP_SSID);
  if (!res) {
    Serial.println("Failed to connect");
    Serial.println("\n");
  } else {
    Serial.println("Connectet...yay :)");
    Serial.println("\n");
    IPaddress = WiFi.localIP().toString();
    Serial.println(IPaddress);
    ArduinoOTA.setHostname(APRScall);
    ArduinoOTA.handle();
  }
}

void webupdater() {
  MDNS.begin(host);
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 81);
}

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  connector();
  delay(1000);
  Wire.begin();
  bmp280.begin();
  delay(125);
  aht20.begin();
  delay(250);
  OTA();
  server.on("/", handleRoot);
  server.begin();
  webupdater();
  aprs_sender();
}



/*
  Copyright (c) 2024 by do8pgg

  This Code is free software; you can redistribute it and/or
  modify it under the terms of the Open-source software license for ethical use (OSL-EU),
  version 2.1 of the License, or (at your option) any later version.

  This Code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this Code; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

  If you find this software helpful and would like to express your gratitude,
  consider buying me a coffee... Or Beer. Your support is greatly appreciated! 😊

  Please note that the use of this software by racists, fascists, or any individuals or organizations
  promoting hate, discrimination, or any form of illegal or immoral activities is strictly prohibited.
  By using this software, you confirm that you do not associate it with such disallowed activities
  and respect democratic principles.
*/
