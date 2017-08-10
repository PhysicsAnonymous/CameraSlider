#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG_OUTPUT

/*** Slider constants *******************************************************/
#define SLIDER_STEP_PIN 2 //D2
#define SLIDER_DIR_PIN 3 //D3
//In Steps/second
#define SLIDER_MAX_ACCEL 3000.0
#define SLIDER_MAX_SPEED 8000.0
//Max possible position is 2,147,483,647.
//Can be a function like steps/inch * inches, but recommend parens ()
#define SLIDER_MAX_POSITION (-400 * 32)
#define MAX_HOMING_SPEED 500
/****************************************************************************/

/*** Camera constants *******************************************************/
#define CAMERA_STEP_PIN 4 //D4
#define CAMERA_DIR_PIN 5 //D5
//in Steps/second
#define CAMERA_MAX_ACCEL 4000.0
#define CAMERA_MAX_SPEED 3000.0
//Max possible position is 2,147,483,647
//Should probably be 1/4 steps required for a 360 degree camera revolution
#define CAMERA_MAX_POSITION 1000
//Raise this number until the jitter at adjustment goes away
#define MIN_CAMERA_JITTER 2
/****************************************************************************/

/*** Timing constants *******************************************************/
//Time in seconds to traverse the length of the slider in both modes
#define VIDEO_TRAVERSAL_TIME_MIN 10
#define VIDEO_TRAVERSAL_TIME_MAX 60

#define LAPSE_TRAVERSAL_TIME_MIN 60
#define LAPSE_TRAVERSAL_TIME_MAX 600
/****************************************************************************/

/*** Button constants *******************************************************/
#define HOME_STOP_PIN 9 //D9
#define END_STOP_PIN 14 //Not used by mike's board yet
#define GO_PIN 8 //D8
#define DEBOUNCE_INTERVAL 20 //in us
/****************************************************************************/

/*** POT constants **********************************************************/
#define SPEED_POT_PIN 1 //A1
#define CAMERA_POT_PIN 2 //A2
//Adjust this if the camera pan is too slow to adjust to the adjuster pot
#define TICKS_PER_POT_READ 1000
//Use these if the direction of your pot doesn't match up with the direction
//you want the interface to work
#define REVERSE_SPEED_POT
//#define REVERSE_CAMERA_POT
/****************************************************************************/

/*** Error LED constants ****************************************************/
#define ERROR_LED_PIN 13 //D13
/****************************************************************************/

/*** Three-way switch *******************************************************/
#define VIDEO_MODE_PIN 6 //D6
#define LAPSE_MODE_PIN 7 //D7
/****************************************************************************/

#endif //CONFIG_H
