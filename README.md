# Teensy SN76489 MIDI Synth
This project contains code and instructions to use a
[SN76489 sound chip](https://en.wikipedia.org/wiki/Texas_Instruments_SN76489)
as a MIDI controlled synth, driven by a Teensy 2.0 microcontroller.

## Getting it Running
1. Install the [Arduino IDE](https://www.arduino.cc/en/Main/Software)
1. Install the [Teensy software](https://www.pjrc.com/teensy/tutorial.html)
   for the Arduino IDE
1. Open the `teensy-sn76489-midi-synth.ino` file in the Arduino IDE
1. Under the `Tools` menu option select `Board` -> `Teensy 2.0`
1. Under the `Tools` menu option select `USB Type` -> `MIDI`
1. Under the `Sketch` menu option select `Upload`
1. In your [DAW](https://en.wikipedia.org/wiki/Digital_audio_workstation) of
   choice select the "Teensy MIDI" output device and use the MIDI channels as
   described in the "MIDI Configuration" section below

## MIDI Configuration
### Channel 1
* Controls square tone 1
* Pitch wheel is supported on this channel
* Note velocity is linked to the 16 level attenuation of square tone 1
* The brightness of LED 1 is linked to the attenuation level of square tone 1

### Channel 2
* Controls square tone 2
* Pitch wheel is supported on this channel
* Note velocity is linked to the 16 level attenuation of square tone 2
* The brightness of LED 2 is linked to the attenuation level of square tone 2

### Channel 3
* Controls square tone 3
* Pitch wheel is supported on this channel
* Note velocity is linked to the 16 level attenuation of square tone 3
* The brightness of LED 3 is linked to the attenuation level of square tone 3

### Channel 4
* Controls the noise generator
* Note velocity is linked to the 16 level attenuation of the 4th square tone
* The brightness of LED 4 is linked to the attenuation level of square tone 4
* MIDI CC 1 (mod wheel) on this channel controls the frequency of square tone
  3, which is used by notes `F4` and `C5` to control the noise shift rate

Unlike the other MIDI channels only 8 notes produce sound on this channel,
these control the different settings of the noise generator. The notes are:

 Note |                        Noise
------|-----------------------------------------------------
  C4  | Periodic noise, shift rate = clock speed (Hz) / 512
  D4  | Periodic noise, shift rate = clock speed (Hz) / 1024
  E4  | Periodic noise, shift rate = clock speed (Hz) / 2048
  F4  | Perioic noise, shift rate = Square tone 3 frequency
  G4  | White noise, shift rate = clock speed (Hz) / 512
  A4  | White noise, shift rate = clock speed (Hz) / 1024
  B4  | White noise, shift rate = clock speed (Hz) / 2048
  C5  | White noise, shift rate = Square tone 3 frequency

NOTE: If you use square tone 3 to modify the shift rate of the noise
channel you will interfere with any tones playing on channel 3.

## To Do
Here are some features I might look to add in the future:
* Attack and decay support (via CC control)
* 3 voice polyphony on a single channel
* Legato support
* Arpeggiator support
* Mod wheel controlled vibrato on square waves

## Credit
Initial inspiration for this project was taken from the (more advanced)
[CHIP_BASED_SYNTHESIZERS](https://github.com/brianmarkpeters/CHIP_BASED_SYNTHESIZERS/tree/master/QUAD%20SN76489%20SYNTH)
repository by Brian Peters.
