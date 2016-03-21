#ifndef Quest_h
#define Quest_h

#include "Arduino.h"

class Quest
{
  public:
    Quest();
    bool visible = true;
    bool active = true;
    byte id = 0;
    byte stage = 0;
    byte maxStage = 1;
    byte stageKeys[5] = {0,0,0,0,0};
    byte descriptions[5] = {0,0,0,0,0};
    int title = 1;
    //int xp=1000;
    bool nextStage();
    byte getDescIndex();
    byte getStageKeyIndex();
  private:
};


#endif


/*
 * User scans in a tag
 *  tag is then compared to the list of active quests
 *  if the tag matches a quest
 *    if stage == maxStage:
 *      stage complete. 
 *      print message
 *      add xp to self
 *    else
 *      stage += 1
 *      print message
 *      
 * 
 * 
 * 
 * 
 * 
 * 
 */
