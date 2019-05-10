# multifunctionCayenne
making the Arduino multi-function board available on Cayenne
## Contents of this repoitory
The multi-function board features
* 3 user push buttons which can be read out
* 4 user LEDs
* a 4 digit 7-segemnt display
* a passive buzzer
* a potentiometer connected to the Arduino 10 bit ADC
* an optional LM35 analog thermometer
* an optional DS18B20 digital thermometer

The LM35 and the DS18B20 are exclusiv, only one of the two can be used at a time.
The repository consists of two parts: the *driver* part with programs testing hardware access to the above sensors and actuators and a *cayenne* part which implements a Cayenne client reading out the sensors and sending the results to myDevices Cayenne or it takes commands from Cayenne widgets and drives the corresponding actuators. 

## Hardware access tests
This part has programs showing how to drive the LEDs, how to read out the switches, how to drive the 7-segment displays and how to read out the potentiometer voltage.

The most tricky bit is the 7-segment display which needs a period update of its digits every ~ 25 ms in order to produce a steady display. This is accomplished using a timer triggering an interrupt service routine every 8 ms which refreshes one digit at a time.
## The Cayenne Client
