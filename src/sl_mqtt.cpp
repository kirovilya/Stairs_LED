#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <PubSubClient.h>
#include "sl_mqtt.h"
#include "sl_wifi.h"

#define GET_CHIPID()  (ESP.getChipId())

// MQTT клиент
#define PARAM_FILE      "/param.json"
#define AUX_SETTING_URI "/mqtt_setting"
#define AUX_SAVE_URI    "/mqtt_save"
#define AUX_CLEAR_URI   "/mqtt_clear"
#define PREFIX          "/stairs"
// JSON definition of AutoConnectAux.
// Multiple AutoConnectAux can be defined in the JSON array.
// In this example, JSON is hard-coded to make it easier to understand
// the AutoConnectAux API. In practice, it will be an external content
// which separated from the sketch, as the mqtt_RSSI_FS example shows.
static const char AUX_mqtt_setting[] PROGMEM = R"raw(
[
  {
    "title": "MQTT Setting",
    "uri": "/mqtt_setting",
    "menu": true,
    "element": [
      {
        "name": "header",
        "type": "ACText",
        "value": "<h2>MQTT broker settings</h2>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "caption",
        "type": "ACText",
        "value": "",
        "style": "font-family:serif;color:#4682b4;"
      },
      {
        "name": "mqttserver",
        "type": "ACInput",
        "value": "",
        "label": "Server",
        "pattern": "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\\-]*[a-zA-Z0-9])\\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\\-]*[A-Za-z0-9])$",
        "placeholder": "MQTT broker server"
      },
      {
        "name": "port",
        "type": "ACInput",
        "value": "",
        "label": "Port",
        "pattern": "^[0-9]*$"
      },
      {
        "name": "username",
        "type": "ACInput",
        "label": "User name"
      },
      {
        "name": "userpass",
        "type": "ACInput",
        "label": "User pass"
      },
      {
        "name": "newline",
        "type": "ACElement",
        "value": "<hr>"
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save&amp;Start",
        "uri": "/mqtt_save"
      },
      {
        "name": "discard",
        "type": "ACSubmit",
        "value": "Discard",
        "uri": "/"
      }
    ]
  },
  {
    "title": "MQTT Setting",
    "uri": "/mqtt_save",
    "menu": false,
    "element": [
      {
        "name": "caption",
        "type": "ACText",
        "value": "<h4>Parameters saved as:</h4>",
        "style": "text-align:center;color:#2f4f4f;padding:10px;"
      },
      {
        "name": "parameters",
        "type": "ACText"
      }
    ]
  }
]
)raw";


WiFiClient espClient;
PubSubClient mqttClient(espClient);
String  serverName;
String  port;
String  username;
String  userpass;
String  hostName;
// веря последней попытки подключения к MQTT
long lastMqttReconnectAttempt = 0;
int attemptCount = 0;


bool mqttConnect() {
    static const char alphanum[] = "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";  // For random generation of client ID.
    char    clientId[9];

    if (!mqttClient.connected()) {
        // после 10 попыток прекращаем долбиться
        if (attemptCount > 10) {
            return false;
        }
        // если с момента последней попытки прошло больше 5 секунд 
        // - пробуем подключиться снова
        if (millis() - lastMqttReconnectAttempt > 5000) {
            lastMqttReconnectAttempt = millis();
            mqttClient.setServer(serverName.c_str(), port.toInt());
            Serial.println(String("Attempting MQTT broker:") + serverName);
            for (uint8_t i = 0; i < 8; i++) {
                clientId[i] = alphanum[random(62)];
            }
            clientId[8] = '\0';

            if (mqttClient.connect(clientId, username.c_str(), userpass.c_str())) {
                Serial.println("Established:" + String(clientId));
                // сбросим время последней попытки подключения
                lastMqttReconnectAttempt = 0;
                attemptCount = 0;
                return true;
            }
            else {
                attemptCount++;
                Serial.println("Connection failed:" + String(mqttClient.state()));
            }
        }
    }
    return false;
}

void mqtt_publish(String topic, String value) {
    if (mqttClient.connected()) {
        String path = PREFIX + String("/") + topic;
        mqttClient.publish(path.c_str(), value.c_str());
    }
}

int getStrength(uint8_t points) {
  uint8_t sc = points;
  long    rssi = 0;

  while (sc--) {
    rssi += WiFi.RSSI();
    delay(20);
  }
  return points ? static_cast<int>(rssi / points) : 0;
}

// Load parameters saved with  saveParams from SPIFFS into the
// elements defined in /mqtt_setting JSON.
String loadParams(AutoConnectAux& aux, PageArgument& args) {
    (void)(args);
    File param = SPIFFS.open(PARAM_FILE, "r");
    if (param) {
        if (aux.loadElement(param))
            Serial.println(PARAM_FILE " loaded");
        else
            Serial.println(PARAM_FILE " failed to load");
        param.close();
    }
    else {
        Serial.println(PARAM_FILE " open failed");
    }
    return String("");
}

// Save the value of each element entered by '/mqtt_setting' to the
// parameter file. The saveParams as below is a callback function of
// /mqtt_save. When invoking this handler, the input value of each
// element is already stored in '/mqtt_setting'.
// In Sketch, you can output to stream its elements specified by name.
String saveParams(AutoConnectAux& aux, PageArgument& args) {
    // The 'where()' function returns the AutoConnectAux that caused
    // the transition to this page.
    AutoConnectAux*   mqtt_setting = portal.where();

    AutoConnectInput& mqttserver = mqtt_setting->getElement<AutoConnectInput>("mqttserver");
    serverName = mqttserver.value;
    serverName.trim();

    AutoConnectInput& portv = mqtt_setting->getElement<AutoConnectInput>("port");
    port = portv.value;
    port.trim();

    AutoConnectInput& usernamev = mqtt_setting->getElement<AutoConnectInput>("username");
    username = usernamev.value;
    username.trim();

    AutoConnectInput& userpassv = mqtt_setting->getElement<AutoConnectInput>("userpass");
    userpass = userpassv.value;

    // The entered value is owned by AutoConnectAux of /mqtt_setting.
    // To retrieve the elements of /mqtt_setting, it is necessary to get
    // the AutoConnectAux object of /mqtt_setting.
    File param = SPIFFS.open(PARAM_FILE, "w");
    mqtt_setting->saveElement(param, { "mqttserver", "port", "username", "userpass" });
    param.close();

    // Echo back saved parameters to AutoConnectAux page.
    AutoConnectText&  echo = aux.getElement<AutoConnectText>("parameters");
    echo.value = "Server: " + serverName;
    echo.value += mqttserver.isValid() ? String(" (OK)") : String(" (ERR)");
    echo.value += "<br>Port: " + port + "<br>";
    echo.value += "User name: " + username + "<br>";
    echo.value += "User pass: " + userpass + "<br>";

    return String("");
}

void mqtt_setup() {
    SPIFFS.begin();
    if (portal.load(FPSTR(AUX_mqtt_setting))) {
        AutoConnectAux* mqtt_setting = portal.aux(AUX_SETTING_URI);
        PageArgument  args;
        loadParams(*mqtt_setting, args);
        AutoConnectInput& mqttserver = mqtt_setting->getElement<AutoConnectInput>("mqttserver");
        serverName = mqttserver.value;
        serverName.trim();
        
        AutoConnectInput& portv = mqtt_setting->getElement<AutoConnectInput>("port");
        port = portv.value;
        port.trim();
        
        AutoConnectInput& usernamev = mqtt_setting->getElement<AutoConnectInput>("username");
        username = usernamev.value;
        username.trim();

        AutoConnectInput& userpassv = mqtt_setting->getElement<AutoConnectInput>("userpass");
        userpass = userpassv.value;

        config.bootUri = AC_ONBOOTURI_HOME;
        config.homeUri = "/";
        portal.config(config);

        portal.on(AUX_SETTING_URI, loadParams);
        portal.on(AUX_SAVE_URI, saveParams);
    }
    else
        Serial.println("load error");
}

void mqtt_loop() {
    if (!mqttClient.connected()) {
        mqttConnect();
    } else {
        mqttClient.loop();
    }
}