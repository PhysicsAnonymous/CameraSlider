
#include "mike.hpp"

//#include <avr/io.h>
//#include <util/delay.h>
//TODO: remove this unneded include
#include <cstdio>
void TBD(const char * str){printf("%s\n\0",str);};
//void TBD(const char * str){};


/*** Global data ************************************************************/
//AccelStepper SLIDER_MOTOR(AccelStepper::DRIVER, SLIDER_STEP_PIN, SLIDER_DIR_PIN);
//AccelStepper CAMERA_MOTOR(AccelStepper::DRIVER, CAMERA_STEP_PIN, CAMERA_DIR_PIN);
SliderFSM state_machine;
long CAMERA_TARGET_START=0;
long CAMERA_TARGET_STOP=0;
long SLIDE_TARGET_STOP=SLIDER_MAX_POSITION;
ERROR_T ERR=ERROR_T::NONE;
char STATIC_MEMORY_ALLOCATION[sizeof(StateFirstAdjust)]; //Largest "state"
/****************************************************************************/

/*** Utility functions ******************************************************/
SWITCH_STATE read_3way(){
  TBD("Read switch; for now we just return something");
  return SWITCH_STATE::PROGRAM_MODE;
}

long read_camera_pot(){
  TBD("long pos_raw = analogRead(CAMERA_POT_PIN);");
  //Results are between 0 and 1023, shift it between -528 527, and scale it
  //to our camera min/max position:
  TBD("long target_pos = (pos_raw-512) * (CAMERA_MAX_POSITION/512)");
  TBD("return target_pos;");
  return 0;
}

/****************************************************************************/

/*** AbstractState **********************************************************/
AbstractState::~AbstractState(){}

void AbstractState::run_loop(){}

void AbstractState::go_button(){}

void AbstractState::home_stop(){}

void AbstractState::end_stop(){}

bool AbstractState::transition_allowed(STATES new_state){return true;}

void AbstractState::exit_state(){}

void AbstractState::enter_state(){}

STATES AbstractState::get_state_as_enum(){}


void* AbstractState::operator new(size_t sz){
  return (void*)STATIC_MEMORY_ALLOCATION;
}

void AbstractState::operator delete(void* p){
  //AbstractState *abptr = (AbstractState*)p;
  //abptr->~AbstractState();
}
/****************************************************************************/


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
}

void SliderFSM::update_state(){
  //TODO: Technically, we should probably block interrupts here, because
  //while we are in this function we are in an undefined state.

  //if the transition is not allowed, it's a software error
  if (! m_state->transition_allowed(m_target_state)) {
    ERR=ERROR_T::SOFTWARE;
    m_target_state=STATES::ERROR;
  }
  m_state->exit_state();
  delete m_state;
  switch (m_target_state) {
    case IDLE: 
      m_state = new  StateIdle(this);
      break;
    case FIRST_HOME:
      m_state = new StateFirstHome(this);
      break;
    case FIRST_ADJUST:
      m_state = new StateFirstAdjust(this);
      break;
    case FIRST_END_MOVE:
      m_state = new StateFirstEndMove(this);
      break;
    case SECOND_ADJUST:
      m_state = new StateSecondAdjust(this);
      break;
    case SECOND_HOME:
      m_state = new StateSecondHome(this);
      break;
    case WAIT:
      m_state = new StateWait(this);
      break;
    case EXECUTE:
      m_state = new StateExecute(this);
      break;
    case ERROR:
      m_state = new StateError(this);
      break;
    default:
      ERR=ERROR_T::SOFTWARE;
      m_state = new StateError(this);
  }
  m_target_state=STATES::NO_STATE;
  m_state->enter_state();
}

SliderFSM::SliderFSM() {
  m_state = new StateIdle(this);
  m_state->enter_state();
  m_target_state = STATES::NO_STATE;
}
/****************************************************************************/

/*** Idle state *************************************************************/
void StateIdle::run_loop(){
  TBD("sleep in between idle loop runs");
}

//Transition to programming state, if the 3-way switch is set to program.
void StateIdle::go_button(){
  if (SWITCH_STATE::PROGRAM_MODE == read_3way()){
    m_machine->change_state(STATES::FIRST_HOME);
  }
  //else, wrong mode and ignore spurrious button push
}

//Only allow transitions to the First Home state from here
bool StateIdle::transition_allowed(STATES new_state){
  return STATES::FIRST_HOME == new_state;
};

void StateIdle::enter_state(){
  TBD("SLIDER_MOTOR.disableOutputs();");
  TBD("CAMERA_MOTOR.disableOutputs();");
};

STATES StateIdle::get_state_as_enum(){return STATES::IDLE;};

StateIdle::StateIdle(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** First home state *******************************************************/
void StateFirstHome::run_loop(){
  TBD("SLIDER_MOTOR.run();");
}

bool StateFirstHome::transition_allowed(STATES new_state){
  return new_state == STATES::FIRST_ADJUST;
};

void StateFirstHome::enter_state(){
  TBD("SLIDER_MOTOR.enableOutputs();");
  TBD("CAMERA_MOTOR.enableOutputs();");
  TBD("CAMERA_MOTOR.setCurrentPosition(0);");
  TBD("SLIDER_MOTOR.move(-2147483648);"); //maximum negative distance
}

void StateFirstHome::home_stop(){
  TBD("SLIDER_MOTOR.setCurrentPosition(0);");
  TBD("SLIDER_MOTOR.runTo(0);"); //possibly unecessary, blocks until done 
                                 //(but should already be done when called)
  m_machine->change_state(STATES::FIRST_ADJUST);
}

void StateFirstHome::end_stop(){
  ERR=ERROR_T::SOFTWARE;
  m_machine->change_state(STATES::ERROR);
}

void StateFirstHome::go_button(){
  //The "go" button is now "stop."  Like pressing the "start menu" to shut down.
  ERR=ERROR_T::CANCEL;
  m_machine->change_state(STATES::ERROR);
}

STATES StateFirstHome::get_state_as_enum(){return STATES::FIRST_HOME;};

StateFirstHome::StateFirstHome(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** First adjust state *******************************************************/
void StateFirstAdjust::run_loop(){
  if (0 >= m_update_counter){
    long target_pos=read_camera_pot();
    TBD("CAMERA_MOTOR.moveTo(target_pos);");
    m_update_counter = TICKS_PER_POT_READ;
  }
  TBD("CAMERA_MOTOR.run();");
}

bool StateFirstAdjust::transition_allowed(STATES new_state){
  return new_state == STATES::FIRST_END_MOVE;
};

void StateFirstAdjust::enter_state(){
  TBD("CAMERA_MOTOR.setCurrentPosition(0);");
  m_update_counter=0;
}

void StateFirstAdjust::go_button(){
  TBD("CAMERA_TARGET_START = CAMERA_MOTOR.currentPosition();");
  m_machine->change_state(STATES::FIRST_END_MOVE);
}

STATES StateFirstAdjust::get_state_as_enum(){return STATES::FIRST_ADJUST;};

StateFirstAdjust::StateFirstAdjust(SliderFSM* machine) : 
                                           AbstractState(machine),
                                           m_update_counter(0) 
{};
/****************************************************************************/

/*** First end move state ***************************************************/
void StateFirstEndMove::run_loop(){
  TBD("SLIDER_MOTOR.run();");
}

bool StateFirstEndMove::transition_allowed(STATES new_state){
  return new_state == STATES::SECOND_ADJUST;
};

void StateFirstEndMove::enter_state(){
  TBD("SLIDER_MOTOR.moveTo(SLIDE_TARGET_STOP);");
}

void StateFirstEndMove::home_stop(){
  ERR=ERROR_T::SOFTWARE;
  m_machine->change_state(STATES::ERROR);
}

void StateFirstEndMove::end_stop(){
  //Oops, we over-shot.  No problem, just stop quickly and update our target:
  TBD("SLIDE_TARGET_STOP = SLIDER_MOTOR.currentPosition();");
  TBD("SLIDER_MOTOR.runTo(SLIDE_TARGET_STOP);");
  m_machine->change_state(STATES::SECOND_ADJUST);
}

void StateFirstEndMove::go_button(){
  //The "go" button is now "stop."  Like pressing the "start menu" to shut down.
  ERR=ERROR_T::CANCEL;
  m_machine->change_state(STATES::ERROR);
}

STATES StateFirstEndMove::get_state_as_enum(){return STATES::FIRST_END_MOVE;};

StateFirstEndMove::StateFirstEndMove(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** Second adjust state ****************************************************/
void StateSecondAdjust::run_loop(){
  if (0 >= m_update_counter){
    long target_pos=read_camera_pot();
    TBD("CAMERA_MOTOR.moveTo(target_pos);");
    m_update_counter = TICKS_PER_POT_READ;
  }
  TBD("CAMERA_MOTOR.run();");
}

bool StateSecondAdjust::transition_allowed(STATES new_state){
  return new_state == STATES::SECOND_HOME;
};

void StateSecondAdjust::enter_state(){
  m_update_counter=0;
}

void StateSecondAdjust::go_button(){
  TBD("CAMERA_TARGET_END = CAMERA_MOTOR.currentPosition();");
  m_machine->change_state(STATES::SECOND_HOME);
}

STATES StateSecondAdjust::get_state_as_enum(){return STATES::SECOND_ADJUST;};

StateSecondAdjust::StateSecondAdjust(SliderFSM* machine) : 
                                           AbstractState(machine),
                                           m_update_counter(0) 
{};
/****************************************************************************/

/*** Second home state ******************************************************/
void StateSecondHome::run_loop(){
  TBD("SLIDER_MOTOR.run();");
}

bool StateSecondHome::transition_allowed(STATES new_state){
  return new_state == STATES::WAIT;
};

void StateSecondHome::enter_state(){
  TBD("SLIDER_MOTOR.moveTo(0);");
}

void StateSecondHome::home_stop(){
  //Make sure we stopped close enough to zero:
  TBD("if (SLIDER_MOTOR.currentPosition() >= MAX_REHOME_DIFFERENCE){");
  TBD("  ERR=ERROR_T::UNKNOWN;");
  TBD("  m_machine->change_state(STATES::ERROR); }");
  TBD("else {");
    m_machine->change_state(STATES::WAIT);
  //}
}

void StateSecondHome::end_stop(){
  ERR=ERROR_T::SOFTWARE;
  m_machine->change_state(STATES::ERROR);
}

void StateSecondHome::go_button(){
  //The "go" button is now "stop."  Like pressing the "start menu" to shut down.
  ERR=ERROR_T::CANCEL;
  m_machine->change_state(STATES::ERROR);
}

STATES StateSecondHome::get_state_as_enum(){return STATES::SECOND_HOME;};

StateSecondHome::StateSecondHome(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** Wait state *************************************************************/
void StateWait::run_loop(){
  TBD("sleep in between wait loop runs");
}

void StateWait::go_button(){
  //TODO: Change this back to PROGRAM_MODE!
  if (SWITCH_STATE::INVALID != read_3way()){
    m_machine->change_state(STATES::EXECUTE);
  }
  //If we haven't chosen video or lapse mode, ignore the execute and wait.
}

bool StateWait::transition_allowed(STATES new_state){
  return STATES::EXECUTE == new_state;
};

STATES StateWait::get_state_as_enum(){return STATES::EXECUTE;};

StateWait::StateWait(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** Execute state **********************************************************/
void StateExecute::run_loop(){
  TBD("SLIDER_MOTOR.run();");
  TBD("CAMERA_MOTOR.run();");
}

void StateExecute::go_button(){
  //Treat "go" as emergency stop.
  ERR=ERROR_T::CANCEL;
  m_machine->change_state(STATES::ERROR);
}

void StateExecute::end_stop(){
  //All done, even if we didn't quite hit our targets
  m_machine->change_state(STATES::IDLE);
}

void StateExecute::enter_state(){
  TBD("do speed calculations here!");
}

bool StateExecute::transition_allowed(STATES new_state){
  return STATES::IDLE == new_state;
};

STATES StateExecute::get_state_as_enum(){return STATES::EXECUTE;};

StateExecute::StateExecute(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** Error state **********************************************************/
void StateError::run_loop(){
  #ifdef ERROR_LED_PIN
  switch (ERROR) {
    case NONE:
      //We shouldn't have an error if it's none.  Which makes this... a
      //software error?  Follow through with software error part:
    case SOFTWARE:
      for (int i = 0; i<3; i++) {
        TBD("turn on LED pin");
        TBD("sleep 0.5 seconds");
        TBD("turn off LED pin");
        TBD("sleep 0.5 seconds");
      }
      break;
    case CANCEL:
      for (int i = 0; i<2; i++) {
        TBD("turn on LED pin");
        TBD("sleep 1 seconds");
        TBD("turn off LED pin");
        TBD("sleep 0.5 seconds");
      }
      break;
    case UNKNOWN:
      TBD("turn on LED pin");
      TBD("sleep 1.5 seconds")
      TBD("turn off LED pin");
  }
  #endif
  //if we haven't defined an error pin, just leave this mode.
  ERR=ERROR_T::NONE;
  m_machine->change_state(STATES::IDLE);
}

void StateError::enter_state(){
  TBD("SLIDER_MOTOR.disableOutputs();");
  TBD("CAMERA_MOTOR.disableOutputs();");
}

bool StateError::transition_allowed(STATES new_state){
  return STATES::IDLE == new_state;
};

STATES StateError::get_state_as_enum(){return STATES::ERROR;};

StateError::StateError(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/


/*** Main *******************************************************************/
void loop() {
  state_machine.run_loop();
}

void setup() {
TBD("SLIDER_MOTOR.setMaxSpeed(SLIDER_MAX_SPEED);");
TBD("SLIDER_MOTOR.setAcceleration(SLIDER_MAX_ACCEL);");
TBD("CAMERA_MOTOR.setMaxSpeed(CAMERA_MAX_SPEED);");
TBD("CAMERA_MOTOR.setAcceleration(CAMERA_MAX_ACCEL);");
TBD("Set up go_button, end_stop, and home_stop as interrupt pins?");
}

/*
int main(void){
  setup();
  while(true){
    loop();
  }
  return 0;
}*/


int main(int argc, char** argv) {
  TBD("Hopefully we've uncommented the motor definitions at the start");
  setup();
  loop();
  loop(); //Idle
  state_machine.go_button(); //Switch to homing
  loop();
  loop();
  loop();
  state_machine.home_stop(); //Switch to adjusting
  loop();
  loop();
  loop();
  state_machine.go_button(); //Switch to move to end
  loop();
  loop();
  loop();
  state_machine.end_stop(); //switch to adjust 2
  loop();
  loop();
  loop();
  state_machine.go_button(); //switch to home2
  loop();
  loop();
  loop();
  state_machine.home_stop(); //switch to waiting
  loop();
  loop();
  loop();
  state_machine.go_button(); //probably won't work since mode button doesn't work
  loop();
  loop();
  loop();
  state_machine.end_stop();
  loop();
  loop();
  return 0;
  
} 
/****************************************************************************/

