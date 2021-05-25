/*
Library for I2C (TWI) communication with display in Kewet/buddy electrical cars

Version 0.1
Author: Jens@2stroke4fun.dk
Code location: https://github.com/J-f-Jensen

Developed under the beer licens, e.g. if you like it you can send me a beer :-)
You can use this code without any restrictions as long as you keep this description
*/

#ifndef libKewetDisplay_h
#define libKewetDisplay_h

#include <Arduino.h>

// define the I2C bus address for SAA1064 (pin 1 to GND) ****  valid addresses HEX 70, 72, 74 and 76 for writing and 71, 73, 75 and 77 for reading
#define saaOddEcoSoc  0x74 >> 1 // Shared address for two SAA1064, switch between them by setting I2CaddBit1 and I2CaddBit2
#define saaAddTrip    0x72 >> 1
#define saaAddSpeedo1 0x70 >> 1
#define saaAddSpeedo2 0x76 >> 1

// Pins used
#define TripResetPin PA15
#define I2CaddBit1 PB3
#define I2CaddBit2 PB4

// control byte (dynamic mode on, digits 1+3 on, digits 2+4 on + segment current
enum displaySegmentConfigAndCurrent {
  d3mA = B00010111,
  d6mA = B00100111,
  d9mA = B00110111,
  d12mA = B01000111,
  d18mA = B01100111,
  d21mA = B01110111
};

//Target functions that you can write to
enum displayTarget { speedometer, odometer, socMeter, ecoMeter, tripCounter };

class libKewetDisplay
{
    public:
        //libKewetDisplay(int dontCare); // Not needed when classe not is initiatet with libKewetDisplay xx(); but libKewetDisplay xx;
        void initDisplay( displaySegmentConfigAndCurrent displayConfigValue );
        byte begin(void);
        byte write(int number, displayTarget targetValue);

    private:
        void _speedometer( byte kmh );
        void _odometer( int number );
        void _socMeter( byte soc );
        void _ecoMeter(byte eco );
        void _tripCounter( int number );
        
        byte digits[16]={63, 6, 91, 79, 102, 109, 125,7, 127, 111, 119, 124, 57, 94, 121, 0}; // these are the byte representations of pins required to display each digit 0~9 then A~E then blank=0
        byte ledStripBytes[8]={1,2,4,8,16,32,64,128}; // Used in LED strip functions speedometer
        byte saa1DigitNumberOrder[4]={0,2,1,3}; // Used in LED strip function speedometer
};

#endif
