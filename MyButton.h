#ifndef MY_BUTTON.H
#define MY_BUTTON.H

#include"Arduino.h"

#define DELAY_DEBOUNCE 10

class MyButton
{

  private:
    byte thePin;
    uint8_t currentState;
    uint8_t lastState;
    unsigned long debounceTime;
    unsigned delayRepeat;
    bool toPress = false;

  public:
    MyButton();
    void setup(byte pin);
    void loop();
    bool isPressed;
    bool isChange;
    void SetRepeat(unsigned long tempo);
    bool ToPress();
};

#endif
