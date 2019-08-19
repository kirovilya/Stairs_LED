// для АЦП 
#include <Wire.h>
#include <Adafruit_ADS1015.h>

#define PIN_PIR_DOWN D5         // PIR Downstairs Pin
#define PIN_PIR_UP D6           // PIR Upstairs Pin

// счетчики последовательных срабатываний датчиков
int upc = 0;
int downc = 0;
bool up_detected = false;
bool down_detected = false;

// плата АЦП на пинах D1 D2
Adafruit_ADS1115 ads(0x48);

void sensor_setup() {
    // pinMode(PIN_PIR_DOWN, INPUT);
    // pinMode(PIN_PIR_UP, INPUT);
    // digitalWrite(PIN_PIR_DOWN, LOW);
    // digitalWrite(PIN_PIR_UP, LOW);
    ads.begin();
}

void sensor_loop() {
    // Считываем датчики
    int16_t adc0, adc1, adc2, adc3;
    adc0 = ads.readADC_SingleEnded(0);
    adc1 = ads.readADC_SingleEnded(1);
    //Serial.print("ADC0: ");
    //Serial.print(adc0);
    //Serial.print("  ADC1: ");
    //Serial.println(adc1);
    if (adc0 > 2000) {  // Walk Down
        upc += 1;
        up_detected = (upc > 2);
    } else {
        up_detected = false;
        upc = 0;
    }
    if (adc1 > 2000) {  // Walk Up
        downc += 1;
        down_detected = (downc > 2);
    } else {
        down_detected = false;
        downc = 0;
    }
}