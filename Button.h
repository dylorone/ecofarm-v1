#ifndef Button_h
#define Button_h

typedef enum {
  NO_CHANGE = 0, 
  NO_PRESS = 1,
  PRESS = 2,
  LONG_PRESS = 3,
  HOLD = 4
};

class Button {
  private:
  bool __isPressed;
  unsigned long __lastToggle;
  int __rattlingMS;
  unsigned long __lastNonRattlingToggle;
  unsigned int __holdTimeNeeded;
  int __pin;
  bool __mode;
  
  public:
  Button(int pin, unsigned int holdTimeNeeded, bool mode);
  bool toggle();
  unsigned long getHoldTime();
  bool isPressed();
  unsigned long getLastToggle();
  int check();
  unsigned int getHoldTimeNeeded();
  int getPin();
};

/*class ButtonsCombination {
  private:
  Button &button1;
  Button &button2;
  //bool __isBtn1Pressed;

  public:
  ButtonsCombination(Button btn1, Button btn2);
  bool check();
  Button getButton1();
  Button getButton2();
};*/

#endif
