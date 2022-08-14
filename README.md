# Swamp-Cooler - Embedded Systems Semester Project
Code for swamp cooler embedded system written in C++ / ANSI C

This program can be used when hooked up to the specified hardware to run a swamp cooler type state 
machine that monitors / displays the environments temperature and humidity.  If the system detects 
that the temperature is over a set temperature then the fan will be triggered on and the state led 
will change.  If it is under the specified temperature then the system will stay in running state 
if and when the system has been turned on and therefore taken out of the disabled state.  A water 
level sensor is used to observe whether the water from the "swamp cooler" has gotten too low; if it 
has then the system is sent into an error state until the water level has been replenished.

Embedded System Components Include:
- Arduino Mega 2560
- Breadboard(s)
- Wires, LEDs, switches, pushbuttons
- Power supply module
- DC motor (fan)
- Stepper motor (vent angle)
- Water level sensor
- DHT sensor (Humidity & Temperature)
- LCD
- Real time Clock (RTC) module
