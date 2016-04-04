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
    int getRange();
    //int getMinRange();
    //int getMaxRange();
    void setRange(int range);
    //void setMinRange(int range);
    //void setMaxRange(int range);
  private:
    int _pin;
    int _sliderPos;
    int _range;
    int _minRange;
    int _maxRange;
    int _jitter;
    int _lastReading;
};

#endif


