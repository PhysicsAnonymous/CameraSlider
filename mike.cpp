
#include "Arduino.h"
#include "AccelStepper/AccelStepper.cpp"
#include "Bounce2-master/Bounce2.cpp"
#include "mike.hpp"

//void TBD(const char * str){};


/*** Global data ************************************************************/
AccelStepper SLIDER_MOTOR(AccelStepper::DRIVER, SLIDER_STEP_PIN, SLIDER_DIR_PIN);
AccelStepper CAMERA_MOTOR(AccelStepper::DRIVER, CAMERA_STEP_PIN, CAMERA_DIR_PIN);
SliderFSM state_machine;
long CAMERA_TARGET_START=0;
long CAMERA_TARGET_STOP=0;
long SLIDE_TARGET_STOP=SLIDER_MAX_POSITION;
ERROR_T ERR=ERROR_T::NONE;
char STATIC_MEMORY_ALLOCATION[sizeof(StateFirstAdjust)]; //Largest "state"

Bounce GO_BUTTON;
Bounce HOME_STOP;
Bounce END_STOP;

STATES REPEAT_TOGGLE=STATES::REVERSE_EXECUTE;
/****************************************************************************/

/*** Utility functions ******************************************************/
long de_jitter(const long value){
  static long previous = 0;
  if (abs(previous - value) <= MIN_CAMERA_JITTER){
    return previous;
  }
  previous = value;
  return value;
}

SWITCH_STATE read_3way(){
  bool video = !digitalRead(VIDEO_MODE_PIN);
  bool lapse = !digitalRead(LAPSE_MODE_PIN);
  #ifdef DEBUG_OUTPUT
    Serial.print("video switch: ");
    Serial.print(video);
    Serial.print(" lapse switch: ");
    Serial.println(lapse);
  #endif //DEBUG_OUTPUT
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
  #ifdef DEBUG_OUTPUT
    Serial.print("raw camera pot");
    Serial.println(pos_raw);
  #endif //DEBUG_OUTPUT
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
  #ifdef DEBUG_OUTPUT
    Serial.print("raw speed pot");
    Serial.println(raw_speed);
  #endif //DEBUG_OUTPUT
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

//STATES AbstractState::get_state_as_enum(){}


void* AbstractState::operator new(size_t sz){
  return (void*)STATIC_MEMORY_ALLOCATION;
}

void AbstractState::operator delete(void* p){
  //Technically, we should call the destructor here... but all our destructors
  //are empty anyway, so we won't bother.
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
  else {
    ERR=ERROR_T::SOFTWARE;
    m_target_state = STATES::ERROR;
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
    case REVERSE_EXECUTE:
      m_state = new StateReverseExecute(this);
      break;
    case REPEAT_WAIT:
      m_state = new StateRepeatWait(this);
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
  delay(1); //wait 50ms
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
  SLIDER_MOTOR.disableOutputs();
  CAMERA_MOTOR.disableOutputs();
};

STATES StateIdle::get_state_as_enum(){return STATES::IDLE;};

StateIdle::StateIdle(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** First home state *******************************************************/
void StateFirstHome::run_loop(){
  SLIDER_MOTOR.runSpeed();
  CAMERA_MOTOR.run();
}

bool StateFirstHome::transition_allowed(STATES new_state){
  return new_state == STATES::FIRST_ADJUST;
};

void StateFirstHome::enter_state(){
  SLIDER_MOTOR.enableOutputs();
  CAMERA_MOTOR.enableOutputs();
  if (!CAMERA_TARGET_START && !CAMERA_TARGET_STOP) {
    //If this is our first run, assume we are pointing the right way
      CAMERA_MOTOR.setCurrentPosition(0);
  }
  CAMERA_MOTOR.moveTo(0);
  SLIDER_MOTOR.move(-2147483648); //maximum negative distance
  SLIDER_MOTOR.setSpeed(MAX_HOMING_SPEED);
}

void StateFirstHome::home_stop(){
  SLIDER_MOTOR.setCurrentPosition(0);
  SLIDER_MOTOR.moveTo(0); //possibly unecessary, blocks until done 
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
    CAMERA_MOTOR.moveTo(target_pos);
    m_update_counter = TICKS_PER_POT_READ;
  }
  CAMERA_MOTOR.run();
  m_update_counter--;
}

bool StateFirstAdjust::transition_allowed(STATES new_state){
  return new_state == STATES::FIRST_END_MOVE;
};

void StateFirstAdjust::enter_state(){
  CAMERA_MOTOR.setCurrentPosition(0);
  m_update_counter=0;
}

void StateFirstAdjust::go_button(){
  CAMERA_TARGET_START = CAMERA_MOTOR.currentPosition();
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
  SLIDER_MOTOR.run();
  if (0 == SLIDER_MOTOR.distanceToGo()){
    m_machine->change_state(STATES::SECOND_ADJUST);
  }
}

bool StateFirstEndMove::transition_allowed(STATES new_state){
  return new_state == STATES::SECOND_ADJUST;
};

void StateFirstEndMove::enter_state(){
  SLIDER_MOTOR.moveTo(SLIDE_TARGET_STOP);
}

void StateFirstEndMove::home_stop(){
  ERR=ERROR_T::SOFTWARE;
  m_machine->change_state(STATES::ERROR);
}

void StateFirstEndMove::end_stop(){
  //Oops, we over-shot.  No problem, just stop quickly and update our target:
  SLIDE_TARGET_STOP = SLIDER_MOTOR.currentPosition();
  SLIDER_MOTOR.moveTo(SLIDE_TARGET_STOP);
  SLIDER_MOTOR.runToPosition(); //Go back to the position we just marked,
                                          //in case we over-shot.  Not sure if
                                          //this would actually work.
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
    CAMERA_MOTOR.moveTo(target_pos);
    m_update_counter = TICKS_PER_POT_READ;
  }
  CAMERA_MOTOR.run();
  m_update_counter--;
}

bool StateSecondAdjust::transition_allowed(STATES new_state){
  return new_state == STATES::SECOND_HOME;
};

void StateSecondAdjust::enter_state(){
  m_update_counter=0;
}

void StateSecondAdjust::go_button(){
  CAMERA_TARGET_STOP = CAMERA_MOTOR.currentPosition();
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
  SLIDER_MOTOR.run();
  CAMERA_MOTOR.run();
  if (0 == SLIDER_MOTOR.distanceToGo() && 0 == CAMERA_MOTOR.distanceToGo()){
    m_machine->change_state(STATES::WAIT);
  }
}

bool StateSecondHome::transition_allowed(STATES new_state){
  return new_state == STATES::WAIT;
};

void StateSecondHome::enter_state(){
  SLIDER_MOTOR.moveTo(0);
  CAMERA_MOTOR.moveTo(CAMERA_TARGET_START);
}

void StateSecondHome::home_stop(){
  //Make sure we stopped close enough to zero:
  long pos = SLIDER_MOTOR.currentPosition();
  if (pos > MAX_REHOME_DIFFERENCE){
    ERR=ERROR_T::UNKNOWN;
    m_machine->change_state(STATES::ERROR); }
  else {
    //update our targets based on our hitting home this time
    //(since this time we should have been almost stopped when we hit)
    //SLIDE_TARGET_STOP+=pos; //Don't update this since it's somewhat arbitrary anyway
    SLIDER_MOTOR.setCurrentPosition(0);
    m_machine->change_state(STATES::WAIT);
  }
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
  delay(1);
}

void StateWait::go_button(){
  if (SWITCH_STATE::PROGRAM_MODE != read_3way()){
    m_machine->change_state(STATES::EXECUTE);
  }
  //If we haven't chosen video or lapse mode, ignore the execute and wait.
}

bool StateWait::transition_allowed(STATES new_state){
  return STATES::EXECUTE == new_state;
};

STATES StateWait::get_state_as_enum(){return STATES::WAIT;};

StateWait::StateWait(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** Execute state **********************************************************/
void StateExecute::run_loop(){
  SLIDER_MOTOR.runSpeedToPosition();
  CAMERA_MOTOR.runSpeedToPosition();
  if (0 == SLIDER_MOTOR.distanceToGo() &&
      0 == CAMERA_MOTOR.distanceToGo()){
    m_machine->change_state(STATES::REPEAT_WAIT); //All done!
  }
}

void StateExecute::go_button(){
  //Treat "go" as emergency stop.
  ERR=ERROR_T::CANCEL;
  m_machine->change_state(STATES::ERROR);
}

void StateExecute::end_stop(){
  //All done, even if we didn't quite hit our targets
  m_machine->change_state(STATES::REPEAT_WAIT);
}

void StateExecute::enter_state(){
  REPEAT_TOGGLE=STATES::REVERSE_EXECUTE; //Next time, reverse excute if we go again.
  long secs = calculate_travel_time();
  if (0.0 == secs) {
    //Okay to use 0 comparison in float; we set this as an error condition
    //rather than calculating it.
    ERR=ERROR_T::SOFTWARE;
    m_machine->change_state(STATES::ERROR);
  }
  else {
    //Now that we know how long it should take, we can figure out how fast we
    //should move.
    //slider steps per second
    float slider_sps = SLIDE_TARGET_STOP / secs;
    //camera pan steps per second
    float camera_sps = (CAMERA_TARGET_STOP - CAMERA_TARGET_START) / secs;
    SLIDER_MOTOR.moveTo(SLIDE_TARGET_STOP);
    SLIDER_MOTOR.setSpeed(slider_sps);
    CAMERA_MOTOR.moveTo(CAMERA_TARGET_STOP);
    CAMERA_MOTOR.setSpeed(camera_sps);
    #ifdef DEBUG_OUTPUT
      Serial.print("secs: ");
      Serial.println(secs);
      Serial.print("slider speed: ");
      Serial.println(slider_sps);
      Serial.print("camera speed: ");
      Serial.println(camera_sps);
      Serial.print("camera position: ");
      Serial.println(CAMERA_MOTOR.currentPosition());
      Serial.print("camera start: ");
      Serial.println(CAMERA_TARGET_START);
      Serial.print("camera end: ");
      Serial.println(CAMERA_TARGET_STOP);
    #endif //DEBUG_OUTPUT
  }
}

bool StateExecute::transition_allowed(STATES new_state){
  return STATES::REPEAT_WAIT == new_state;
};

STATES StateExecute::get_state_as_enum(){return STATES::EXECUTE;};

StateExecute::StateExecute(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** RepeatWait state *******************************************************/
void StateRepeatWait::run_loop(){
  delay(1);
}

void StateRepeatWait::go_button(){
  if (SWITCH_STATE::PROGRAM_MODE != read_3way()){
    m_machine->change_state(REPEAT_TOGGLE);
  }
  else {
    //Otherwise, re-home and start again.
    m_machine->change_state(STATES::FIRST_HOME);
  }
}

bool StateRepeatWait::transition_allowed(STATES new_state){
  return STATES::FIRST_HOME == new_state || 
         STATES::REVERSE_EXECUTE == new_state || 
         STATES::EXECUTE == new_state;
};

STATES StateRepeatWait::get_state_as_enum(){return STATES::REPEAT_WAIT;};

StateRepeatWait::StateRepeatWait(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** ReverseExecute state ***************************************************/
void StateReverseExecute::run_loop(){
  SLIDER_MOTOR.runSpeedToPosition();
  CAMERA_MOTOR.runSpeedToPosition();
  if (0 == SLIDER_MOTOR.distanceToGo() &&
      0 == CAMERA_MOTOR.distanceToGo()){
    m_machine->change_state(STATES::REPEAT_WAIT); //All done!
  }
}

void StateReverseExecute::go_button(){
  //Treat "go" as emergency stop.
  ERR=ERROR_T::CANCEL;
  m_machine->change_state(STATES::ERROR);
}

void StateReverseExecute::home_stop(){
  //All done, even if we didn't quite hit our targets
  m_machine->change_state(STATES::REPEAT_WAIT);
}

void StateReverseExecute::enter_state(){
  REPEAT_TOGGLE=STATES::EXECUTE;//next time, execute if we repeat
  long secs = calculate_travel_time();
  if (0.0 == secs) {
    //Okay to use 0 comparison in float; we set this as an error condition
    //rather than calculating it.
    ERR=ERROR_T::SOFTWARE;
    m_machine->change_state(STATES::ERROR);
  }
  else {
    //Now that we know how long it should take, we can figure out how fast we
    //should move.
    //slider steps per second, negative this time because we are going back
    float slider_sps = -SLIDE_TARGET_STOP / secs;
    //camera pan steps per second (reversed because we are going back)
    float camera_sps = (CAMERA_TARGET_START - CAMERA_TARGET_STOP) / secs;
    SLIDER_MOTOR.moveTo(0);
    SLIDER_MOTOR.setSpeed(slider_sps);
    CAMERA_MOTOR.moveTo(CAMERA_TARGET_START);
    CAMERA_MOTOR.setSpeed(camera_sps);
    #ifdef DEBUG_OUTPUT
      Serial.print("secs: ");
      Serial.println(secs);
      Serial.print("slider speed: ");
      Serial.println(slider_sps);
      Serial.print("camera speed: ");
      Serial.println(camera_sps);
      Serial.print("camera position: ");
      Serial.println(CAMERA_MOTOR.currentPosition());
      Serial.print("camera start: ");
      Serial.println(CAMERA_TARGET_START);
      Serial.print("camera end: ");
      Serial.println(CAMERA_TARGET_STOP);
    #endif //DEBUG_OUTPUT
  }
}

bool StateReverseExecute::transition_allowed(STATES new_state){
  return STATES::REPEAT_WAIT == new_state;
};

STATES StateReverseExecute::get_state_as_enum(){return STATES::REVERSE_EXECUTE;};

StateReverseExecute::StateReverseExecute(SliderFSM* machine) : AbstractState(machine){};
/****************************************************************************/

/*** Error state **********************************************************/
void StateError::run_loop(){
  #ifdef ERROR_LED_PIN
  switch (ERR) {
    case NONE:
      //We shouldn't have an error if it's none.  Which makes this... a
      //software error?  Follow through with software error part:
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
  ERR=ERROR_T::NONE;
  m_machine->change_state(STATES::IDLE);
}

void StateError::enter_state(){
  SLIDER_MOTOR.disableOutputs();
  CAMERA_MOTOR.disableOutputs();
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
  //if our button has changed, and is high
  if (GO_BUTTON.update() && !GO_BUTTON.read()) {
    state_machine.go_button();
  }
  if (HOME_STOP.update() && !HOME_STOP.read()) {
    state_machine.home_stop();
  }
  if (END_STOP.update() && !END_STOP.read()) {
    state_machine.end_stop();
  }
}

void setup() {
//turn the light on while we boot (even though it will probably be off
//before humans can see it.)
pinMode(ERROR_LED_PIN, OUTPUT);

#ifdef DEBUG_OUTPUT
  Serial.begin(9600);
#endif
digitalWrite(ERROR_LED_PIN, HIGH);

//AccelStepper will set pins as output for us
SLIDER_MOTOR.setMaxSpeed(SLIDER_MAX_SPEED);
SLIDER_MOTOR.setAcceleration(SLIDER_MAX_ACCEL);
CAMERA_MOTOR.setMaxSpeed(CAMERA_MAX_SPEED);
CAMERA_MOTOR.setAcceleration(CAMERA_MAX_ACCEL);

pinMode(GO_PIN, INPUT_PULLUP);
pinMode(HOME_STOP_PIN, INPUT_PULLUP);
pinMode(END_STOP_PIN, INPUT_PULLUP);
pinMode(VIDEO_MODE_PIN, INPUT_PULLUP);
pinMode(LAPSE_MODE_PIN, INPUT_PULLUP);
GO_BUTTON.attach(GO_PIN);
HOME_STOP.attach(HOME_STOP_PIN);
END_STOP.attach(END_STOP_PIN);
//debounce interval in ms
GO_BUTTON.interval(20);
HOME_STOP.interval(20);
END_STOP.interval(20);

pinMode(SPEED_POT_PIN, INPUT);
pinMode(CAMERA_POT_PIN, INPUT);

digitalWrite(ERROR_LED_PIN, LOW);
}

/*
int main(void){
  setup();
  while(true){
    loop();
  }
  return 0;
}*/

/****************************************************************************/

