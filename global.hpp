/*****************************************************************************
== Physics Anonymous CC-BY 2017 ==
******************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include "config.hpp"

/*** Global definitions *****************************************************/
enum ERROR_T{
  NONE=0,
  UNKNOWN,
  CANCEL,
  SOFTWARE
};

#define ENDWARD         (SLIDER_MAX_POSITION/abs(SLIDER_MAX_POSITION))
#define HOMEWARD        (-1 * ENDWARD)
#define BACK_OFF_STEP   abs((SLIDER_MAX_POSITION)/1000)
/****************************************************************************/

/*** Debug print functions **************************************************/
template<class M, class T>
void DEBUG(const M msg, const T value){
  #ifdef DEBUG_OUTPUT
    Serial.print(msg);
    Serial.println(value);
  #endif
}

template<class T>
void DEBUG(const T msg){
  #ifdef DEBUG_OUTPUT
    Serial.println(msg);
  #endif
}
/****************************************************************************/

#endif
