#ifndef Icon_h
#define Icon_h

#include "Arduino.h"


class Icon
{
  public:
    Icon(byte _x, byte _y);
    byte x;
    byte y;
    byte w = 32;
    byte h = 40;
    byte sbw = 45;
    byte sbh = 45;
    byte offsetX=6;
    byte offsetY=2;
    byte title;
    byte Mode = 102;
    unsigned char *img;
  private:
};

#endif


