// Основано на https://github.com/dmonty2/ArduinoMotionStairLights
#include "utils.h"
#include "sl_leds.h"
#include "sl_sensor.h"
// #include "sl_wifi.h"
// #include "sl_mqtt.h"
// OTA
#include <ArduinoOTA.h>

void setup() {
  Serial.begin(115200); // инициализация порта для отладки
  logln("start...");
  
  leds_setup();
  sensor_setup();

  logln("started");
  
  // настройка WIFI, веб-сервера
//  web_setup();
  // mqtt
//  mqtt_setup();
  
//  web_start();

  //

  // ArduinoOTA.onStart([]() {
  //   Serial.println("Start");  //  "Начало OTA-апдейта"
 
  // });
  // ArduinoOTA.onEnd([]() {
  //   Serial.println("\nEnd");  //  "Завершение OTA-апдейта"
  // });
  // ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  //   Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  // });
  // ArduinoOTA.onError([](ota_error_t error) {
  //   Serial.printf("Error[%u]: ", error);
  //   if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
  //   //  "Ошибка при аутентификации"
  //   else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
  //   //  "Ошибка при начале OTA-апдейта"
  //   else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
  //   //  "Ошибка при подключении"
  //   else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
  //   //  "Ошибка при получении данных"
  //   else if (error == OTA_END_ERROR) Serial.println("End Failed");
  //   //  "Ошибка при завершении OTA-апдейта"
  // });
  // ArduinoOTA.begin();
}

// Основной цикл.
void loop() {
    // ArduinoOTA.handle();
    //delay(200);
    sensor_loop();

    if (up_detected) {  // Walk Down.
        runGoDown();
    }
    if (down_detected) {  // Walk Up.
        runGoUp();
    }

    leds_loop();
  
  // webserver
//  web_loop();
  // mqtt
//  mqtt_loop();
    delay(10);
}