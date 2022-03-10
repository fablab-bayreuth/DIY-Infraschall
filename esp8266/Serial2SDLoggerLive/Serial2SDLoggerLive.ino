/*
   This Sketch reads serial data an stores it on SD card

   default SSID: DIY-Infraschall
   default Password: fablab24

   Webfrontend on http://192.168.4.1

   Commands via WebSocket
   setConf
   getConf
   setTime
   getTime
   startLogging
   stopLogging
   startLive
   stopLive
   getDir
   deleteFile
   getAll

   download File is handled by Webserver

   You have to upload the sketch _AND_ the data in two steps
   for the data use ESP8266 Sketch Data Upload tool
   https://github.com/esp8266/arduino-esp8266fs-plugin

   Install the following libraries via library manager:
   - WebSockets (by Markus Sattler)
   - ArduinoJSON


*/
#define SW_VERSION "1.0.3"

#define LED_GREEN D1
#define LED_RED D2

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#include <WebSocketsServer.h>
WebSocketsServer webSocket(81);    // create a websocket server on port 81

#include "config.h"
#include "sd.h"
#include "live.h"
#include "socket.h"
#include "handler.h"


void setup(void) {
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_RED, HIGH);
  pinMode(LED_RED, OUTPUT);
  Serial.setRxBufferSize(8192);

  EEPROM.begin(sizeof(cfg));
  delay(1000);
  if (! digitalRead(0)) {
    uint8_t delete_count = 0;
    while (! digitalRead(0) && delete_count < 10) {
      digitalWrite(LED_GREEN, LOW);
      delay(50);
      digitalWrite(LED_GREEN, HIGH);
      delay(100);
      delete_count++;
    }
    if (! digitalRead(0)) {
      eraseConfig();
      digitalWrite(LED_RED, LOW);
      delay(200);
      digitalWrite(LED_RED, HIGH);
    }
    digitalWrite(LED_GREEN, LOW);
  }

  SPIFFS.begin();
  loadConfig();
  Serial.begin(cfg.baud);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(cfg.ssid, cfg.password);
  delay(100);
  IPAddress Ip(192, 168, 4, 1);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  server.on("/download", download);
  server.begin();
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);

}

void loop(void) {
  webSocket.loop();                           // constantly check for websocket events
  sendEvent();
  server.handleClient();
  if(device.logging) handleSerial();
  if(device.live) handleLive();
}
