# KEWET_Display

The example KEWET_Display.ino file is tested with a GPS module as input for the speedometer running on a bluepile stm32f103 board. This is used due to the low cost and number of serial ports allowing both debug and GPS via serial.

Please see picture regarding how to connect the display to the controller.
6 pins is as minimum needed:

/+ (Plus use same voltage as controller)
GND
SCL (I2C SCL pin defined for the controller used via the wire library)
SDA (I2C SDA pin defined for the controller used via the wire library)
I2CAdd1 PB3 (Defined in libKewetDisplay.h)
I2CAdd2 PB4 (Defined in libKewetDisplay.h)


Optional:
TripResetPin PA15 (Defined in libKewetDisplay.h)
"analog light out" pin can be connected to a analog pin and be used to control the brightness of the display by the ma setting. 
If "analog light out" is used +/-8 volt have to be supplied

I have tested both 3.3 and 5 volt controllers and the display works great with both voltages.

You can dump all values from the eeprom on the display using the dump or eeRead command. See in the code how to use them.

Libraries needed to run the example, beside the one in the folder is documented in the ino file. If you use a bluepile stm32f103 board then please read how to modify the eeprom library in the ino file
 
