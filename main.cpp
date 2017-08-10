/*****************************************************************************
== Physics Anonymous CC-BY 2017 ==
******************************************************************************/
#include <Arduino.h>
#include <limits.h>
#include <Bounce2.h>
#include <AccelStepper.h>
#include "global.hpp"
#include "config.hpp"
#include "SliderFSM.hpp"

/*****************************************************************************
Adruino's libraries handle the actual "main()" function, and instead use
"setup()" and "loop()".  That's what we use here.
*****************************************************************************/

/*** Global data ************************************************************/
AccelStepper SLIDER_MOTOR(AccelStepper::DRIVER, SLIDER_STEP_PIN, SLIDER_DIR_PIN);
AccelStepper CAMERA_MOTOR(AccelStepper::DRIVER, CAMERA_STEP_PIN, CAMERA_DIR_PIN);
long CAMERA_TARGET_START=0;
long CAMERA_TARGET_STOP=0;
long SLIDE_TARGET_STOP=SLIDER_MAX_POSITION;
ERROR_T ERR=ERROR_T::NONE;

Bounce GO_BUTTON;
Bounce HOME_STOP;
Bounce END_STOP;

bool HAVE_HOMED=false;

int NEXT_DIRECTION=ENDWARD;

SliderFSM state_machine;

/****************************************************************************/

/*** loop *******************************************************************/
void loop() {
  //Checking and de-bouncing buttons is a time-consuming process - so we only
  //check a button every COUNTER_STEP steps.  This is plenty fast enough to
  //catch a physical buttom push, while leaving most of our time free for
  //managing the steppers.
  static int counter = 0;
  #define COUNTER_MAX 9
  #define COUNTER_STEP COUNTER_MAX / 3
  state_machine.run_loop(); //Update state if necessary
  if(COUNTER_STEP == counter){
    //if our button has changed, and is high
    if (GO_BUTTON.update() && !GO_BUTTON.read()) {
      DEBUG(F("Go button pressed"));
      state_machine.go_button();
    }
  }
  if(COUNTER_STEP * 2 == counter){
    if (HOME_STOP.update() && !HOME_STOP.read()) {
      DEBUG(F("Home stop hit"));
      state_machine.home_stop();
    }
  }
  if(0 == counter){
    if (END_STOP.update() && !END_STOP.read()) {
      DEBUG(F("End stop hit"));
      state_machine.end_stop();
    }
    counter = COUNTER_MAX; //loops until button check
  }
  counter--;
  #undef COUNTER_MAX
  #undef COUNTER_STEP
}
/****************************************************************************/

/*** setup ******************************************************************/
void setup() {
//turn the light on while we boot (even though it will probably be off
//before humans can see it.)
pinMode(ERROR_LED_PIN, OUTPUT);
digitalWrite(ERROR_LED_PIN, HIGH);

#ifdef DEBUG_OUTPUT
  Serial.begin(9600);
#endif
DEBUG(F("Starting up"));

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
GO_BUTTON.interval(DEBOUNCE_INTERVAL);
HOME_STOP.interval(DEBOUNCE_INTERVAL);
END_STOP.interval(DEBOUNCE_INTERVAL);

pinMode(SPEED_POT_PIN, INPUT);
pinMode(CAMERA_POT_PIN, INPUT);

//Turn the light back off to signal bootup is complete.
digitalWrite(ERROR_LED_PIN, LOW);
}

/****************************************************************************/
