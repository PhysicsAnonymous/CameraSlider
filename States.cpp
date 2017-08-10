/*****************************************************************************
== Physics Anonymous CC-BY 2017 ==
******************************************************************************/

#include <Arduino.h>
#include <limits.h>
#include <Bounce2.h>
#include <AccelStepper.h>
#include "config.hpp"
#include "global.hpp"
#include "SliderFSM.hpp"
#include "States.hpp"

/*** Global externs **********************************************************
  Globals are defined in main.cpp to keep them in one place, but we use
  several of them here.

  //TODO: consider movign all of this to a hardware class, because
  OOP/encapsulation/blahblahblah.
*****************************************************************************/
extern AccelStepper SLIDER_MOTOR;
extern AccelStepper CAMERA_MOTOR;
extern long CAMERA_TARGET_START;
extern long CAMERA_TARGET_STOP;
extern long SLIDE_TARGET_STOP;
extern ERROR_T ERR;
extern Bounce GO_BUTTON;
extern Bounce HOME_STOP;
extern Bounce END_STOP;
extern int NEXT_DIRECTION;
extern bool HAVE_HOMED;

//We dynamically allocate a new state on each state transition.  This would
//require us to introduce complex memory management, such as a heap, on the
//arduino - except that we only ever need one state at a time.  So instead
//we'll just reserve a slice of memory that is as large as our largest state,
//and use that for all state instantiations.
//if any child state is larger, use it as the size of our reserve.  If they
//are all the same, just pick one.
char STATIC_MEMORY_ALLOCATION[sizeof(ConcreteState<STATES::ADJUST>)];
/****************************************************************************/

/*** Utility functions ******************************************************/

long de_jitter(const long value){
  static int previous = 0;
  if (abs(previous - value) <= MIN_CAMERA_JITTER){
    return previous;
  }
  previous = value;
  return value;
}

SWITCH_STATE read_3way(){
  bool video = !digitalRead(VIDEO_MODE_PIN);
  bool lapse = !digitalRead(LAPSE_MODE_PIN);
  DEBUG(F("vswitch: "), video);
  DEBUG(F("lswitch: "), lapse);
  if (video && lapse) { return SWITCH_STATE::INVALID; }
  else if (video) { return SWITCH_STATE::VIDEO_MODE; }
  else if (lapse) { return SWITCH_STATE::LAPSE_MODE; }
  else { return SWITCH_STATE::PROGRAM_MODE; }
}

long read_camera_pot(){
  long pos_raw = analogRead(CAMERA_POT_PIN);
  #ifdef REVERSE_CAMERA_POT
    pos_raw = 1023 - pos_raw;
  #endif // REVERSE_CAMERA_POT
  //DEBUG(F("raw camera pot: "),pos_raw);
  //make sure there was a real change, so we aren't going back and forth
  //between two very close values.
  pos_raw = de_jitter(pos_raw);
  //Results are between 0 and 1023, shift it between -512 511, and scale it
  //to our camera min/max position:
  long target_pos = (pos_raw-512) * (CAMERA_MAX_POSITION/512.0);
  return target_pos;
}

float calculate_travel_time(){
  long raw_speed=analogRead(SPEED_POT_PIN);
  #ifdef REVERSE_SPEED_POT
    raw_speed = 1023 - raw_speed;
  #endif //REVERSE_SPEED_POT
  //DEBUG(F("raw speed pot"),raw_speed);
  long range,start;
  //Get the range (from 5 seconds to 30 seconds has a 25 second range)
  //and then scale it by the value of the slider (2.5v out of 5v would
  //be half, 12.5s.)  Then add it to the start (5s) for the  total
  //traversal time (17.5s in this case.)
  switch (read_3way()) {
    case VIDEO_MODE:
      range = VIDEO_TRAVERSAL_TIME_MAX - VIDEO_TRAVERSAL_TIME_MIN;
      start = VIDEO_TRAVERSAL_TIME_MIN;
      break;
    case LAPSE_MODE:
      range = LAPSE_TRAVERSAL_TIME_MAX - LAPSE_TRAVERSAL_TIME_MIN;
      start = LAPSE_TRAVERSAL_TIME_MIN;
      break;
    default:
      return 0.0;
  }
  float secs = ( float(raw_speed)/1023.0 ) * float(range);
  secs += start;
  return secs;
}

//initial guess is how far past the stop we *think* we went.  Guess 10 if
//we are clueless.
void back_off_stop(Bounce &stop){
  const long FIRST_BACK_OFF_STEPS = 50;
  const long BACK_OFF_STEPS = 10;
  //First, make sure we stop!
  SLIDER_MOTOR.setSpeed(0);
  //Now let's find some directions
  long direction = ENDWARD;
  if(&END_STOP == &stop){
    direction = HOMEWARD;
  }
  digitalWrite(ERROR_LED_PIN, HIGH);
  long distance= FIRST_BACK_OFF_STEPS * direction; //Arbitrary number of steps
  stop.update();
  while (!stop.read()){
    DEBUG(F("backing off stop (steps): "),distance);
    SLIDER_MOTOR.move(distance); //relative move
    SLIDER_MOTOR.runToPosition();
    //wait to ensure there's no button bounce
    delay(1);
    stop.update();
    distance= BACK_OFF_STEPS * direction;
  }
  digitalWrite(ERROR_LED_PIN, LOW);
}

void get_accels(float slider_sps, float camera_sps, float &slider_accel, float &camera_accel){
   float slider_accel_time = abs(slider_sps / SLIDER_MAX_ACCEL);
   float camera_accel_time = abs(camera_sps / CAMERA_MAX_ACCEL);
   //whichever takes the longest, that is what we must scale to
   if (slider_accel_time >= camera_accel_time){
     camera_accel = abs(camera_sps/slider_accel_time);
     slider_accel = SLIDER_MAX_ACCEL;
   }
   else { //just the opposite calculation:
     slider_accel = abs(slider_sps/camera_accel_time);
     camera_accel = CAMERA_MAX_ACCEL;
  }
}


/****************************************************************************/

/*** AbstractState **********************************************************/
void* AbstractState::operator new(size_t sz){
  return (void*)STATIC_MEMORY_ALLOCATION;
};

void AbstractState::operator delete(void* p){
  //Technically, we should call the destructor here... but all our destructors
  //are empty anyway, so we won't bother.
}

//These could be templates too, but typing the function name is easier than
//typing the template.
void AbstractState::setSoftwareError(){
  DEBUG(F("Software Error"));
  ERR=ERROR_T::SOFTWARE;
  m_machine->change_state(STATES::ERROR);
}

void AbstractState::setUnknownError(){
  DEBUG(F("Unknown Error"));
  ERR=ERROR_T::UNKNOWN;
  m_machine->change_state(STATES::ERROR);
}

void AbstractState::setCancel(){
  DEBUG(F("Cancel"));
  ERR=ERROR_T::CANCEL;
  m_machine->change_state(STATES::ERROR);
}

/****************************************************************************/

/*** Wait state *************************************************************/
template<>
void StateWait::go_button(){
  if (SWITCH_STATE::PROGRAM_MODE == read_3way()){
    //If we are in programming mode and execute, we go home no matter what
    //we had previously been doing.
    NEXT_DIRECTION=HOMEWARD;
    m_machine->change_state(STATES::FIRST_HOME);
  }
  else { //not in program mode
    //as long as we have established a home, go ahead and run
    if (HAVE_HOMED) {
      m_machine->change_state(STATES::EXECUTE);
    }
    else { //not in program mode, and have no targets/home set
      setUnknownError(); // Not really an error, just blink to let user
                         //know we are not in the right mode! (program mode)
    }
  }
}

template<>
bool StateWait::transition_allowed(STATES new_state){
  return new_state == FIRST_HOME ||
         new_state == EXECUTE;
}
/****************************************************************************/

/*** First home state *******************************************************/
template<>
void StateFirstHome::run_loop(){
  SLIDER_MOTOR.runSpeed();
  CAMERA_MOTOR.run(); //Camera should already be at correct position
}

template<>
bool StateFirstHome::transition_allowed(STATES new_state){
  return new_state == STATES::ADJUST;
};

template<>
void StateFirstHome::enter_state(){
  //Set everything back to defaults
  CAMERA_TARGET_START=0;
  CAMERA_TARGET_STOP=0;
  SLIDER_MOTOR.enableOutputs();
  CAMERA_MOTOR.enableOutputs();
  CAMERA_MOTOR.setCurrentPosition(0);
  CAMERA_MOTOR.moveTo(0);
  long target = LONG_MAX; //Unfortunately the macro processor complains if
  target *= HOMEWARD;     //we try to do this math with macros
  SLIDER_MOTOR.move(target); //maximum negative distance
  SLIDER_MOTOR.setSpeed(MAX_HOMING_SPEED);
}

template<>
void StateFirstHome::exit_state(){
  NEXT_DIRECTION = ENDWARD;
}

template<>
void StateFirstHome::home_stop(){
  back_off_stop(HOME_STOP);
  SLIDER_MOTOR.setCurrentPosition(0);
  HAVE_HOMED=true;
  m_machine->change_state(STATES::ADJUST);
}

template<>
void StateFirstHome::end_stop(){
  DEBUG(F("We went the wrong way"));
  setSoftwareError();
}

template<>
void StateFirstHome::go_button(){
  //The "go" button is now "stop."  Like pressing the "start menu" to shut down.
  setCancel();
}
/****************************************************************************/

/*** Adjust state ***********************************************************/
template<>
void StateAdjust::run_loop(){
  //Only read the pot every TICKS_PER_POT_READ, because Analog read is slow.
  static int counter = 0;
  if (0 >= counter){
    long target_pos=read_camera_pot();
    CAMERA_MOTOR.moveTo(target_pos);
    counter = TICKS_PER_POT_READ;
  }
  CAMERA_MOTOR.run();
  counter--;
}

template<>
bool StateAdjust::transition_allowed(STATES new_state){
  return new_state == STATES::FIRST_END_MOVE ||
         new_state == STATES::SECOND_HOME;
};

template<>
void StateAdjust::go_button(){
  if(ENDWARD == NEXT_DIRECTION) {
    CAMERA_TARGET_START = CAMERA_MOTOR.currentPosition();
    m_machine->change_state(STATES::FIRST_END_MOVE);
  }
  else {
    CAMERA_TARGET_STOP = CAMERA_MOTOR.currentPosition();
    m_machine->change_state(STATES::SECOND_HOME);
  }
}
/****************************************************************************/

/*** First end move state ***************************************************/
template<>
void StateFirstEndMove::run_loop(){
  SLIDER_MOTOR.run();
  if (0 == SLIDER_MOTOR.distanceToGo()){
    m_machine->change_state(STATES::ADJUST);
  }
}

template<>
bool StateFirstEndMove::transition_allowed(STATES new_state){
  return new_state == STATES::ADJUST;
};

template<>
void StateFirstEndMove::enter_state(){
  SLIDER_MOTOR.moveTo(SLIDE_TARGET_STOP);
}

template<>
void StateFirstEndMove::exit_state(){
  NEXT_DIRECTION = HOMEWARD;
}

template<>
void StateFirstEndMove::home_stop(){
  DEBUG(F("We went the wrong way"));
  setSoftwareError();
}

template<>
void StateFirstEndMove::end_stop(){
  //Oops, we over-shot.  No problem, just update our target:
  back_off_stop(END_STOP);
  SLIDE_TARGET_STOP = SLIDER_MOTOR.currentPosition();
  SLIDER_MOTOR.moveTo(SLIDE_TARGET_STOP);
  m_machine->change_state(STATES::ADJUST);
}

template<>
void StateFirstEndMove::go_button(){
  //The "go" button is now "stop."  Like pressing the "start menu" to shut down.
  setCancel();
}
/****************************************************************************/

/*** Second home state ******************************************************/
template<>
void StateSecondHome::run_loop(){
  SLIDER_MOTOR.run();
  CAMERA_MOTOR.run();
  if (0 == SLIDER_MOTOR.distanceToGo() && 0 == CAMERA_MOTOR.distanceToGo()){
    m_machine->change_state(STATES::WAIT);
  }
}

template<>
bool StateSecondHome::transition_allowed(STATES new_state){
  return new_state == STATES::WAIT;
};

template<>
void StateSecondHome::enter_state(){
  SLIDER_MOTOR.moveTo(0);
  CAMERA_MOTOR.moveTo(CAMERA_TARGET_START);
}

template<>
void StateSecondHome::exit_state(){
  NEXT_DIRECTION = ENDWARD;
}

template<>
void StateSecondHome::home_stop(){
  //Make sure we stopped close enough to zero:
  back_off_stop(HOME_STOP);
  SLIDER_MOTOR.setCurrentPosition(0);
  m_machine->change_state(STATES::WAIT);
  //This blocks, but also should never not be finished (so it should return
  //almost immediately)
  CAMERA_MOTOR.runToPosition(); //make sure our camera movement is finished too!
}

template<>
void StateSecondHome::end_stop(){
  setSoftwareError();
}

template<>
void StateSecondHome::go_button(){
  //The "go" button is now "stop."  Like pressing the "start menu" to shut down.
  setCancel();
}
/****************************************************************************/

/*** Execute state **********************************************************/
template<>
void StateExecute::run_loop(){
  SLIDER_MOTOR.run();
  CAMERA_MOTOR.run();
  if (0 == SLIDER_MOTOR.distanceToGo() &&
      0 == CAMERA_MOTOR.distanceToGo()){
    m_machine->change_state(STATES::WAIT); //All done!
  }
}


template<>
void StateExecute::go_button(){
  //Treat "go" as emergency stop.
  setCancel();
}

template<>
void StateExecute::end_stop(){
  back_off_stop(END_STOP);
  //All done, even if we didn't quite hit our targets
  m_machine->change_state(STATES::WAIT);
}

template<>
void StateExecute::home_stop(){
  //All done, even if we didn't quite hit our targets
  back_off_stop(HOME_STOP);
  m_machine->change_state(STATES::WAIT);
}

template<>
void StateExecute::enter_state(){
  float secs = calculate_travel_time();
  if (0.0 == secs) {
    //Okay to use 0 comparison in float; we set this as an error condition
    //rather than calculating it.
    setSoftwareError();
  }
  else {
    //Now that we know how long it should take, we can figure out how fast we
    //should move.
    //slider steps per second
    float slider_sps = (float)SLIDE_TARGET_STOP / secs;
    slider_sps *= (float)NEXT_DIRECTION;
    //camera pan steps per second
    float camera_sps = ((float)CAMERA_TARGET_STOP -
      (float)CAMERA_TARGET_START) / secs;
    camera_sps *= (float)NEXT_DIRECTION;
    if(ENDWARD == NEXT_DIRECTION){
      SLIDER_MOTOR.moveTo(SLIDE_TARGET_STOP);
      CAMERA_MOTOR.moveTo(CAMERA_TARGET_STOP);
    }
    else {
      SLIDER_MOTOR.moveTo(0);
      CAMERA_MOTOR.moveTo(CAMERA_TARGET_START);
    }
    float slider_accel=0;
    float camera_accel=0;
    get_accels(slider_sps, camera_sps, slider_accel, camera_accel);
    SLIDER_MOTOR.setAcceleration(slider_accel);
    SLIDER_MOTOR.setMaxSpeed(abs(slider_sps));
    CAMERA_MOTOR.setAcceleration(camera_accel);
    CAMERA_MOTOR.setMaxSpeed(abs(camera_sps));

    DEBUG(F("NEXT_DIRECTION: "),NEXT_DIRECTION);
    DEBUG(F("secs: "),secs);
    DEBUG(F("slider speed: "),slider_sps);
    DEBUG(F("camera speed: "),camera_sps);
    DEBUG(F("slider accel: "),slider_accel);
    DEBUG(F("camera accel: "),camera_accel);
    DEBUG(F("camera cur pos: "),CAMERA_MOTOR.currentPosition());
    DEBUG(F("camera target: "),CAMERA_MOTOR.targetPosition());
    DEBUG(F("slider cur pos: "),SLIDER_MOTOR.currentPosition());
    DEBUG(F("slider target: "),SLIDER_MOTOR.targetPosition());
  }
}

template<>
void StateExecute::exit_state(){
  NEXT_DIRECTION*= -1; //Reverse direction when we hit the other side
  //return motors to normal speeds
  SLIDER_MOTOR.setAcceleration(SLIDER_MAX_ACCEL);
  SLIDER_MOTOR.setMaxSpeed(SLIDER_MAX_SPEED);
  CAMERA_MOTOR.setAcceleration(CAMERA_MAX_ACCEL);
  CAMERA_MOTOR.setMaxSpeed(CAMERA_MAX_SPEED);
}

template<>
bool StateExecute::transition_allowed(STATES new_state){
  return STATES::WAIT == new_state;
};
/****************************************************************************/

/*** Error state ************************************************************/
template<>
void StateError::run_loop(){
  #ifdef ERROR_LED_PIN
  switch (ERR) {
    case NONE:
      //We shouldn't have an error if it's none.  Which makes this... a
      //software error?  Follow through with software error part:
      DEBUG(F("ERROR NONE?"));
    case SOFTWARE:
      for (int i = 0; i<3; i++) {
        digitalWrite(ERROR_LED_PIN, HIGH);
        delay(500);
        digitalWrite(ERROR_LED_PIN, LOW);
        delay(500);
      }
      break;
    case CANCEL:
      for (int i = 0; i<2; i++) {
        digitalWrite(ERROR_LED_PIN, HIGH);
        delay(1000);
        digitalWrite(ERROR_LED_PIN, LOW);
        delay(500);
      }
      break;
    case UNKNOWN:
      digitalWrite(ERROR_LED_PIN, HIGH);
      delay(1500);
      digitalWrite(ERROR_LED_PIN, LOW);
  }
  #endif
  //if we haven't defined an error pin, just leave this mode.
  //Since we don't know what happened, better to assume we haven't even homed
  HAVE_HOMED=false;
  ERR=ERROR_T::NONE;
  m_machine->change_state(STATES::WAIT);
}

template<>
void StateError::enter_state(){
  SLIDER_MOTOR.disableOutputs();
  CAMERA_MOTOR.disableOutputs();
}

template<>
bool StateError::transition_allowed(STATES new_state){
  return STATES::WAIT == new_state;
};
/****************************************************************************/
