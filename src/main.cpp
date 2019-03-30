// Основано на https://github.com/dmonty2/ArduinoMotionStairLights
#include <Arduino.h>
#include <NeoPixelBus.h>
// осталось как предыдущая библиотека для работы в лентной, 
// теперь используются только некоторые алгоритмы и классы
#include <FastLED.h>

#define NUM_STEPS 22            // Количество ступеней в пролёте
#define LEDS_PER_STAIR 6       // Количество диодов на ступеньку
#define NUM_LEDS NUM_STEPS*LEDS_PER_STAIR // Всего диодов на лестнице
#define BRIGHTNESS 120          // 0...255  ( макс яркость, используется в fade7 )
//#define PIN_LED D1              // LED Data pin
#define PIN_PIR_DOWN D5         // PIR Downstairs Pin
#define PIN_PIR_UP D6           // PIR Upstairs Pin
#define GO_UP -1                // Direction control - Arduino at top of stairs
#define GO_DOWN 1               // Direction control - Arduino at top of stairs
uint8_t gHue = 0;               // track color shifts.
int16_t gStair = 0;             // track curent stair.
int16_t gStairLeds = 0;         // tracking lights per stair.
uint8_t gBright = 0;            // track brightness
uint16_t gUpDown[NUM_LEDS];     // directional array to walk/loop up or down stairs.
int8_t  gupDownDir = 1;         // direction of animation up or down
CRGB    leds[NUM_LEDS];         // setup leds object to access the string
CRGBPalette16 gPalette;         // some favorite and random colors for display.
CRGBPalette16 fade6 =          (CRGB( BRIGHTNESS, 0, 0),       CRGB(BRIGHTNESS,BRIGHTNESS,0), CRGB(0,BRIGHTNESS,0),
                                CRGB(0,BRIGHTNESS,BRIGHTNESS), CRGB(0,0,BRIGHTNESS),          CRGB(BRIGHTNESS, 0, BRIGHTNESS),
                                CRGB( BRIGHTNESS, 0, 0));
CRGBPalette16 z;
int8_t gLastPalette = 15;       // track last chosen palette.
unsigned long currentMillis = millis(); // define here so it does not redefine in the loop.
long previousMillis = 0;
long previousOffMillis = 0;     // момент последнего срабатывания, для таймера отключения
long offInterval = 10*1000;     // время отключения после срабатывания, 30сек
long effectInterval = 40;       // пауза (в миллисекундах) между обновлением эффекта
// Эффекты: шаги, вспышки, затухание
enum Effects { effectWalk, effectFlicker, effectFade };
Effects effect = effectWalk;
// Эффект для шагания: блеск, пульсация1, пульсация1, вспышки
enum WalkEffects { sparkle, pulsate1, pulsate2, flash };
WalkEffects walk_effect = sparkle;
// Этапы анимации.  Позволяем датчикам реактивировать состояние анимации.
enum Stage { off, stage_init, stage_grow, stage_init_run, stage_run, stage_init_dim, stage_dim };
Stage stage = off; // текущий этап анимации
int i = 0;
int x = 0;
uint8_t topBrightness = 200;  // May preserve LED life if not running at full brightness 255?
uint8_t rnd = 0;
uint8_t r = 0, g = 0, b = 0, h = 0, s = 0, v = 0; // red green blue hue sat val
int16_t stair = 0;
// текущие цвета для эффекта
CRGB c1;
CRGB c2;
CRGB trans;
CRGB trans2;
// счетчики последовательных срабатываний датчиков
int upc = 0;
int downc = 0;
// лента (пин фиксирован для esp8266 - D4)
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1Ws2813Method> strip(NUM_LEDS);


void log(const String &s){
  Serial.print(s);
}

void logln(const String &s){
  Serial.println(s);
}

void show(){
  RgbColor pixel;
  for (int i = 0; i < NUM_LEDS; i++)
  {
    pixel = RgbColor(leds[i].r, leds[i].g, leds[i].b);
    strip.SetPixelColor(i, pixel);
  }
  strip.Show();
}

void chooseEffects(){
  randomSeed(millis());
  r = random8(1, 255);
  //effect = effectFlicker; return;  // temporarily force effect for debugging: effectWalk, effectFlicker, effectFade
  if ( r >= 0 && r <= 100 ){
    effect = effectWalk;     // My favorite transition with random effect variations
  } else if ( r > 100 && r <= 175 ){
    effect = effectFlicker;  // Candle with embers.
  } else {
    effect = effectFade;    // hueshift rainbow.
  } 
}

// разметка порядка диодов для эффекта в зависимости от направления:
// 0,1,2,3... или наоборот:  ...3,2,1,0
void setUpDown(int8_t upDownDir){
  gupDownDir = upDownDir;
  int16_t gStairStart = 0;
  if (upDownDir == GO_UP){
    for ( gStair = NUM_LEDS -1; gStair >= 0; gStair-- ){
      gUpDown[gStair] = gStairStart++;
    }
  } else {
    for ( gStair = 0; gStair <= NUM_LEDS; gStair++ ){
      gUpDown[gStair] = gStairStart++;
    }  
  }
}

void readSensors(){
  bool up_detected = false;
  bool down_detected = false;
  // устраним ошибочное срабатывание.
  // только если подряд 30 раз сработал
  if ( digitalRead(PIN_PIR_UP) == LOW ){  // Walk Down.
    upc += 1;
    up_detected = (upc > 1);
  } else {
    upc = 0;
  }
  if ( digitalRead(PIN_PIR_DOWN) == LOW ){  // Walk Up.
    downc += 1;
    down_detected = (downc > 1);
  } else {
    downc = 0;
  }
  if ( up_detected ){  // Walk Down.
    //log("PIR UP LOW: ");
    //logln(String(upc));
    previousOffMillis = currentMillis;
    if ( stage == off ){
      logln("Start go down");
      chooseEffects();
      stage = stage_init;
      setUpDown(GO_DOWN);
    } else if ( stage == stage_dim || stage == stage_init_dim ){
      stage = stage_init_run;
    }
  }
  if ( down_detected ){ // Walk Up.
    //log("PIR DOWN LOW: ");
    //logln(String(downc));
    previousOffMillis = currentMillis;
    if ( stage == off ){
      logln("Start go up");
      chooseEffects();
      stage = stage_init;
      setUpDown(GO_UP);
    } else if ( stage == stage_dim || stage == stage_init_dim){
      stage = stage_init_run;
    }
  }
}

// Increment to the next color pair in the palette.
void choosePalette(){
  if ( gLastPalette >= 15 ) {
    gLastPalette = 0;
  } else {
    gLastPalette+=2;
  }
}

// Fill a palette with some colors that my wife picked.
void setPalette(){
  /*
   * Jenn's colors RGB  0 0 81  BLUE
   * 0 100 100 Teal 006464
   * 60 100 100 Cool White 3C6464
   * 60 10 100 Violet 3C0A64
   * 60 0 50 Purple 3C0032
   * start white fades to Teal
   * violet to purple
   * teal to blue
   * red to blue
   */
  uint8_t r = random8(1, 255); // call it once first.
  fill_solid( gPalette, 16, CRGB::Red);
  gPalette[0] = CRGB( 60, 100, 100 ); // Jenn cool white
  gPalette[1] = CRGB( 0, 90, 90 );    // Jenn teal
  gPalette[2] = CRGB( 60, 10, 100 );  // Jenn violet
  gPalette[3] = CRGB( 60, 0, 50 );    // Jenn purple
  gPalette[4] = CRGB( 0, 0, 81);      // Jenn blue
  gPalette[5] = CRGB( 100, 0, 0);     // Red
  gPalette[6] = CRGB( 0, 0, 100);     // Blue
  gPalette[7] = CRGB( 120, 0, 120);
  // Random fill the rest.
  for (uint8_t i = 8; i<16; i++){
    gPalette[i] = CRGB(random8(3,100), random8(3,100), random8(3,100));
  }
}

// Добавление случайных вспышек
void addGlitter( fract8 chanceOfGlitter) {
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB(100,100,100);
  }
}

// Random effects for the walk() stair function.
void randomEffect(){
  switch (walk_effect) {
    case sparkle:
      effectInterval = 8;
      fill_solid(leds, NUM_LEDS, c2);
      addGlitter(80);
      break;
    case pulsate1:
      effectInterval = 10;
      if ( b < 255 ){
        if ( i < 4 ) {
          trans2 = blend(z[i],z[i+1],b);
          fill_solid(leds, NUM_LEDS, trans2);
          b=qadd8(b,1);
        } else {
          i = 0;
        }
      } else {
        i++;
        b=0;
      }
      break;
    case pulsate2:
      effectInterval = 5;
      for(gStair=0; gStair < NUM_LEDS; gStair++) {
        trans2 = blend(c1,c2,quadwave8(r+=( -20 * gupDownDir )));
        leds[gStair] = trans2;
      }
      gStair = 0;
      r = ++g;
      break;
    case flash:
      if ( x == 0 ) {
        for(gStair=0; gStair <= (NUM_LEDS - LEDS_PER_STAIR); gStair+=LEDS_PER_STAIR) {
          for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
            leds[gUpDown[gStair + gStairLeds]] = CRGB( 100, 100, 100);
          }
          show();
        }
        for(gStair=0; gStair <= (NUM_LEDS - LEDS_PER_STAIR); gStair+=LEDS_PER_STAIR) {
          for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
            leds[gUpDown[gStair + gStairLeds]] = c2;
          }
          show();
        }
        x = 1;
        gStair=0;
      }
      break;
  }
}

// Шагание по лестнице с добавлением случайного эффекта
void walk() {
  // в зависимости от этапа
  switch (stage) {
    case stage_init: // инициализация
      logln("walk_stage_init");
      topBrightness = 200;
      // Взять два цвета из палитры
      choosePalette();
      c1 = gPalette[gLastPalette];
      c2 = gPalette[gLastPalette+1];
      // либо (если удача), взять случайные цвета
      if ( random8( 5 ) == 3 ){
        c1 = CRGB(random8(3,100),random8(3,100),random8(3,100));
        c2 = CRGB(random8(3,100),random8(3,100),random8(3,100));
      }
      // если выбранный цвет не яркий, то перевыбрать
      if ( (int(c1.r) + int(c1.g) + int(c1.b)) < 8 ){
        c1 = gPalette[2];
        c2 = gPalette[4];
      }
      trans = CRGB::Black;
      trans2 = CRGB::Black;
      z[0] = c2;
      z[1] = c1;
      z[2] = CRGB(random8(2,100),random8(2,100),random8(2,100));
      z[3] = c1;
      z[4] = c2;
      //(r2-r1)/ticks * tick)
      // текущая ступенька
      gStair=0;
      // текущая яркость
      gBright=0;
      effectInterval=5;
      i = 0;
      x = 0;
      r = 0;
      g = 0;
      b = 0;
      // выберем эффект для ожидания подъема
      walk_effect = (WalkEffects)random8( 0, 4 );
      // следующий этап - нарастание
      stage = stage_grow;
      break;
    case stage_grow: // нарастание
      // если яркость еще не полная
      if (gBright < 255){
        // если текущая ступенька еще не последняя
        if ( gStair <= (NUM_LEDS - LEDS_PER_STAIR)) {
          // blend - получаем значение цвета, в зависимости от текущей яркости
          // по яркости стремимся от черного к цвету c1
          trans = blend(CRGB::Black, c1, gBright);
          // выставим диоды для текущей ступеньки
          for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
            leds[gUpDown[gStair + gStairLeds]] = trans;
          }
        }
        // если текущая ступенька не первая
        if ( gStair >= LEDS_PER_STAIR ) { // shift last two stairs to the 2nd color.
          // blend - получаем значение цвета, в зависимости от текущей яркости
          // по яркости стремимся от c1 к цвету c2
          trans2 = blend(c1, c2, gBright);
          // выставим диоды для текущей ступеньки
          for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
            leds[gUpDown[gStair - (1 + gStairLeds)]] = trans2;
          }
        }
        // выставим следующий уровень яркости
        gBright = qadd8(gBright, 32); // 16 - скорость нарастания яркости ступеньки
      } else { // если мы уже добрали яркость ступеньки - перейдем к следующей
        // если текущая ступенька еще не последняя
        if ( gStair <= ( NUM_LEDS - LEDS_PER_STAIR ) ) {
          // перейдем к следующей ступеньке
          gStair += LEDS_PER_STAIR;
        } else {
          // если была последняя - перейдем к следующему этапу - анимация ожидания подъема
          stage = stage_init_run;
          gStair = 0;
        }
        gBright = 0;
      }
      break;
    case stage_init_run: // подготовка к ожиданию подъема
      logln("walk_stage_init_run");
      // заполним всю лестницу цветом c2
      fill_solid(leds, NUM_LEDS, c2);
      x = 0;
      // следующий этап - ожидание (пока кто-то поднимается)
      stage = stage_run;
      break;
    case stage_run: // ожидание подъема
      trans2 = c2;
      // крутим выбранный ранее эффект, пока не придет время гасить
      randomEffect();
      break;
    case stage_init_dim: // подготовка к гашению
      logln("walk_stage_init_dim");
      effectInterval = 3;
      for(b=0; b<255; b++) {
        trans = blend(trans2,c2,b);
        fill_solid(leds, NUM_LEDS, trans);
        show();
      }
      effectInterval = 8;
      gBright = 0;
      gStair = 0;
      logln("walk_stage_dim");
      stage = stage_dim;
      break;
    case stage_dim: // гашение
      if ( gBright <= topBrightness  ) {
        if ( gStair <= ( NUM_LEDS - LEDS_PER_STAIR ) ){
          for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
            leds[gUpDown[gStair + gStairLeds]].fadeToBlackBy( 16 );
          }
          gBright += 16;
        } else {
          logln("walk_stage_off");
          stage = off;
        }
      } else {
        for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
          leds[gUpDown[gStair + gStairLeds]] = CRGB( 0, 0, 0);
        }
        gStair += LEDS_PER_STAIR; 
        gBright = 0;
      }
      break;
    default:
      stage = off;
  }
}

// paint rainbow
void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

// built-in FastLED rainbow, plus some random sparkly glitter
void rainbowWithGlitter() {
  rainbow();
  addGlitter(80);
}

// Мерцание свечи
void flicker(){
  switch (stage) {
    case stage_init:
      logln("flicker_stage_init");
      i = 0;
      rnd = 0;
      r = 0; g = 0; b = 0;
      stair = 0;
      gStair = 0;
      x = 0;
      gBright = 0;
      effectInterval = 5;
      stage = stage_grow;
      break;
    case stage_grow:
      if ( i <= 10 ){  // number of flicker between steps
        if ( gStair <= (NUM_LEDS - LEDS_PER_STAIR)){  // for each step
          for ( stair = 0; stair <= gStair; stair += LEDS_PER_STAIR ){  // up to currently lit step.
            rnd = random8(1, 4);
            if ( rnd == 2 ){
              gBright = random8(110,140);
              for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
                leds[gUpDown[stair + gStairLeds]] = CHSV( 60, 200, gBright );
              }
            }
          }
          i++;
        } else {
          logln("flicker_stage_init_run");
          stage = stage_init_run;
        }
      } else {
        i = 0;
        gStair += LEDS_PER_STAIR;
      }
      break;
    case stage_init_run:
      stage = stage_run;
      break;
    case stage_run:
      for( gStair = 0; gStair <= (NUM_LEDS - LEDS_PER_STAIR); gStair += LEDS_PER_STAIR) {  
        rnd = random8(1, 4);
        if ( rnd == 2 ){
          gBright = random8(110,140);
          for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
            leds[gStair + gStairLeds] = CHSV( 60, 200, gBright );
          }
        }
      }
      break;
    case stage_init_dim:
      logln("flicker_stage_init_dim");
      // Blow out candles and leave an ember.
      for(gStair=0; gStair <= (NUM_LEDS - LEDS_PER_STAIR); gStair += LEDS_PER_STAIR) {
        rnd = random8(4, 6);
        r = rnd+1;
        g = rnd-2;
        for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
          leds[gUpDown[gStair + gStairLeds]] = CRGB( r,g,0 );
        }
        show();
      }
      i = 0;
      gStair=0;
      logln("flicker_stage_dim");
      stage = stage_dim;
      break;
    case stage_dim:
      if ( i <= 150 ){
        rnd = random8(0, NUM_LEDS);
        leds[gUpDown[rnd]].fadeToBlackBy( 3 );
        i++;
      } else {
        fill_solid(leds, NUM_LEDS, CRGB( 0, 0, 0 ));
        show();
        logln("flicker_stage_off");
        stage = off;
      }
      break;
    default:
      stage = off;
  }  
}

// Fade effect with each led using a hue shift
void fade(){
  switch (stage) {
    case stage_init:
      logln("fade_stage_init");
      gBright = 0;
      gStair = 0;
      effectInterval = 5;
      h = 128;
      s = 140;
      v = BRIGHTNESS;
      r = 0;
      g = ( random8() < 120 );
      stage = stage_grow;
      break;
    case stage_grow:
      if ( gBright<255 ){
        if ( gStair <= (NUM_LEDS - LEDS_PER_STAIR) ){
          trans = blend(CHSV(h,s,0),CHSV(h,s,v),gBright);
          for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
            leds[gUpDown[gStair + gStairLeds ]] = trans;
          }
          gBright = qadd8(gBright, 16);
        } else {
          stage = stage_init_run;
          gBright=0;
          gStair=0;
        }
        gBright = qadd8(gBright, 8);
      } else {
        gBright = 0;
        gStair += LEDS_PER_STAIR;
      }
      break;
    case stage_init_run:
      logln("fade_stage_init_run");
      v = BRIGHTNESS;
      effectInterval = 70;
      stage = stage_run;
      break;
    case stage_run:
      r = h;
      for(gStair=0; gStair < NUM_LEDS; gStair++) {
          h+=(3*gupDownDir); // left PIR go down
          leds[gUpDown[gStair]] = CHSV(h, s, v);
      }
      h = r + (3*gupDownDir*-1);
      break;
    case stage_init_dim:
      logln("fade_stage_init_dim");
      effectInterval = 7;
      h = h - gStair;
      gStair = 0;
      logln("fade_stage_dim");
      stage = stage_dim;
      break;
    case stage_dim:
      if ( v > 0 ) {
        if ( gStair <= (NUM_LEDS - LEDS_PER_STAIR )){
          for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
            leds[gUpDown[gStair + gStairLeds]] = CHSV(gStair + h, s, v);
          }
          v = qsub8(v, 16);
        } else {
          logln("fade_stage_off");
          stage = off;
        }
      } else {
        for ( gStairLeds=0; gStairLeds < LEDS_PER_STAIR; gStairLeds++ ){
          leds[gUpDown[gStair + gStairLeds]] = CRGB( 0, 0, 0);
        }
        gStair += LEDS_PER_STAIR; 
        v = BRIGHTNESS;
        h+=2;
      }
      break;
    default:
      stage = off;
  }
}

void update_effect(){
  switch (effect) {
    case effectWalk:
      walk();
      break;
    case effectFlicker:
      flicker();
      break;
    case effectFade:
      fade();
      break;
  }
}

// Приветственная радуга - даем датчикам раскачатьсяю. А еще - показатель, что программа работает :)
void welcomeRainbow(){
  for ( int i = 0; i < 500; i++ ){
    rainbowWithGlitter();
    show();
    EVERY_N_MILLISECONDS( 20 ) { gHue++; }
  }
  for (int tick=0; tick < 64; tick++){ 
    for ( uint16_t i = 0; i < NUM_LEDS; i++ ){
      leds[i].fadeToBlackBy( 64 );
      show();
    }
  }
}

void setup() {
  Serial.begin(115200); // инициализация порта для отладки
  logln("start...");
  strip.Begin();
  strip.ClearTo(RgbColor(0));
  strip.Show();
  delay (3000); // ожидание 3 секунды на загрузку
  randomSeed(millis());
  pinMode(PIN_PIR_DOWN, INPUT);
  pinMode(PIN_PIR_UP, INPUT);
  digitalWrite(PIN_PIR_DOWN, LOW);
  digitalWrite(PIN_PIR_UP, LOW);
  //welcomeRainbow();             // запускаем приветственную радугу - даем время датчикам запуститься
  logln("started");
  setUpDown(GO_DOWN);           // populate the array index used for stair direction.
  setPalette();                 // setup some favorite & random colors
  stage = off;
}

// Основной цикл.
void loop() {
  currentMillis = millis();
  // Считываем датчики
  readSensors();
  // выжидаем нужную паузу и обновляем эффект
  if(currentMillis - previousMillis > effectInterval) {
    previousMillis = currentMillis;
    // обновляем эффект
    update_effect(); 
    // отображаем на ленте
    show();
  }
  // если пришло время выключить эффект - выключаем
  if((currentMillis - previousOffMillis > offInterval) && stage == stage_run){
    stage = stage_init_dim;
    i = 0; r = 0; g = 0; b = 0;
  }
}
