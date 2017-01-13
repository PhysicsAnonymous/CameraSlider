#ifndef CONFIG_H
#define CONFIG_H
//TODO: Not sure which of these are motor1 and motor2 in the drawing, so 
//      guesses used below.
//TODO: values for speed and accel and position are rectally derrived.

/*** Slider constants *******************************************************/
#define SLIDER_STEP_PIN 2 //D2
#define SLIDER_DIR_PIN 3 //D3
//In Steps/secon
#define SLIDER_MAX_ACCEL 4000.0
#define SLIDER_MAX_SPEED 8000.0
//Max possible position is 2,147,483,647.  
//Can be a function like steps/inch * inches
#define SLIDER_MAX_POSITION -400 * 32//-860 * 32
//If we head to zero, but hit the "home" switch a few ticks early, that's
//okay - but if we're way off that isn't.  This is the maximum steps we can
//be different from zero and still call it "home"
#define MAX_REHOME_DIFFERENCE 100
#define MAX_HOMING_SPEED 1000
/****************************************************************************/

/*** Camera constants *******************************************************/
#define CAMERA_STEP_PIN 4 //D4
#define CAMERA_DIR_PIN 5 //D5
#define CAMERA_MAX_ACCEL 4000.0
#define CAMERA_MAX_SPEED 2000.0
//Max possible position is 2,147,483,647
//Should probably be 1/4 steps required for a camera revolution
#define CAMERA_MAX_POSITION 1000
//Raise this number until the jitter at adjustment goes away
#define MIN_CAMERA_JITTER 2
/****************************************************************************/

/*** Timing constants *******************************************************/
//Time in seconds to traverse the length of the slider in both modes
#define VIDEO_TRAVERSAL_TIME_MIN 4
#define VIDEO_TRAVERSAL_TIME_MAX 15 //30

#define LAPSE_TRAVERSAL_TIME_MIN 20//30
#define LAPSE_TRAVERSAL_TIME_MAX 60//60 * 60 * 1 //60 seconds * 60 minutes *1 hour
/****************************************************************************/

/*** Button constants *******************************************************/
#define HOME_STOP_PIN 9 //D9
#define END_STOP_PIN 14 //Nope
#define GO_PIN 8 //D8
/****************************************************************************/

/*** POT constants **********************************************************/
#define SPEED_POT_PIN 1 //A1
#define CAMERA_POT_PIN 2 //A2
//Adjust this if the camera pan is too slow to adjust to the adjuster pot
#define TICKS_PER_POT_READ 1000
/****************************************************************************/

/*** Error LED constants ****************************************************/
#define ERROR_LED_PIN 13 //D13
/****************************************************************************/

/*** Three-way switch *******************************************************/
#define VIDEO_MODE_PIN 6 //D6
#define LAPSE_MODE_PIN 7 //D7
/****************************************************************************/

#endif //CONFIG_H

