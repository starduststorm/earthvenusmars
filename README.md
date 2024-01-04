# Earth Venus Mars
##### *if men are from mars and women are from venus then does that mean enbies are from earth 🧐*

## A Wearable LED Transgender Symbol
##### *because I wanted this to exist*
#
### I made a small pile of these
You can get them on [Tindie].
#
## Hardware

* LEDs: SK9822-EC20, very tiny & very bright
* PCB: Designed with KiCad, design files are in ./board
* Chip: SAMD21G18a, 32-bit ARM

<!-- ![KiCad Screenshot of PCB](doc/assets/pcb.png "PCB") -->
<img src="doc/assets/pcb.png" alt="screenshot of kicad design" height=400>
&nbsp;
<img src="doc/assets/board.jpg" alt="a fully assembled pcb" height=400>

### Features: 
* 78 blindingly-bright LEDs
* Three capacitive touch pads designed into the aesthetic for interactivity/customizability
* Two SPST buttons for selecting patterns & colors
* Onboard I2S microphone for sound responsiveness
* Onboard thermistor for temperature management
* Brightness dial to manage level of eye-lasering
* Power switch for longer storage
* USB-C for power & hacking, requires USB Power
* SWD pads for low-level hacking
* Exposed (but unused) I2C pads and SPI pads for you to hack with as you please
* Mount points for bar pin, chain, other attachments

#
## Software

Arduino-compatible, C++ code built to run against the Arduino Zero bootloader & stack on a SAMD21g mcu.

<img src="doc/assets/lit.jpg" alt="This board is brightly lit with trans flag colors" height=400>

### Features

* LED patterns custom-designed for this pixel layout
* Sound patterns responsive to volume & pitch
* Touch responsiveness for playing with the venus, mars, and 'earth' iconography
* Power management built in to brightness dial - turn the dial down to put the device to sleep
* Automatic thermal management scales down max brightness since the LEDs get very hot when at higher brightnesses
* Paletting system for a variety of pride flags
* Modes for automatic palette & pattern rotation

### Code

The project is built with arm-none-eabi-g++ using [PlatformIO] in Visual Studio Code.

### Libraries & Dependencies

* [FastLED]
* [Adafruit Zero FFT]
* [Adafruit Zero I2S]
* [Adafruit FreeTouch]

### Build Environments

There are multiple build environments, referencing the hardware version of the pcb. 
* v1 - don't use
* v2 - Plastic buttons, small number of these out in the wild
* v3 - Metal buttons, Tindie version

### Controls

* Power switch (lower right)

* Brightness dial (lower left)

* Colors
  - Single press - Next Palette
  - Double Press - Previous Palette
  - Long Press - Palette Rotation

* Patterns
  - Single Press - Next Pattern
  - Double Press - Previous Pattern
  - Long Press - Pattern Rotation

* Circular metal touchpads control each "spoke", the earth, venus, and mars symbols, of the design
  - Press and hold for half a second to keep spoke active
  - Pressing the <i>Colors</i> or <i>Patterns</i> button while holding a touch pad will change the pattern or colors on that spoke only. Use this to mix and match.

Note that the top-left "earth" spoke cycles through additional states when using the <i>Patterns</i>-button chord. You can use this to e.g. turn off the cross-bar or arrow shape pixels only, allowing that spoke to be used as a mars or venus symbol to give the device two mars or two venus symbols (gay).

### Power

The board is designed to be portable & wearable and will run off of just about any USB battery pack. However, when running at full brightness the LEDs may pull up to 15W and make short work of small batteries. 

When the brightness dial is turned all the way down, the device will enter sleep mode but will still passively draw power if the power switch is on.

### Temperature

When the LEDs are lit near maximum brightness they dissipate a lot of heat. Software will automatically begin scaling maximum brightness down when a temperature sensor reads around 45°C, but parts of the board may still become too hot to touch. 

<hr>

## Third party thanks

This project would not have been possible without the incredible open-source work by the fine folks at [FastLED], which drives SPI for the LEDs in this project and provides lots of efficient 8- and 16-bit math & palette routines, and is a joy to work with.

Similarly many of the advanced features of this project are only possible due to the open-source work at the relentlessly-brilliant [Adafruit], including SAMD support for the I2S peripheral, a high-level FFT library for sound responsiveness, and an abstraction of all the complexities of QTouch with the FreeTouch library. On top of that there's adalink for bootloader programming & lots of help in Adafruit's guides on everything from bootloaders to thermistors.

And of course the venerable [Arduino] for creating such a widespread hackable platform to begin with.

Thank you for all the work you do for makers & hobbyists the world over.

<hr>

<!-- github no likey video tag -->
<!-- <video height="646" src="doc/assets/seven.mp4" type="video/mp4" loop autoplay /> -->
<img src="doc/assets/seven.webp" style="width: 861px, height: auto">

[Arduino]: <https://arduino.cc>
[Adafruit]: <https://www.adafruit.com>
[FastLED]: https://github.com/FastLED/FastLED
[Adafruit Zero FFT]: <https://github.com/adafruit/Adafruit_ZeroFFT>
[Adafruit Zero I2S]: <https://github.com/adafruit/Adafruit_ZeroI2S>
[Adafruit FreeTouch]: <https://github.com/adafruit/Adafruit_FreeTouch>
[PlatformIO]: <https://platformio.org>
[Tindie]: <https://www.tindie.com/products/starduststorm/earthvenusmars>

