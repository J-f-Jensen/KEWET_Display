/*
Library for I2C (TWI) communication with display in Kewet/buddy electrical cars

Version 0.1
Author: Jens@2stroke4fun.dk
Code location: https://github.com/J-f-Jensen

Developed under the beer licens, e.g. if you like it you can send me a beer :-)
You can use this code without any restrictions as long as you keep this description
*/

#include "libKewetDisplay.h"
#include <Wire.h> // I2C bus library

//libKewetDisplay::libKewetDisplay(int dontCare)
//{
//}

byte libKewetDisplay::begin(void)
{
  //Initialize input ports
  pinMode(TripResetPin, INPUT);
  
  //Initialize output ports
  pinMode(I2CaddBit1, OUTPUT);
  pinMode(I2CaddBit2, OUTPUT);

  digitalWrite(I2CaddBit1,HIGH);
  digitalWrite(I2CaddBit2,HIGH);

  Wire.begin(); // start up I2C bus

  initDisplay( d3mA );

  return 0;
  
}

byte libKewetDisplay::write(int number, displayTarget targetValue)
{
  switch(targetValue) {
   case speedometer   :
      _speedometer((byte)number);
      break;
      
   case odometer  :
      _odometer( number);
      break;

   case socMeter  :
      _socMeter((byte)number);
      break;

   case ecoMeter  :
      _ecoMeter((byte)number);
      break;

   case tripCounter  :
      _tripCounter(number);
      break;
  }
  
  return 0;
}

void libKewetDisplay::initDisplay(displaySegmentConfigAndCurrent displayConfigValue )
{
 digitalWrite(I2CaddBit1,LOW);
 digitalWrite(I2CaddBit2,HIGH);
 
 Wire.beginTransmission(saaOddEcoSoc);
 Wire.write(B00000000); // this is the instruction byte. Zero means the next byte is the control byte
 Wire.write(displayConfigValue); // control byte
 Wire.endTransmission();

 digitalWrite(I2CaddBit1,HIGH);
 digitalWrite(I2CaddBit2,LOW);
 
 Wire.beginTransmission(saaOddEcoSoc);
 Wire.write(B00000000); // this is the instruction byte. Zero means the next byte is the control byte
 Wire.write(displayConfigValue); // control byte 
 Wire.endTransmission();

 digitalWrite(I2CaddBit1,HIGH);
 digitalWrite(I2CaddBit2,HIGH);

 Wire.beginTransmission(saaAddTrip);
 Wire.write(B00000000); // this is the instruction byte. Zero means the next byte is the control byte
 Wire.write(displayConfigValue); // control byte
 Wire.endTransmission();

 Wire.beginTransmission(saaAddSpeedo1);
 Wire.write(B00000000); // this is the instruction byte. Zero means the next byte is the control byte
 Wire.write(displayConfigValue); // control byte 
 Wire.endTransmission();
 
 Wire.beginTransmission(saaAddSpeedo2);
 Wire.write(B00000000); // this is the instruction byte. Zero means the next byte is the control byte
 Wire.write(displayConfigValue); // control byte 
 Wire.endTransmission();
}

void libKewetDisplay::_speedometer( byte kmh )
{
  digitalWrite(I2CaddBit1,HIGH);
  digitalWrite(I2CaddBit2,HIGH);
  
  if (kmh > 100)
  {
    kmh=100;
  }
  
  byte digitNumber = kmh/16;
  byte digitValue = (kmh%16)/2;
  
  Wire.beginTransmission(saaAddSpeedo1);
  Wire.write(1); // instruction byte - first digit to control is 1 (right hand side)
  for (int z=0; z<4; z++)
  {
    if (z == saa1DigitNumberOrder[digitNumber] && digitNumber < 4)
    {
      Wire.write(ledStripBytes[digitValue]);
    }
    else
    {
      Wire.write(0);
    }
  }
  Wire.endTransmission();

  Wire.beginTransmission(saaAddSpeedo2);
  Wire.write(1); // instruction byte - first digit to control is 1 (right hand side)
  for (int z=0; z<4; z++)
  {
    if (z == saa1DigitNumberOrder[digitNumber-4] && digitNumber > 3)
    {
      Wire.write(ledStripBytes[digitValue]);
    }
    else
    {
      Wire.write(0);
    }
  }
  Wire.endTransmission();
}

void libKewetDisplay::_odometer(int number)
{
  digitalWrite(I2CaddBit1,LOW);
  digitalWrite(I2CaddBit2,HIGH);

  byte odoDigitArray[5];
  
  for (int i=0; i < 5; i++)
  {
    byte character = number % 10; //get the value of the rightmost digit
    if (number == 0 && i > 0) character = 0xf;
    odoDigitArray[i] = character;
    number = number/10;
  }
 
  Wire.beginTransmission(saaOddEcoSoc);
  Wire.write(1); // instruction byte - first digit to control is 1 (right hand side)
  Wire.write(digits[odoDigitArray[4]]); // digit 5 odmeter (LHS) Highest digit
  Wire.write(digits[odoDigitArray[3]]); // digit 4
  Wire.write(digits[odoDigitArray[2]]); // digit 3
  Wire.write(digits[odoDigitArray[1]]); // digit 2 
  Wire.endTransmission();
  
  digitalWrite(I2CaddBit1,HIGH);
  digitalWrite(I2CaddBit2,LOW);
  
  Wire.beginTransmission(saaOddEcoSoc);
  Wire.write(1); // instruction byte - first digit to control is 1 (right hand side)
  Wire.write(digits[odoDigitArray[0]]); // digit 1 odmeter (RHS) Lowest digit
  Wire.endTransmission();
  
  digitalWrite(I2CaddBit1,HIGH);
  digitalWrite(I2CaddBit2,HIGH);
}

void libKewetDisplay::_socMeter(byte soc)
{
 digitalWrite(I2CaddBit1,HIGH);
 digitalWrite(I2CaddBit2,LOW);

 byte b = 0;
 byte socArray[2]={0,0};
 
 if (soc > 100) { soc = 100; }

 if (soc <= 50)
 {
  b = soc*10/63;
  socArray[0] = ledStripBytes[b];
 }
 else
 {
  b = (soc*10-500)/63;
  socArray[1] = ledStripBytes[b];
 }
 
 Wire.beginTransmission(saaOddEcoSoc);
 Wire.write(2); // instruction byte - first digit to control is 1 (right hand side)
 Wire.write(socArray[1]);   // SOC HIGH range
 Wire.write(socArray[0]);   // SOC LOW range
 Wire.endTransmission();

 digitalWrite(I2CaddBit1,HIGH);
 digitalWrite(I2CaddBit2,HIGH);
}

void libKewetDisplay::_ecoMeter(byte eco )
{
 digitalWrite(I2CaddBit1,HIGH);
 digitalWrite(I2CaddBit2,LOW);

 if (eco > 3) { eco = 3; }
 if (eco == 3) { eco = 4; }
 
 Wire.beginTransmission(saaOddEcoSoc);
 Wire.write(4); // instruction byte - first digit to control is 1 (right hand side)
 Wire.write(eco);           // ECO meter (1 = green, 2 = yellow, 3 = red)
 Wire.endTransmission();

 digitalWrite(I2CaddBit1,HIGH);
 digitalWrite(I2CaddBit2,HIGH);
}

void libKewetDisplay::_tripCounter (int number)
{
  //Ensure I2C addresse bits are correct
  digitalWrite(I2CaddBit1,HIGH);
  digitalWrite(I2CaddBit2,HIGH);

  byte TripDigitArray[4];
  int tempNumber = number;
  
  for (int i=0; i < 4; i++)
  {
    byte character = number % 10; //get the value of the rightmost digit
    if (number == 0 && i > 0) character = 0xf;
    TripDigitArray[i] = character;
    number = number/10;
  }

  Wire.beginTransmission( saaAddTrip );
  Wire.write(1); // instruction byte - first digit to control is 1 (right hand side)
  Wire.write(digits[TripDigitArray[3]]); // digit 1 (LHS)
  Wire.write(digits[TripDigitArray[2]]); // digit 2
  if (tempNumber < 10) // If number is below 10 then write a zero in front of the digit sign
  {
    Wire.write(digits[0]+128); // digit 3 with decimal point
  }
  else 
  {
    Wire.write(digits[TripDigitArray[1]]+128); // digit 3 with decimal point
  }
  Wire.write(digits[TripDigitArray[0]]); // digit 4 (RHS)
  Wire.endTransmission();
}
