# Slider

## How to build

You'll need the following libraries installed to compile:

* Bounce2 (available in the Arduino IDE library, or [here](https://github.com/thomasfredericks/Bounce2_).

* AccelStepper (available [here](http://www.airspayce.com/mikem/arduino/AccelStepper/))

### Arduino IDE

Open the sketch, add the Bounce2 and AccelStepper libraries, and click the
check mark.

### Command line

Assuming you are using linux, make sure the appropriate packages are installed.
This will include gcc, avr-gcc, arduino, and arduino.mk, and possibly more
depending on your distribution.

Then verify the paths in the included Makefile are correct for your distribution.

Then run "make"

## Usage

The camera slider software is intended to be operated with a three-way switch,
a momentary switch, and two adjustable pots (camera and speed).

Set the three-way switch to "programming mode" and press the button.  The
slider will home.  When it reaches home, adjust the camera pot to point the
camera in the desired direction, then press the button again.  The slider will
move to the end of the rail, where you can again adjust the camera with the
pot.  Press the button again, and the slider will return to home.  Now also
adjust your speed pot for the desired speed, and adjust the
switch from programming to either video or time lapse (which will control the
range of the pot.  See config.hpp for details).  Lastly, press the execute
button again, and the slider will run at the desired speed, rotating the camera
to the desired position as it goes.

## License

Creative Commons "By" license.  Details are [here](https://creativecommons.org/licenses/by/4.0/),
but basically do whatever you want with this code as long as you attribute the original authors.
