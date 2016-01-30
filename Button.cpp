#include <Arduino.h>
#include "Button.h"

Button::Button(int pin, bool repeating)
{
  pinMode(pin, INPUT_PULLUP);
  _pin = pin;
  _lastState = LOW;
  _lastDebounceTime = 0;
  _lastRebounceTime = 0;
  _rebounceDelay = 400;
  _debounceDelay = 50;
  _repeats = repeating;
  _checked = 0;
}

int Button::isPressed() 
{
  int reading = digitalRead(_pin);
  if (reading)
    reading=0;
  else
    reading=1;
  
  if (reading != _lastState)  
  {
    _lastDebounceTime = millis();
  }
  _lastState = reading;

  if ((millis() - _lastDebounceTime) > _debounceDelay)  
  {  
    if (reading != _buttonState)  {
      _buttonState = reading;
    }
  }

  
  
  if (_buttonState == 1)	
  {
    //Serial.print("btn: ");
    //Serial.println(_checked);
    if (_checked != 1)  
	  {
      _checked = 1;
      _lastRebounceTime = millis();
      return 1;
    }
  
    else  
	  {
      return -1;
    }
  }
  
  _checked = 0;  
  return 0;
}

bool Button::isRepeating()	{
  return _repeats;
}

void Button::reset()	{
  if (_checked == 1)  {
    if ((millis() - _lastRebounceTime) > _rebounceDelay)  {
      _checked = 0;
    } 
  }
}

unsigned long Button::getRebounceTime()	{
  return _lastRebounceTime;
}
