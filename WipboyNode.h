#ifndef WipboyNode_h
#define WipboyNode_h

#include "Arduino.h"


class WipboyNode
{
  public:
    WipboyNode();
    byte pos;
    byte node;
    String ssid;
    long rssi;
  private:
};

#endif


