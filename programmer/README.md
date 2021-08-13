# Programmer Rig

Bootloader-programming rig matched to the main PCB board shape allows SWD programming & debugging. Pogo-pins on the programming board contact SWD pins on the main board while a 3D-printed part holds the board in place.

## Assembly

#### Parts
* 3D-printed caddy.stl
* 0985-1-15-20-71-14-11-0 [Pogo pins]
* M3.5 screws
* ATMEL programmer device such as the (non-commercial) [J-Link Edu Mini]

An assembled EVM board should slot into the programmer rig & make contact with the pogo pins.

<img src="../doc/assets/programmer.jpg" alt="An assembled programmer rig" height=400>

#
## Programming Instructions

These instructions are only necessary for reflashing the bootloader on the samd21g in case of assembling a board from scratch. An already-programmed device will appear as an Arduino Zero over USB for loading regular code.

* Assemble a programmer rig
* Download and build a copy of the Arduino Zero bootloader from [ArduinoCore-samd]
* Install [adalink] (If you're not on Widows)
* Attach 3.3v to the programmer pcb
* Run

```sh
$ adalink -v atsamd21g18 -p jlink -w -h path/to/ArduinoCore-samd/bootloaders/zero/samd21_sam_ba.hex
```
OR
If you're on Windows you can probably use ATMEL's first-party programming tools, though I haven't tried this.

[Pogo pins]: <https://www.digikey.com/en/products/detail/mill-max-manufacturing-corp/0985-1-15-20-71-14-11-0/5823029>
[adalink]: <https://github.com/adafruit/Adafruit_Adalink>
[ArduinoCore-samd]: <https://github.com/arduino/ArduinoCore-samd>
[J-Link Edu Mini]: <https://www.segger.com/products/debug-probes/j-link/models/j-link-edu-mini/>
