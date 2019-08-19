//#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
//#include <AutoConnect.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

extern ESP8266WebServer  server;
//extern AutoConnect       portal;
//extern AutoConnectConfig config;
extern WiFiManager wifiManager;

void web_setup();
void web_loop();
void web_start();