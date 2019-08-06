#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>

extern WebServer         server;
extern AutoConnect       portal;
extern AutoConnectConfig config;

void web_setup();
void web_loop();
void web_start();