# EE3102-Microcontroller
Source files for Microcontroller code used in EE3102 (Fall 2021) At the University of Minnesota. This works by taking in digital voltage data 
(from an ADC chip communicated over SPI), extracts the voltage data from the 32-bit chunk sent over, converts this voltage value relative to a current output of a photodiode,
and then displays this value (between 0 and 5 microAmps) on an LCD screen using I2C

The microcontroller used was a Microchip PIC24FV32KA302, and the pinout is shown in ![Screenshot of Pinout](./pic24fv_pinout.PNG) <br />
This code is not necessarily meant to be reused, but this repo is for archival purposes. main.c details the everything that is done (in order) in the comments.
This microcontroller was used in combination with a custom build PCB (that I did not design).
