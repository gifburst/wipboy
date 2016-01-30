#ifndef Button_h
#define Button_h

#include "Arduino.h"

class Button
{
  public:
    Button(int pin, bool repeating);
    int isPressed();
    bool isRepeating();
    void reset();
    unsigned long getRebounceTime();
  private:
    int _pin;
    int _buttonState;
    int _lastState;
    bool _repeats;
    int _checked;
    unsigned long _lastDebounceTime;
    int _debounceDelay;
    unsigned long _lastRebounceTime;
    int _rebounceDelay;
};

#endif
