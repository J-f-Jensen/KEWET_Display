

/*
extEEPROM.cpp needs to be modified if used on other hardware than AVR, rem out the line shown below:
byte extEEPROM::begin(twiClockFreq_t twiFreq)
{
    Wire.begin();
    //TWBR = ( (F_CPU / twiFreq) - 16) / 2; <- Rem out this line

*/

#include <Wire.h> // I2C bus library
#include <extEEPROM.h>    //https://github.com/JChristensen/extEEPROM
#include <Streaming.h>    //http://arduiniana.org/libraries/streaming/
//#include <SPI.h>
//#include <mcp_can.h>      //https://github.com/coryjfowler/MCP_CAN_lib
#include "libKewetDisplay.h"
#include <TinyGPS++.h>

// GPS configuration defines
//#define PMTK_CMD_HOT_START "$PMTK101*32"
//#define PMTK_CMD_WARM_START "PMTK102*31"
//#define PMTK_ENABLE_WAAS "$PMTK301,2*2E"
//#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
//#define PMTK_API_SET_FIX_CTL_5HZ  "$PMTK300,200,0,0,0,0*2F"
//#define PMTK_API_SET_FIX_CTL_1HZ  "$PMTK300,1000,0,0,0,0*1C"
//#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
//#define PMTK_SET_NMEA_OUTPUT_RMC "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"

// Chinese AT6558 GPS reciver configuration commands
#define PCAS_CMD_HOT_START "$PCAS10,0*1C"
//#define PCAS_CMD_FACTORY_RESTART "$PCAS10,3*1F"
#define PCAS_SET_BAUDRATE_115200 "$PCAS01,5*19"
#define PCAS_API_SET_FIX_CTL_5HZ "$PCAS02,200*1D"
#define PCAS_SET_NMEA_OUTPUT_RMC "$PCAS03,0,0,0,0,1,0,0,0,0,0,,,0,0*03"
//#define PCAS_SET_NMEA_OUTPUT_RMCGGA "$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02"
#define PCAS_SET_AUTOMOTIVE_MODE "$PCAS11,3*1E"


// Pins used by CAN
//#define CS_CAN_PIN 9
// SS                  10
// MOSI                11
// MISO                12
// SCK                 13

//Declare display
libKewetDisplay kewetDisplay;

// Declare eeprom on display module, eeprom is a ST device type M24C02 with 256 bytes (2Kbit)
extEEPROM displayEeprom(kbits_2, 1, 16,0x51);         //device size, number of devices, page size

//// Declare CAN module
//MCP_CAN CAN0(CS_CAN_PIN);

bool CAN_Status;

// CAN RX Variables
unsigned long rxID;
byte dlc;
byte rxBuf[8];

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
#define ss Serial1 //TX-PA9/RX-PA10 - Bluepill stm32f103
//  Serial2.begin(9600); //TX-PA2/RX-PA3 - Bluepill stm32f103
//  Serial3.begin(9600); //TX-PB10/RX-PB11 - Bluepill stm32f103
//  Serial.begin(9600);  //USB (PA11/PA12) - Bluepill stm32f103

// GPS Variables
int gpsRecivedCounts = 0;
int gpsLastSecond = 0;
bool firstLocationRecord = true;
float lastLat;
float lastLng;
float avgLat;
float avgLng;

// Display variables
int tripCounterValue = 0;
int odometerValue = 0;
byte ecoValue = 0;
byte socValue = 0;
byte speedValue = 0;


void setup()
{

  // Configure LED Pin
  pinMode(PC13,OUTPUT);
  digitalWrite(PC13,LOW); // Light the red led

//  //Initialize input ports
//  pinMode(MISO,INPUT);   // This should be default when loading the SPI module
//
//  //Initialize output ports
//  pinMode(MOSI,OUTPUT);  // This should be default when loading the SPI module
//  pinMode(SS,OUTPUT);    // This should be default when loading the SPI module
//  pinMode(CS_CAN_PIN,OUTPUT);

  Serial.begin(115200); // Debug - remove later
  
  uint8_t eepStatus = displayEeprom.begin(extEEPROM::twiClock100kHz);   //Init Wire and eeprom
  
  if (eepStatus) {
      Serial << endl << F("displayEeprom.begin() failed, status = ") << eepStatus << endl;
  }
  
  delay(500);

//  SPI.begin();
//  //SPI.setClockDivider(SPI_CLOCK_DIV8);
//
//  // Initialize MCP2515 running at 8MHz with a baudrate 500kbps, with std IDs
//  if(CAN0.begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK) CAN_Status = true;
//
//  if (CAN_Status) // Continue CAN configuration if CAN init is successful
//  {
////    // Set CAN message filters to only accept messages with ID 0x1806E5F4
////    CAN0.init_Mask(0,1,0x1FFFFFFF);                // Init first mask...
////    CAN0.init_Filt(0,1,0x1806E5F4);                // Init first filter...
//
//    CAN0.setMode(MCP_NORMAL); // Change to normal mode to allow messages to be transmittet
//  }

  // Start GPS communication and configure GPS
  ss.begin(9600);
  ss.println(PCAS_CMD_HOT_START);
  delay(1000);
  ss.println(PCAS_SET_BAUDRATE_115200);
  delay(100);
  ss.begin(115200);
  ss.flush();
  ss.println(PCAS_API_SET_FIX_CTL_5HZ);
  ss.println(PCAS_SET_NMEA_OUTPUT_RMC);
  ss.println(PCAS_SET_AUTOMOTIVE_MODE);
  delay(100); 

  // Start kewet dispaly
  kewetDisplay.begin();

  // Set all to zero until we have real data
  kewetDisplay.write(0, odometer);
  kewetDisplay.write(0, tripCounter);
  kewetDisplay.write(0, speedometer);
  kewetDisplay.write(0, socMeter);
  kewetDisplay.write(0, ecoMeter);
}

// For debug usage, 
const uint32_t totalKBytes = 2;         //for read and write test functions

//read test data (32-bit integers) from eeprom, "chunk" bytes at a time
void eeRead(uint8_t chunk)
{
    chunk &= 0xFC;                //force chunk to be a multiple of 4
    uint8_t data[chunk];
    uint32_t val = 0, testVal;
    Serial << F("Reading...") << endl;
    uint32_t msStart = millis();
    
    for (uint32_t addr = 0; addr < totalKBytes * 1024; addr += chunk) {
        if ( (addr &0xFFF) == 0 ) Serial << addr << endl;
        displayEeprom.read(addr, data, chunk);
        for (uint8_t c = 0; c < chunk; c += 4) {
            testVal =  ((uint32_t)data[c+0] << 24) + ((uint32_t)data[c+1] << 16) + ((uint32_t)data[c+2] << 8) + (uint32_t)data[c+3];
            if (testVal != val) Serial << F("Error @ addr ") << addr+c << F(" Expected ") << val << F(" Read ") << testVal << F(" 0x") << _HEX(testVal) << endl;
            ++val;
        }
    }
    uint32_t msLapse = millis() - msStart;
    Serial << "Last value: " << --val << " Read lapse: " << msLapse << " ms" << endl;
}

//dump eeprom contents, 16 bytes at a time.
//always dumps a multiple of 16 bytes.
void dump(uint32_t startAddr, uint32_t nBytes)
{
    Serial << endl << F("EEPROM DUMP 0x") << _HEX(startAddr) << F(" 0x") << _HEX(nBytes) << ' ' << startAddr << ' ' << nBytes << endl;
    uint32_t nRows = (nBytes + 15) >> 4;

    uint8_t d[16];
    for (uint32_t r = 0; r < nRows; r++) {
        uint32_t a = startAddr + 16 * r;
        displayEeprom.read(a, d, 16);
        Serial << "0x";
        if ( a < 16 * 16 * 16 ) Serial << '0';
        if ( a < 16 * 16 ) Serial << '0';
        if ( a < 16 ) Serial << '0';
        Serial << _HEX(a) << ' ';
        for ( int c = 0; c < 16; c++ ) {
            if ( d[c] < 16 ) Serial << '0';
            Serial << _HEX( d[c] ) << ( c == 7 ? "  " : " " );
        }
        Serial << endl;
    }
}

void loop()
{
   // Recive and display GPS data
  while (ss.available() > 0)
  {
    if (gps.encode(ss.read()))
    {
      displayInfo();
    }
  }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS detected: check wiring."));
    while(true);
  }

 //dump(0, 256); //Send alle 2 Kbit eeprom date to serial port
 //eeRead(2); //Send alle 2 Kbit eeprom date to serial port
}

 void displayInfo()
{ 
  digitalWrite(PC13,!digitalRead(PC13)); //Blink red led
  
  speedValue = gps.speed.isValid() ? gps.speed.kmph() : 0; // Set speed to zero if we don't recive a valid value

  if ( gps.speed.age() > 1500 ) // If age is higher that 1500 ms we have probably lost fix
  {
    speedValue = 0;
  }

  if (speedValue <= 2) { speedValue = 0; } // GPS data is not accurate at very low speed/standstill

  // Verify that GPS location is valid, location age is below 1500 ms and speed is higher than 3 Km/h, at lower speed the GPS signal wanders giving false readings.
  if (gps.location.isValid() && speedValue > 3 && gps.location.age() < 1500 )
  { 
    // We only sample distance every 11 sample
    if (gpsRecivedCounts < 10)
    {
      gpsRecivedCounts++;
//      avgLat += (float)gps.location.lat(); // Prep for average lat/lng values, if there is a problem with wandering
//      avgLng += (float)gps.location.lng();
    }
    else
    {
      if (firstLocationRecord)
      {
        lastLat = (float) gps.location.lat();
        lastLng = (float) gps.location.lng();
        firstLocationRecord = false;
      }
      else
      {
        tripCounterValue+=(int) gps.distanceBetween( (float)gps.location.lat(), (float)gps.location.lng(), (float)lastLat, (float)lastLng) / 10;
        lastLat = (float) gps.location.lat();
        lastLng = (float) gps.location.lng();

        gpsRecivedCounts = 0;
      }
    }
  }

  if (gpsRecivedCounts == 0) // Only update every ~two secounds 
  {
    kewetDisplay.write(odometerValue, odometer);
    kewetDisplay.write(tripCounterValue/10, tripCounter);
    kewetDisplay.write(socValue, socMeter);
  }
  
  kewetDisplay.write(speedValue, speedometer);
  kewetDisplay.write(ecoValue, ecoMeter);

  Serial.print(F(" Speed: "));
  Serial.print( speedValue );
  Serial.print(F("Km/h "));

  Serial.print(F(" Trip: "));
  Serial.print(tripCounterValue*10);
  Serial.print(F(" meter "));

  Serial.print(F("  "));
  if (gps.time.isValid())
  {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());

    Serial.print(F("  Location age:"));
    Serial.print(gps.location.age());

    Serial.print(F(" Lat/Lng:"));
    Serial.print(gps.location.lat());
    Serial.print(F("/"));
    Serial.print(gps.location.lng());
  }

  Serial.println();
}
