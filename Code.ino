#include <EEPROM.h>

#include<LiquidCrystal_I2C.h>
#include<iarduino_RTC.h>
#include<FastLED.h>

#include"RTOS.h"
#include"Button.h"
#include"Chars.h"

#define INC_BTN 10
#define DEC_BTN 4
#define MODE_BTN 2

#define PUMP 3
#define COOL 11
#define LED 6
#define RELE A6

#define RST_PIN 8
#define CLK_PIN 7
#define DAT_PIN 9

#define HUM_SENSOR A0

#define NUM_OF_LEDS 42

bool isPumpEnabled;
bool isCoolEnabled;
bool isLedEnabled;

LiquidCrystal_I2C lcd(0x27, 16, 2);
iarduino_RTC watch(RTC_DS1302, RST_PIN, CLK_PIN, DAT_PIN);

Button mode(MODE_BTN, 500, true);
Button inc(INC_BTN, 500, false);
Button dec(DEC_BTN, 500, false);

//ButtonsCombination debugMode(mode, inc);

//bool isDebugModeStarted = false;                        //Режим отладки

unsigned long lastColonDerp = 0;                          //Время моргания (:) часов 
bool colonState = 1;

unsigned long lastButtonPress = 0;

byte currentMode = 0;                                      //0 - Sleep mode, 1 - Menu, 2 - Clock cfg, 3 - Wind cfg, 4 - Watering cfg, 5 - Light cfg

bool isModeChanged = false;
bool ignoreSleep = true;

byte mins, hours_;

//EEPROM addresses
#define COOLING_START_HOURS 0x0000
#define COOLING_START_MINS 0x0001
#define COOLING_STOP_HOURS 0x0002
#define COOLING_STOP_MINS 0x0003

#define WATERING_START_HOURS 0x0004
#define WATERING_START_MINS 0x0005
#define WATERING_STOP_HOURS 0x0006
#define WATERING_STOP_MINS 0x0007

#define LIGHT_START_HOURS 0x0008
#define LIGHT_START_MINS 0x0009
#define LIGHT_STOP_HOURS 0x000A
#define LIGHT_STOP_MINS 0x000B

byte coolingStartHours = 12;
byte coolingStartMins = 0;
byte coolingStopHours = 12;
byte coolingStopMins = 5;

byte wateringStartHours = 12;
byte wateringStartMins = 0;
byte wateringStopHours = 12;
byte wateringStopMins = 5;

byte lightStartHours = 12;
byte lightStartMins = 0;
byte lightStopHours = 12;
byte lightStopMins = 5;

/*If mode == menu, so 0 - quit, 1 - Light, 2 - Cooling, 3 - Watering, 4 - Clock;*/
/*If mode == Clock cfg, so 0 - back, 1 - hour, 2 - min, 3 - OK*/
/*If mode == Wind cfg, so 0 - back, 1 - hour (begin), 2 - mins (begin), 3 - hours (stop), 4 - mins (stop), 5 - OK*/
/*If mode == Watering cfg, so 0 - quit, 1 - hour (begin), 2 - mins (begin), 3 - hours (stop), 4 - mins (stop), 5 - OK*/
/*If mode == Light cfg, so 0 - quit, 1 - hour (begin), 2 - mins (begin), 3 - hours (stop), 4 - mins (stop), 5 - OK*/
byte cursorPos = 0;
unsigned long lastCursorDerp = 0;
bool cursorState = true;
//0 - Select, 1 - Change
bool cursorMode = 0;

CRGB leds[NUM_OF_LEDS];

//RTOS tasks' ID
#define BUTTONS_CHECK 0
#define LCD_UPDATE 1
#define SLEEP_MODE_CHECK 2
#define CLIMATE_CHECK 3

void printTimeLCD(bool mode){
   if(millis() - lastColonDerp >= 1000){
    lastColonDerp = millis();
    colonState = !colonState;
  }
  if(!mode){
    if(colonState) lcd.print(watch.gettime("H:i"));
    else lcd.print(watch.gettime("H i"));
  }
  else{
    if(colonState) lcd.print(watch.gettime("H:i"));
    else lcd.print(watch.gettime("     "));
  }
}

void cursorDerp(byte id, byte invid){
  if(cursorState){
    lcd.write(id);
  }
  else{
    lcd.write(invid);
  }
}

void updateLCD(){
  if(isModeChanged){
    lcd.clear();
    isModeChanged = false;
  }
  if(currentMode == 0){
    lcd.setCursor(8+3, 0);
    printTimeLCD(false);
  
    lcd.setCursor(0, 1);
    lcd.write((byte) LIGHT_SYMBOL);
    lcd.print(" ");
    if(lightStartHours < 10) lcd.print("0" + String(lightStartHours));
    else lcd.print(lightStartHours);
    
    lcd.print(":");
    
    if(lightStartMins < 10) lcd.print("0" + String(lightStartMins));
    else lcd.print(lightStartMins);
    
    lcd.setCursor(4, 0);
    lcd.print("    ");
    lcd.setCursor(0, 0);
    lcd.write((byte) HUMIDITY_SYMBOL);
    lcd.print(" " + String(analogRead(HUM_SENSOR)));
  
    lcd.setCursor(9, 1);
    lcd.write((byte) WATERING_SYMBOL);
    lcd.print(" ");
    if(wateringStartHours < 10) lcd.print("0" + String(wateringStartHours));
    else lcd.print(wateringStartHours);
    
    lcd.print(":");
    
    if(wateringStartMins < 10) lcd.print("0" + String(wateringStartMins));
    else lcd.print(wateringStartMins);
  }

  else if(currentMode == 1){
    screenForOptionsMenu();
  }

  else if(currentMode == 2){
    screenForTimeCfg();
  }
  else if(currentMode == 3){
    screenForCoolingTimeCfg();
    /*lcd.setCursor(0, 1);
    if(cursorPos != 5){
      lcd.print("OK");
    }*/
  }
  else if(currentMode == 4){
    screenForWateringTimeCfgMenu();
  }

  else if(currentMode == 5){
    screenForLightTimeCfgMenu();
  }
}

void showTime(byte args){
  if(!cursorMode){
    if(cursorState){
      if(args >= 10) lcd.print(args);
      else lcd.print("0"+String(args));
    }
    else lcd.print("  ");
  }

   else {
      if(args >= 10) lcd.print(args);
      else lcd.print("0"+String(args));
   }
}

void screenForOptionsMenu(){
    if(millis() - lastCursorDerp >= 1000){
      cursorState = !cursorState;
      lastCursorDerp = millis();
    }
    
    switch(cursorPos){
      case 0:
        lcd.setCursor(0, 1);
        cursorDerp(B01111111, BACK_SYMBOL_SELECTED);
        
        lcd.setCursor(2, 1);
        lcd.print("Quit ");
      break;
      case 1:
        lcd.setCursor(0, 1);
        cursorDerp(LIGHT_SYMBOL, LIGHT_SYMBOL_SELECTED);

        lcd.setCursor(2, 1);
        lcd.print("Light");
      break;
      case 2:
        lcd.setCursor(0, 1);
        cursorDerp(COOLING_SYMBOL, COOLING_SYMBOL_SELECTED);

        lcd.setCursor(2, 1);
        lcd.print("Wind ");
      break;
      case 3:
        lcd.setCursor(0, 1);
        cursorDerp(WATERING_SYMBOL, WATERING_SYMBOL_SELECTED);

        lcd.setCursor(2, 1);
        lcd.print("Water");
      break;
      case 4:
        lcd.setCursor(0, 1);
        lcd.print(" ");

        lcd.setCursor(11, 0);
        printTimeLCD(true);

        lcd.setCursor(2, 1);
        lcd.print("Clock");
      break;
    }

    lcd.setCursor(11, 0);
    if(cursorPos != 4) printTimeLCD(false);
}

void screenForTimeCfg(){
  if(millis() - lastCursorDerp >= 1000){
      cursorState = !cursorState;
      lastCursorDerp = millis();
    }
    
    switch(cursorPos){
      case 0:
        lcd.setCursor(0, 0);
        cursorDerp(B01111111, BACK_SYMBOL_SELECTED);
      break;
      case 1:
        lcd.setCursor(11, 0);
        showTime(hours_);
      break;
      case 2:
        lcd.setCursor(14, 0);
        showTime(mins);
      break;
      case 3:
        lcd.setCursor(14, 1);
        if(cursorState) lcd.print("OK");
        else lcd.print("  ");
      break;
    }
    
    lcd.setCursor(0, 0);
    if(cursorPos != 0) lcd.write(B01111111);

    lcd.setCursor(11, 0);
    if(cursorPos != 1){
      if(hours_ >= 10) lcd.print(hours_);
      else lcd.print("0"+String(hours_));
    }
    
    lcd.setCursor(13, 0);
    lcd.print(":");

    lcd.setCursor(14, 0);
    if(cursorPos != 2){
      if(mins >= 10) lcd.print(mins);
      else lcd.print("0"+String(mins));
    }
    
    lcd.setCursor(14, 1);
    if(cursorPos != 3)lcd.print("OK");
}

void screenForCoolingTimeCfg(){
    if(millis() - lastCursorDerp >= 1000){
      cursorState = !cursorState;
      lastCursorDerp = millis();
    }

    lcd.setCursor(10, 0);
    lcd.print("ON");

    lcd.setCursor(10, 1);
    lcd.print("OFF");

    lcd.setCursor(5, 0);
    lcd.print(":");

    lcd.setCursor(5, 1);
    lcd.print(":");
    
    switch(cursorPos){
      case 0:
        lcd.setCursor(0, 0);
        cursorDerp(0B01111111, BACK_SYMBOL_SELECTED);
      break;
      case 1:
        lcd.setCursor(3, 0);
        showTime(coolingStartHours);
      break;
      case 2:
        lcd.setCursor(6, 0);
        showTime(coolingStartMins);
      break;
      case 3:
        lcd.setCursor(3, 1);
        showTime(coolingStopHours);
      break;
      case 4:
        lcd.setCursor(6, 1);
        showTime(coolingStopMins);
      break;
      case 5:
        lcd.setCursor(14, 1);
        if(cursorState) lcd.print("OK");
        else lcd.print("  ");
      break;
    }
    
    lcd.setCursor(0, 0);
    if(cursorPos != 0) lcd.write(B01111111);

    lcd.setCursor(3, 0);
    if(cursorPos != 1){
      if(coolingStartHours >= 10) lcd.print(coolingStartHours);
      else lcd.print("0"+String(coolingStartHours));
    }

    lcd.setCursor(6, 0);
    if(cursorPos != 2){
      if(coolingStartMins >= 10) lcd.print(coolingStartMins);
      else lcd.print("0"+String(coolingStartMins));
    }

    lcd.setCursor(3, 1);
    if(cursorPos != 3){
      if(coolingStopHours >= 10) lcd.print(coolingStopHours);
      else lcd.print("0"+String(coolingStopHours)); 
    }

    lcd.setCursor(6, 1);
    if(cursorPos != 4){
      if(coolingStopMins >= 10) lcd.print(coolingStopMins);
      else lcd.print("0"+String(coolingStopMins)); 
    }

    lcd.setCursor(14, 1);
    if(cursorPos != 5) lcd.print("OK");
    
}

void screenForWateringTimeCfgMenu(){
    if(millis() - lastCursorDerp >= 1000){
      cursorState = !cursorState;
      lastCursorDerp = millis();
    }

    lcd.setCursor(10, 0);
    lcd.print("ON");

    lcd.setCursor(10, 1);
    lcd.print("OFF");

    lcd.setCursor(5, 0);
    lcd.print(":");
    
    lcd.setCursor(5, 1);
    lcd.print(":");
    
    switch(cursorPos){
      case 0:
        lcd.setCursor(0, 0);
        cursorDerp(B01111111, BACK_SYMBOL_SELECTED);
      break;
      case 1:
        lcd.setCursor(3, 0);
        showTime(wateringStartHours);
      break;
      case 2:
        lcd.setCursor(6, 0);
        showTime(wateringStartMins);
      break;
      case 3:
        lcd.setCursor(3, 1);
        showTime(wateringStopHours);
      break;
      case 4:
        lcd.setCursor(6, 1);
        showTime(wateringStopMins);
      break;
      case 5:
        lcd.setCursor(14, 1);
        if(cursorState) lcd.print("OK");
        else lcd.print("  ");
      break;
    }

    lcd.setCursor(0, 0);
    if(cursorPos != 0){
      lcd.write(B01111111);
    }

    lcd.setCursor(3, 0);
    if(cursorPos != 1){
      if(wateringStartHours >= 10) lcd.print(wateringStartHours);
      else lcd.print("0"+String(wateringStartHours));
    }

    lcd.setCursor(6, 0);
    if(cursorPos != 2){
      if(wateringStartMins >= 10) lcd.print(wateringStartMins);
      else lcd.print("0"+String(wateringStartMins));
    }

    lcd.setCursor(3, 1);
    if(cursorPos != 3){
      if(wateringStopHours >= 10) lcd.print(wateringStopHours);
      else lcd.print("0"+String(wateringStopHours));
    }

    lcd.setCursor(6, 1);
    if(cursorPos != 4){
      if(wateringStopMins >= 10) lcd.print(wateringStopMins);
      else lcd.print("0"+String(wateringStopMins));
    }

    lcd.setCursor(14, 1);
    if(cursorPos != 5) lcd.print("OK");
    
}

void screenForLightTimeCfgMenu(){
      if(millis() - lastCursorDerp >= 1000){
      cursorState = !cursorState;
      lastCursorDerp = millis();
    }

    lcd.setCursor(10, 0);
    lcd.print("ON");

    lcd.setCursor(10, 1);
    lcd.print("OFF");

    lcd.setCursor(5, 0);
    lcd.print(":");
    
    lcd.setCursor(5, 1);
    lcd.print(":");
    
    switch(cursorPos){
      case 0:
        lcd.setCursor(0, 0);
        cursorDerp(B01111111, BACK_SYMBOL_SELECTED);
      break;
      case 1:
        lcd.setCursor(3, 0);
        showTime(lightStartHours);
      break;
      case 2:
        lcd.setCursor(6, 0);
        showTime(lightStartMins);
      break;
      case 3:
        lcd.setCursor(3, 1);
        showTime(lightStopHours);
      break;
      case 4:
        lcd.setCursor(6, 1);
        showTime(lightStopMins);
      break;
      case 5:
        lcd.setCursor(14, 1);
        if(cursorState) lcd.print("OK");
        else lcd.print("  ");
      break;
    }

    lcd.setCursor(0, 0);
    if(cursorPos != 0){
      lcd.write(B01111111);
    }

    lcd.setCursor(3, 0);
    if(cursorPos != 1){
      if(lightStartHours >= 10) lcd.print(lightStartHours);
      else lcd.print("0"+String(lightStartHours));
    }

    lcd.setCursor(6, 0);
    if(cursorPos != 2){
      if(lightStartMins >= 10) lcd.print(lightStartMins);
      else lcd.print("0"+String(lightStartMins));
    }

    lcd.setCursor(3, 1);
    if(cursorPos != 3){
      if(lightStopHours >= 10) lcd.print(lightStopHours);
      else lcd.print("0"+String(lightStopHours));
    }

    lcd.setCursor(6, 1);
    if(cursorPos != 4){
      if(lightStopMins >= 10) lcd.print(lightStopMins);
      else lcd.print("0"+String(lightStopMins));
    }

    lcd.setCursor(14, 1);
    if(cursorPos != 5) lcd.print("OK");
}


void switchMode(byte mode){
  currentMode = mode;
  isModeChanged = true;
  if(mode == 0) ignoreSleep = true;
  cursorPos = 0;
}

void checkButtons(){
  
  /*if(debugMode.check()){
    isDebugModeStarted = !isDebugModeStarted;
    Serial.println("Debug mode started!");
  }*/

 
  switch(mode.check()){
    case PRESS:
      lastButtonPress = millis();
      ignoreSleep = false;

      if(currentMode == 1){
        switch(cursorPos){
          case 0:
            switchMode(0);
          break;
          case 1:
            switchMode(5);
          break;
          case 2:
            switchMode(3);
          break;
          case 3:
            switchMode(4);
          break;
          case 4:
            switchMode(2);
            
            watch.gettime();
            hours_ = watch.Hours;
            mins = watch.minutes;
          break;
        }
      }

      else if(currentMode == 2){
        switch(cursorPos){
          case 0:
            switchMode(1);
          break;
          case 1:
            cursorMode = !cursorMode;
          break;
          case 2:
            cursorMode = !cursorMode;
          break;
          case 3:
            watch.settime(-1, mins, hours_, -1, -1, -1, -1);
            switchMode(1);
          break;
        }
      }

      else if(currentMode == 3){
        switch(cursorPos){
          case 0:
            coolingStartHours = EEPROM.read(COOLING_START_HOURS);
            coolingStartMins = EEPROM.read(COOLING_START_MINS);
            coolingStopHours = EEPROM.read(COOLING_STOP_HOURS);
            coolingStopMins = EEPROM.read(COOLING_STOP_MINS);
            
            switchMode(1);
          break;
          case 1:
            cursorMode = !cursorMode;
          break;
          case 2:
            cursorMode = !cursorMode;
          break;
          case 3:
            cursorMode = !cursorMode;
          break;
          case 4:
            cursorMode = !cursorMode;
          break;
          case 5:
            EEPROM.update(COOLING_START_HOURS, coolingStartHours);
            EEPROM.update(COOLING_START_MINS, coolingStartMins);
            EEPROM.update(COOLING_STOP_HOURS, coolingStopHours);
            EEPROM.update(COOLING_STOP_MINS, coolingStopMins);
            switchMode(1);
          break;
        }
      }

      else if(currentMode == 4){
        switch(cursorPos){
          case 0:
            coolingStartHours = EEPROM.read(COOLING_START_HOURS);
            coolingStartMins = EEPROM.read(COOLING_START_MINS);
            coolingStopHours = EEPROM.read(COOLING_STOP_HOURS);
            coolingStopMins = EEPROM.read(COOLING_STOP_MINS);
            
            switchMode(1);
          break;
          case 1:
            cursorMode = !cursorMode;
          break;
          case 2:
            cursorMode = !cursorMode;
          break;
          case 3:
            cursorMode = !cursorMode;
          break;
          case 4:
            cursorMode = !cursorMode;
          break;
          case 5:
            EEPROM.update(WATERING_START_HOURS, wateringStartHours);
            EEPROM.update(WATERING_START_MINS, wateringStartMins);
            EEPROM.update(WATERING_STOP_HOURS, wateringStopHours);
            EEPROM.update(WATERING_STOP_MINS, wateringStopMins);
            switchMode(1);
          break;
        }
      }

      else if(currentMode == 5){
        switch(cursorPos){
          case 0:
            coolingStartHours = EEPROM.read(COOLING_START_HOURS);
            coolingStartMins = EEPROM.read(COOLING_START_MINS);
            coolingStopHours = EEPROM.read(COOLING_STOP_HOURS);
            coolingStopMins = EEPROM.read(COOLING_STOP_MINS);
            
            switchMode(1);
          break;
          case 1:
            cursorMode = !cursorMode;
          break;
          case 2:
            cursorMode = !cursorMode;
          break;
          case 3:
            cursorMode = !cursorMode;
          break;
          case 4:
            cursorMode = !cursorMode;
          break;
          case 5:
            EEPROM.update(LIGHT_START_HOURS, lightStartHours);
            EEPROM.update(LIGHT_START_MINS, lightStartMins);
            EEPROM.update(LIGHT_STOP_HOURS, lightStopHours);
            EEPROM.update(LIGHT_STOP_MINS, lightStopMins);
            switchMode(1);
          break;
        }
      }
      /*Serial.println("MODE");                                             //Проверка схемы
      for(int i=0;i<NUM_OF_LEDS;i++){
        if(isLedEnabled) leds[i] = CRGB::White;
        else leds[i] = CRGB::Black;
      }
      isLedEnabled = !isLedEnabled;
      if(isLedEnabled) leds[0] = CRGB::White;
      else leds[0] = CRGB::Black;
      FastLED.show();*/
  }

  switch(inc.check()){
    case PRESS:
      cursorState = true;
      lastButtonPress = millis();
      ignoreSleep = false;

      if(currentMode == 1){
        if(cursorPos < 4) cursorPos++;
        else cursorPos = 0; 
      }

      else if(currentMode == 2){
        checkIncButtonForTimeCfgMenu();
      }

      if(currentMode == 3){
        checkIncButtonForCoolingTimeCfgMenu();
      }

      else if(currentMode == 4){
        checkIncButtonForWateringTimeCfgMenu();
      }

      else if(currentMode == 5){
        checkIncButtonForLightTimeCfgMenu();
      }
      /*isPumpEnabled = !isPumpEnabled;
      digitalWrite(PUMP, isPumpEnabled);
      Serial.println("INC");*/
    break;
  }

  switch(dec.check()){
    case PRESS:
      cursorState = true;
      lastButtonPress = millis();
      ignoreSleep = false;

      if(currentMode == 1){
        if(cursorPos > 0) cursorPos--;
        else cursorPos = 4;
      }

      else if(currentMode == 2){
        checkDecButtonForTimeCfgMenu();
      }

      else if(currentMode == 3){
        checkDecButtonForCoolingTimeCfgMenu();
      }

      else if(currentMode == 4){
        checkDecButtonForWateringTimeCfgMenu();
      }

      else if(currentMode == 5){
        checkDecButtonForLightTimeCfgMenu();
      }
      /*isCoolEnabled = !isCoolEnabled;
      digitalWrite(COOL, isCoolEnabled);
      Serial.println("DEC");*/
    break;
  }
}

void checkClimate(){
  watch.gettime();
  
  if(watch.Hours >= coolingStartHours && watch.Hours <= coolingStopHours && watch.minutes >= coolingStartMins && watch.minutes < coolingStopMins && currentMode != 3){
    digitalWrite(COOL, HIGH);
  }

  if(watch.Hours >= coolingStopHours && watch.minutes >= coolingStopMins && currentMode != 3){
    digitalWrite(COOL, LOW);
  }

  if(watch.Hours >= wateringStartHours && watch.Hours <= wateringStopHours && watch.minutes >= wateringStartMins && watch.minutes < wateringStopMins && currentMode != 4){
    digitalWrite(PUMP, HIGH);
  }

  if(watch.Hours >= wateringStopHours && watch.minutes >= wateringStopMins && currentMode != 4){
    digitalWrite(PUMP, LOW);
  }

  if(watch.Hours >= lightStartHours && watch.Hours <= lightStopHours && watch.minutes >= lightStartMins && watch.minutes < lightStopMins && currentMode != 4){
    for(int i=0;i<NUM_OF_LEDS;i++){
      leds[i] = CRGB::Purple;
    }
    FastLED.show();
  }

  if(watch.Hours >= lightStopHours && watch.minutes >= lightStopMins && currentMode != 5){
    for(int i=0;i<NUM_OF_LEDS;i++){
      leds[i] = CRGB::Black;
    }
    FastLED.show();
  }
}

void runTask(int index){
  switch(index){
    case BUTTONS_CHECK:
      checkButtons();
    break;
    case LCD_UPDATE:
      updateLCD();
    break;
    case SLEEP_MODE_CHECK:
      checkMode();
    break;
    case CLIMATE_CHECK:
      checkClimate();
    break;
  }
}

void checkMode(){
    if(millis() - lastButtonPress >= 30000 && currentMode != 0){
      isModeChanged = true;
      currentMode = 0;
    }

    else if(millis() - lastButtonPress < 30000 && currentMode != 1 && !ignoreSleep && currentMode == 0) {
      isModeChanged = true;
      currentMode = 1;
    }
}

void loadData(){
  coolingStartHours = EEPROM.read(COOLING_START_HOURS);
  
  if(coolingStartHours == 255){
    EEPROM.write(COOLING_START_HOURS, 12);
    EEPROM.write(COOLING_START_MINS, 0);
    EEPROM.write(COOLING_STOP_HOURS, 12);
    EEPROM.write(COOLING_STOP_MINS, 5);

    EEPROM.write(WATERING_START_HOURS, 12);
    EEPROM.write(WATERING_START_MINS, 0);
    EEPROM.write(WATERING_STOP_HOURS, 12);
    EEPROM.write(WATERING_STOP_MINS, 5);

    EEPROM.write(LIGHT_START_HOURS, 7);
    EEPROM.write(LIGHT_START_MINS, 0);
    EEPROM.write(LIGHT_STOP_HOURS, 7);
    EEPROM.write(LIGHT_STOP_MINS, 5);
  }
  coolingStartHours = EEPROM.read(COOLING_START_HOURS);
  coolingStartMins = EEPROM.read(COOLING_START_MINS);
  coolingStopHours = EEPROM.read(COOLING_STOP_HOURS);
  coolingStopMins = EEPROM.read(COOLING_STOP_MINS);

  wateringStartHours = EEPROM.read(WATERING_START_HOURS);
  wateringStartMins = EEPROM.read(WATERING_START_MINS);
  wateringStopHours = EEPROM.read(WATERING_STOP_HOURS);
  wateringStopMins = EEPROM.read(WATERING_STOP_MINS);

  lightStartHours = EEPROM.read(LIGHT_START_HOURS);
  lightStartMins = EEPROM.read(LIGHT_START_MINS);
  lightStopHours = EEPROM.read(LIGHT_STOP_HOURS);
  lightStopMins = EEPROM.read(LIGHT_STOP_MINS);
}

void setup() {
  Serial.begin(9600);
  
  pinMode(INC_BTN, INPUT_PULLUP);
  pinMode(DEC_BTN, INPUT_PULLUP);
  pinMode(MODE_BTN, INPUT_PULLUP);

  pinMode(PUMP, OUTPUT);
  pinMode(COOL, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("LOADING...");

  loadData();

  lcd.clear();

  lcd.createChar(HUMIDITY_SYMBOL, HUMIDITY_PATTERN);
  lcd.createChar(LIGHT_SYMBOL, LIGHT_PATTERN);
  lcd.createChar(LIGHT_SYMBOL_SELECTED, LIGHT_PATTERN_SELECTED);
  lcd.createChar(COOLING_SYMBOL, COOLING_PATTERN);
  lcd.createChar(COOLING_SYMBOL_SELECTED, COOLING_PATTERN_SELECTED);
  lcd.createChar(WATERING_SYMBOL, WATERING_PATTERN);
  lcd.createChar(WATERING_SYMBOL_SELECTED, WATERING_PATTERN_SELECTED);
  lcd.createChar(BACK_SYMBOL_SELECTED, BACK_PATTERN_SELECTED);

  watch.begin();

  //watch.settime(0, 39, 16, 14, 8, 20, 5);               //сек., мин., час., число, месяц, год, день недели.
  
  FastLED.addLeds<NEOPIXEL, LED>(leds, NUM_OF_LEDS);

  add_task(100, BUTTONS_CHECK);
  add_task(500, LCD_UPDATE);
  add_task(1000, SLEEP_MODE_CHECK);
  add_task(1000, CLIMATE_CHECK);

  /*for(int i=0;i<NUM_OF_LEDS;i++){
    leds[i] = CRGB(255,100,100);
  }

  FastLED.show();*/
}

void loop() {
  checkTasks();
}

void checkTasks(){
  for(int i=0;i<MAX_TASKS;i++){
    if(task_time[i] != 0 && millis() - last_task_time[i] >= task_time[i]){
      last_task_time[i] = millis();
      runTask(i);
    }
  }
}

void checkIncButtonForTimeCfgMenu(){
  if(!cursorMode){
    if(cursorPos < 3) cursorPos++;
    else cursorPos = 0;
  }
  else{
    if(cursorPos == 1) if(hours_< 23) hours_++;
    if(cursorPos == 2) if(mins < 59) mins++;
  }
}

void checkDecButtonForTimeCfgMenu(){
  if(!cursorMode){
    if(cursorPos > 0) cursorPos--;
    else cursorPos = 3;
  }
  else{
    if(cursorPos == 1) if(hours_ > 0) hours_--;
    if(cursorPos == 2) if(mins > 0) mins--;
  }
}

void checkDecButtonForCoolingTimeCfgMenu(){
  if(!cursorMode){
    if(cursorPos > 0) cursorPos--;
    else cursorPos = 5;
  }
  else {
    if(cursorPos == 1){
      if(coolingStartHours > 0) coolingStartHours--;
      else coolingStartHours = 23;
    }
    if(cursorPos == 2){
      if(coolingStartMins > 0) coolingStartMins--;
      else coolingStartMins = 59;
    }
    if(cursorPos == 3){
      if(coolingStopHours > 0) coolingStopHours--;
      else coolingStopHours = 23;
    }
    if(cursorPos == 4){
      if(coolingStopMins > 0) coolingStopMins--;
      else coolingStopMins = 59;
    }
  }
}

void checkIncButtonForCoolingTimeCfgMenu(){
  if(!cursorMode){
    if(cursorPos < 5) cursorPos++;
    else cursorPos = 0;
  }
  else {
    if(cursorPos == 1){
      if(coolingStartHours < 23) coolingStartHours++;
      else coolingStartHours = 0;
    }
    if(cursorPos == 2){
      if(coolingStartMins < 59) coolingStartMins++;
      else coolingStartMins = 0;
    }
    if(cursorPos == 3){
      if(coolingStopHours < 23) coolingStopHours++;
      else coolingStopHours = 0;
    }
    if(cursorPos == 4){
      if(coolingStopMins < 59) coolingStopMins++;
      else coolingStopMins = 0;
    }
  }
}

void checkIncButtonForWateringTimeCfgMenu(){
  if(!cursorMode){
    if(cursorPos < 5) cursorPos++;
    else cursorPos = 0;
  }
  
  else {
    if(cursorPos == 1){
      if(wateringStartHours < 23) wateringStartHours++;
      else wateringStartHours = 0;
    }
    if(cursorPos == 2){
      if(wateringStartMins < 59) wateringStartMins++;
      else wateringStartMins = 0;
    }
    if(cursorPos == 3){
      if(wateringStopHours < 23) wateringStopHours++;
      else wateringStopHours = 0;
    }
    if(cursorPos == 4){
      if(wateringStopMins < 59) wateringStopMins++;
      else wateringStopMins = 0;
    }
  }
}

void checkDecButtonForWateringTimeCfgMenu(){
  if(!cursorMode){
    if(cursorPos > 0) cursorPos--;
    else cursorPos = 5;
  }
  else {
    if(cursorPos == 1){
      if(wateringStartHours > 0) wateringStartHours--;
      else wateringStartHours = 23;
    }
    if(cursorPos == 2){
      if(wateringStartMins > 0) wateringStartMins--;
      else wateringStartMins = 59;
    }
    if(cursorPos == 3){
      if(wateringStopHours > 0) wateringStopHours--;
      else wateringStopHours = 23;
    }
    if(cursorPos == 4){
      if(wateringStopMins > 0) wateringStopMins--;
      else wateringStopMins = 59;
    }
  }
}

void checkIncButtonForLightTimeCfgMenu(){
  if(!cursorMode){
    if(cursorPos < 5) cursorPos++;
    else cursorPos = 0;
  }
  
  else {
    if(cursorPos == 1){
      if(lightStartHours < 23) lightStartHours++;
      else lightStartHours = 0;
    }
    if(cursorPos == 2){
      if(lightStartMins < 59) lightStartMins++;
      else lightStartMins = 0;
    }
    if(cursorPos == 3){
      if(lightStopHours < 23) lightStopHours++;
      else lightStopHours = 0;
    }
    if(cursorPos == 4){
      if(lightStopMins < 59) lightStopMins++;
      else lightStopMins = 0;
    }
  }
}

void checkDecButtonForLightTimeCfgMenu(){
  if(!cursorMode){
    if(cursorPos > 0) cursorPos--;
    else cursorPos = 5;
  }
  else {
    if(cursorPos == 1){
      if(lightStartHours > 0) lightStartHours--;
      else lightStartHours = 23;
    }
    if(cursorPos == 2){
      if(lightStartMins > 0) lightStartMins--;
      else lightStartMins = 59;
    }
    if(cursorPos == 3){
      if(lightStopHours > 0) lightStopHours--;
      else lightStopHours = 23;
    }
    if(cursorPos == 4){
      if(lightStopMins > 0) lightStopMins--;
      else lightStopMins = 59;
    }
  }
}
