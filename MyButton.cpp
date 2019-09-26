#include "MyButton.h"

MyButton::MyButton()
{
}

void MyButton::setup(byte pin)
{
  thePin = pin;
  currentState = LOW;
  lastState = LOW;
  debounceTime = 0;
  pinMode(thePin, INPUT_PULLUP);
  isPressed = false;
  isChange = false;
  delayRepeat = 0;
}

void MyButton::loop()
{
  uint8_t reading = digitalRead(thePin);
  if (reading != lastState)
    debounceTime = millis();
  if ((millis() - debounceTime) > DELAY_DEBOUNCE)
  {
    if (reading != currentState)
    {
      currentState = reading;
      isChange = true;
      if (currentState == HIGH) {
        isPressed = false;
      } else {
        isPressed = true;
        toPress = true;
      }
      
    }
    else{
      isChange = false;
      if( delayRepeat>0)
      {
        if ( (millis()-debounceTime)%delayRepeat==0)
        {
          isChange= true;
        }
      }
        
      }
  }
  lastState = reading;
}

 void MyButton::SetRepeat(unsigned long tempo)
 {
    delayRepeat = tempo;
 }

 bool MyButton::ToPress() {
  bool temp = toPress;
  toPress = false;
  return temp;
 }
