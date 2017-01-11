#ifndef CONFIG_H
#define CONFIG_H
//TODO: Not sure which of these are motor1 and motor2 in the drawing, so 
//      guesses used below.
//TODO: values for speed and accel and position are rectally derrived.

/*** Slider constants *******************************************************/
#define SLIDER_STEP_PIN 32
#define SLIDER_DIR_PIN 1
//In Steps/secon
#define SLIDER_MAX_ACCEL 1000.0
#define SLIDER_MAX_SPEED 5000.0
//Max possible position is 2,147,483,647.  
//Can be a function like steps/inch * inches
#define SLIDER_MAX_POSITION 860 * 32
//If we head to zero, but hit the "home" switch a few ticks early, that's
//okay - but if we're way off that isn't.  This is the maximum steps we can
//be different from zero and still call it "home"
#define MAX_REHOME_DIFFERENCE 100
#define MAX_HOMING_SPEED 1000
/****************************************************************************/

/*** Camera constants *******************************************************/
#define CAMERA_STEP_PIN 2
#define CAMERA_DIR_PIN 9
#define CAMERA_MAX_ACCEL 1000.0
#define CAMERA_MAX_SPEED 200.0
//Max possible position is 2,147,483,647
//Should probably be 1/4 steps required for a camera revolution
#define CAMERA_MAX_POSITION 500
/****************************************************************************/

/*** Timing constants *******************************************************/
//Time in seconds to traverse the length of the slider in both modes
#define VIDEO_TRAVERSAL_TIME_MIN 6
#define VIDEO_TRAVERSAL_TIME_MAX 30

#define LAPSE_TRAVERSAL_TIME_MIN 30
#define LAPSE_TRAVERSAL_TIME_MAX 60 * 60 * 1 //60 seconds * 60 minutes *1 hour
/****************************************************************************/

/*** Button constants *******************************************************/
#define HOME_STOP_PIN 13
#define END_STOP_PIN 14
#define GO_PIN 12
/****************************************************************************/

/*** POT constants **********************************************************/
#define SPEED_POT_PIN 24
#define CAMERA_POT_PIN 25
//Adjust this if the camera pan is too slow to adjust to the adjuster pot
#define TICKS_PER_POT_READ 1000
/****************************************************************************/

/*** Error LED constants ****************************************************/
#define ERROR_LED_PIN 16
/****************************************************************************/

/*** Three-way switch *******************************************************/
#define VIDEO_MODE_PIN PD6
#define LAPSE_MODE_PIN PD7
/****************************************************************************/

#endif //CONFIG_H

