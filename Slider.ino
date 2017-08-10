#include <Bounce2.h>

#include <MultiStepper.h>
#include <AccelStepper.h>

/******************************************************************************
All code is in the .cpp and .hpp files, rather than here.  This causes the
Arduino IDE to skip it's "automagical" build features, like header generation.
This is necessary because we use C++ features the Arduino IDE doesn't support,
like virtual functions.  However, the compiler Arduino IDE uses (GCC) is quite
capable of handling them.  The IDE, however, needs an .ino file, even if it is
blank.  So here is a blank file.  Don't worry, it will pull in all the .cpp
and .hpp files in this directory (another automagical feature.)
******************************************************************************/
