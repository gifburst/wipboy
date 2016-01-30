#ifndef Slider_h
#define Slider_h

#include "Arduino.h"

class Slider
{
  public:
    Slider(int pin, int range, int jitter);
    int hasChanged();
    int getPos();
    int getRawPos();
    void setRange(int range);
  private:
    int _pin;
    int _sliderPos;
    int _range;
    int _jitter;
    int _lastReading;
};

#endif


