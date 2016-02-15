#ifndef Quest_h
#define Quest_h

#include "Arduino.h"


class Quest
{
  public:
    Quest();
    bool visible = true;
    byte stage = 0;
    byte maxStage = 1;
    int stageTags[];
    int descriptions[];
    int title;
    int xp=1000;
  private:
};

#endif

