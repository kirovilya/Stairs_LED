#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>

extern ESP8266WebServer  server;
extern AutoConnect       portal;
extern AutoConnectConfig config;

void web_setup();
void web_loop();
void web_start();