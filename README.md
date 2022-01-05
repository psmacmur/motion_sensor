# motion_sensor
Arduino sketch for an automatic night light.

 * Reads output from a PIR Mini Motion Detector Sensor - 12mm Diameter - AM312 Chip 
 * Lights a WS-2812B light strip when it detects motion & stays lit for 30 sec after last motion
 * Fades light out after timeout so you can wave before the room goes dark

## Notes
* I use Pin 7 for detection as it is the closest interruptable PIN to where I put the sensor. 
If you use a development board other than a Leonardo / Pro Micro, you might need to use a different pin (e.g. 2).