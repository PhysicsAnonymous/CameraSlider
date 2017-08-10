/*****************************************************************************
== Physics Anonymous CC-BY 2017 ==
******************************************************************************/

#include <Arduino.h>
#include <limits.h>
#include <Bounce2.h>
#include "global.hpp"
#include "SliderFSM.hpp"
#include "States.hpp"

extern ERROR_T ERR;

/*** SliderFSM **************************************************************/
void SliderFSM::run_loop(){
  //If we have a request to change state, handle that; otherwise run
  if(STATES::NO_STATE == m_target_state){m_state->run_loop();}
  else{update_state();}
}

void SliderFSM::go_button(){m_state->go_button();}

void SliderFSM::home_stop(){m_state->home_stop();}

void SliderFSM::end_stop(){m_state->end_stop();}

void SliderFSM::change_state(STATES new_state){
  if (STATES::NO_STATE != m_target_state) {
    /*TODO: complain about rapid state changes - should never happen*/
   }
  //Don't update to the same state!
  else if (m_target_state != m_state->get_state_as_enum()) {
    m_target_state = new_state;
  }
  else {
    ERR=ERROR_T::SOFTWARE;
    m_target_state = STATES::ERROR;
  }
}

void SliderFSM::update_state(){
  //TODO: Technically, we should probably block interrupts here, because
  //while we are in this function we are in an undefined state.  But we aren't
  //using interrupts.

  //if the transition is not allowed, it's a software error
  if (! m_state->transition_allowed(m_target_state)) {
    ERR=ERROR_T::SOFTWARE;
    m_target_state=STATES::ERROR;
  }
  m_state->exit_state(); //exit our current state before entering new one
  delete m_state;
  switch (m_target_state) {
    case FIRST_HOME:
      DEBUG(F("State: First Home"));
      m_state = new StateFirstHome(this);
      break;
    case ADJUST:
      DEBUG(F("State: Adjust"));
      m_state = new StateAdjust(this);
      break;
    case FIRST_END_MOVE:
      DEBUG(F("State: FirstEndMove"));
      m_state = new StateFirstEndMove(this);
      break;
    case SECOND_HOME:
      DEBUG(F("State: SecondHome"));
      m_state = new StateSecondHome(this);
      break;
    case WAIT:
      DEBUG(F("State: Wait"));
      m_state = new StateWait(this);
      break;
    case EXECUTE:
      DEBUG(F("State: Execute"));
      m_state = new StateExecute(this);
      break;
    case ERROR:
      DEBUG(F("State: Error"));
      m_state = new StateError(this);
      break;
    default:
      DEBUG(F("Invalid State selected"),m_target_state);
      ERR=ERROR_T::SOFTWARE;
      m_state = new StateError(this);
  }
  m_target_state=STATES::NO_STATE;
  m_state->enter_state();
}

SliderFSM::SliderFSM() {
  m_state = new StateWait(this);
  m_state->enter_state();
  m_target_state = STATES::NO_STATE;
}
/****************************************************************************/
