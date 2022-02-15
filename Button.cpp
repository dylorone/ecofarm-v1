#include "Button.h"
#include<Arduino.h>

Button::Button(int pin, unsigned int holdTimeNeeded, bool mode){
  this->__isPressed = true;
  this->__lastToggle = 0;
  this->__rattlingMS = 20;
  this->__lastNonRattlingToggle = 0;
  this->__pin = pin;
  this->__holdTimeNeeded = holdTimeNeeded;
  this->__mode = mode;
}

bool Button::isPressed(){
  return Button::__isPressed;
}

unsigned long Button::getLastToggle(){
  return Button::__lastToggle;
}

unsigned int Button::getHoldTimeNeeded(){
  return this->__holdTimeNeeded;
}

int Button::getPin(){
  return this->__pin;
}

bool Button::toggle(){
    //Функция проверки дребезга кнопки
    if(millis() - this->__lastToggle <= __rattlingMS) {
      this->__lastToggle = millis();
      return false;
    }
    
    this->__isPressed = !__isPressed;
    this->__lastToggle = millis();
    this->__lastNonRattlingToggle = millis();
    return true;
}

unsigned long Button::getHoldTime(){
    //Возвращает время удержания кнопки
    unsigned long ret = 0;
    if(!__isPressed) ret = millis() - this->__lastNonRattlingToggle;
    return ret;
}

int Button::check(){
  if(!this->__mode){
     if(digitalRead(this->__pin) == this->__isPressed && this->getHoldTime() <= this->__holdTimeNeeded && this->__isPressed) return NO_CHANGE;
     else if(digitalRead(this->__pin) == this->__isPressed && this->getHoldTime() <= __holdTimeNeeded && !this->__isPressed) return HOLD;
     else if(digitalRead(this->__pin) == this->__isPressed && this->getHoldTime() > __holdTimeNeeded) return LONG_PRESS; 
     else if(digitalRead(this->__pin) != this->__isPressed && digitalRead(this->__pin) == !LOW && this->toggle()) return NO_PRESS;
     else if(digitalRead(this->__pin) != this->__isPressed && digitalRead(this->__pin) == !HIGH && this->toggle()) return PRESS;
  }

  if(digitalRead(this->__pin) != this->__isPressed && this->__isPressed == false){
    if(this->getHoldTime() <= this->__holdTimeNeeded && this->toggle()) return PRESS;
    else if(this->getHoldTime() > this->__holdTimeNeeded && this->toggle()) return HOLD;
  }

  else if(digitalRead(this->__pin) != this->__isPressed && this->__isPressed == true && this->toggle()) return NO_PRESS;
}

/*ButtonsCombination::ButtonsCombination(Button btn1, Button btn2){
  this->button1 = btn1;
  this->button2 = btn2;
}

bool ButtonsCombination::check(){
  /*if(digitalRead(button1.getPin()) == !HIGH && !button1.isPressed() && button1.toggle()) this->__isBtn1Pressed = true;
  else if(digitalRead(button1.getPin()) == !LOW && button1.isPressed() && button1.toggle()) this->__isBtn1Pressed = false;

  if(digitalRead(button2.getPin()) == !HIGH && digitalRead(button2.getPin()) == !button2.isPressed() && button2.toggle() && button1.isPressed()) return true;
  return false;
}

Button ButtonsCombination::getButton1(){
  return this->button1;
}

Button ButtonsCombination::getButton2(){
  return this->button2;
}*/
