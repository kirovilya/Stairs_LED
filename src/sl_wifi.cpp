#include "sl_wifi.h"

// веб-сервер
#define SoftAPName "StairsLED"
ESP8266WebServer  server(80);
// AutoConnect       portal(server);
// AutoConnectConfig config;
WiFiManager wifiManager;


// void handleRoot() {
//     String  content =
//         "<html>"
//         "<head>"
//         "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
//         "</head>"
//         "<body>"
//         "<h2 align=\"center\" style=\"color:blue;margin:20px;\">Hello, world</h2>"
//         "<h3 align=\"center\" style=\"color:gray;margin:10px;\">{{DateTime}}</h3>"
//         "<p style=\"text-align:center;\">Reload the page to update the time.</p>"
//         "<p></p><p style=\"padding-top:15px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
//         "</body>"
//         "</html>";
//     static const char *wd[7] = { "Sun","Mon","Tue","Wed","Thr","Fri","Sat" };
//     struct tm *tm;
//     time_t  t;
//     char    dateTime[26];

//     t = time(NULL);
//     tm = localtime(&t);
//     sprintf(dateTime, "%04d/%02d/%02d(%s) %02d:%02d:%02d.",
//         tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
//         wd[tm->tm_wday],
//         tm->tm_hour, tm->tm_min, tm->tm_sec);
//     content.replace("{{DateTime}}", String(dateTime));
//     server.send(200, "text/html", content);
// }


void web_setup(){
    // отключим коннект к известным сетям, если его специально сбросили
    // чтобы поднялась точка доступа
    // config.autoReconnect = false;
    // config.apid = SoftAPName;
    // config.psk  = "";
    // portal.config(config);
    wifiManager.autoConnect("AutoConnectAP");
    Serial.println("local ip");
    Serial.println(WiFi.localIP());
}

void web_loop(){
    //portal.handleClient();
}

void web_start(){
    // webserver
    // server.on("/", handleRoot);
    // if (portal.begin()) {
    //     Serial.println("WiFi connected: " + WiFi.localIP().toString());
    // }
    Serial.println("HTTP server started");
}