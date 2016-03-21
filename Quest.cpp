#include <Arduino.h>
#include "Quest.h"

Quest::Quest()
{
}

bool Quest::nextStage()
{
  stage ++;
  visible=true;
  if (stage >= maxStage)
  {
    active=false;
    return true;
  }
  return false;
}



byte Quest::getDescIndex()
{
  return descriptions[stage];
}



byte Quest::getStageKeyIndex()
{
  return stageKeys[stage];
}


