// LCD digit mapping stuff
const uint8_t truthTable[11] = {    // 7-segment mapping truth table to maskDigitsABCD with A B C D
  0b0111111, //0
  0b0000110, //1
  0b1011011, //2
  0b1001111, //3
  0b1100110, //4
  0b1101101, //5
  0b1111101, //6
  0b0000111, //7
  0b1111111, //8
  0b1101111, //9
  0b1000000, //-
};
const uint64_t digitA[7] = {0x8000000000, 0x2000000000, 0x4000000000, 0x0000000010, 0x0000000040, 0x0000000080, 0x0000000020};
const uint64_t digitB[7] = {0x0800000000, 0x0200000000, 0x0400000000, 0x0000000100, 0x0000000400, 0x0000000800, 0x0000000200};
const uint64_t digitC[7] = {0x0080000000, 0x0020000000, 0x0040000000, 0x0000001000, 0x0000004000, 0x0000008000, 0x0000002000};
const uint64_t digitD[7] = {0x0008000000, 0x0002000000, 0x0004000000, 0x0000100000, 0x0000400000, 0x0000800000, 0x0000200000};
const uint64_t dpABCD = 0x0000020000;
const uint64_t digit0[7] = {0x0000080000000000, 0x0000040000000000, 0x0000020000000000, 0x0000010000000000, 0x0000000100000000, 0x0000002000000000, 0x0000001000000000};
const uint64_t digit1[7] = {0x0008000000000000, 0x0004000000000000, 0x0002000000000000, 0x0001000000000000, 0x0000000001000000, 0x0000800000000000, 0x0000400000000000};
const uint64_t digit2[7] = {0x0080000000000000, 0x0040000000000000, 0x0020000000000000, 0x0010000000000000, 0x0000000000000100, 0x0000200000000000, 0x0000100000000000};
const uint64_t digit3[7] = {0x0800000000000000, 0x0400000000000000, 0x0200000000000000, 0x0100000000000000, 0x0000000000000001, 0x0000008000000000, 0x0000004000000000};
const uint64_t maskDigits0123 = 0xffffff101010101;
const uint64_t maskDigitsABCD = 0xeeeef2fff0;   // Defining maskDigitsABCD to "clean" only bits where LCD digits + dp are located
const uint64_t timeSep = 0x0000000000010000;

// I2C bus toward the Philips PCF8576 LCD driver, as a master with pullup
#include <SoftI2C.h>
SoftI2C SoftWire = SoftI2C(4,5, true);  //sda, scl
const uint8_t I2C_ADDRESS = 0x38;       // PCF8576 address

// I2C bus inbound from cluster's microchip
#include <Wire.h>
uint8_t bytesCmd[5];      
uint8_t bytesData[20];    // Should not be greater, 40x4 bits of RAM on driver = 20x8 bits = 20 bytes
uint8_t bytesDataLength, bytesCmdLength = 0;
bool updateData = false;
#define DATABYTE1_LCD1  0   // Digit data byte 1 stored in i2c message byte 0
#define DATABYTE2_LCD1  1   // Digit data byte 2 stored in i2c message byte 1
#define DATABYTE3_LCD1  3   // Digit data byte 3 stored in i2c message byte 3
#define DATABYTE4_LCD1  4   // Digit data byte 4 stored in i2c message byte 4
#define DATABYTE5_LCD1  5   // Digit data byte 5 stored in i2c message byte 5 - Same as DATABYTE1_LCD2
#define DATABYTE1_LCD2  5   // Same as DATABYTE5_LCD1
#define DATABYTE2_LCD2  6
#define DATABYTE3_LCD2  7
#define DATABYTE4_LCD2  8
#define DATABYTE5_LCD2  9
#define DATABYTE6_LCD2  10
#define DATABYTE7_LCD2  11
#define DATABYTE8_LCD2  13
#define DATABYTESEL_SPEEDMODE 2 // Speed mode indicator (km/h and mph bits) are stored on i2c message byte 2
#define DATABYTESEL_CLOCKMODE 10 // Clock mode indicator (time separator pixel) at byte 10
uint8_t tempDataLcds[20];
const uint8_t maskKPH = 0x05;
const uint8_t maskMPH = 0x08;
const uint8_t maskClock = 0x01;
uint8_t speedMode = 0x00; // 0=off - 1=kph - 2=mph
bool clockMode = false;

// GPS module  https://github.com/Jcabza008/UBLOX/
#include "UBLOX.h"
#define gpsTimeout 1500
unsigned long lastGpsRead = millis();
#define GPS_SERIAL Serial
#define GPS_DEFAULT_BAUDRATE 9600L
#define GPS_WANTED_BAUDRATE 115200L
UBLOX gps(GPS_SERIAL,GPS_WANTED_BAUDRATE);
bool gpsSetupDone = false;
float gpsSpeed = 0;

// Timezone 
#include <Timezone.h> // https://github.com/JChristensen/Timezone
TimeChangeRule timeSummer = {"sumer", Last, Sun, Mar, 2, 120};
TimeChangeRule timeWinter = {"wintr", Last, Sun, Oct, 2, 60};
Timezone timezoneFrance(timeSummer, timeWinter);
TimeChangeRule *tcr;
time_t timedate = now();
bool validTime = false;

void setup() {
  Wire.begin(I2C_ADDRESS);        // Joining cluster's i2c bus
  Wire.onReceive(receiveEvent);   // Registering event for receiving data
  
  // Begin GPS
  delay(5000);  // Leave 2 seconds for the GPS to boot
  setupGPS(); 
  GPS_SERIAL.begin(GPS_WANTED_BAUDRATE); 
  gps.begin(); 
  
  // Starting i2c bus with LCD driver
  SoftWire.begin();  

  // Builtin LED will be used for debugging
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  // If clock mode is on
  if (clockMode) {
    // Getting time from GPS, flagging for update if time has changed
    updateData |= getTime();

    // Masking numbers on LCD 2
    //writeTime(gpsHour, gpsMinute, gpsSecond);
    writeTime();
  }else{
    // Reseting to dumb value
    //gpsHour = UINT8_MAX;
    //gpsMinute = UINT8_MAX;
    timedate = 0;
  }

  // If speed mode is on
  if (speedMode) {
    // Getting speed from GPS, flagging for update if it has changed
    updateData |= getSpeed();

    // Masking numbers on the byte array for LCD 1 (left)
    writeSpeed(gpsSpeed);
    
  } else {
    // Reseting to dumb value
    gpsSpeed = UINT16_MAX;
  }

  // If there is new data to display
  if(updateData){
    // Debug
    digitalWrite(LED_BUILTIN, HIGH);

    // Flipping updateData back to false
    updateData = false;

    // Sending update to LCD driver's RAM registers
    updateLCD();

    // Turning the LED OFF
    digitalWrite(LED_BUILTIN, LOW);
  }
}

bool getSpeed(){
  // Saving previous speed in memory
  float oldSpeed = gpsSpeed;

  if(gps.readSensor()) { // If GPS is available
    if (gps.isGnssFixOk()){  // If GPS has fix
      if(speedMode == 1){
        gpsSpeed = gps.getGroundSpeed_ms() * 36;
      }else{
        gpsSpeed = (gps.getGroundSpeed_fps() / 1.467) * 10;
      }

      // We have a speed with a fix, updating last GPS read timestamp, returning 0
      lastGpsRead = millis();
    }else{   // GPS is available BUT has no fix
      gpsSpeed = UINT16_MAX;
      return true;
    }
  }
  else // GPS isn't available right now but are we within timeout specs?
  {
    if(millis() - lastGpsRead > gpsTimeout){  // Nope... 
      gpsSpeed = UINT16_MAX;
      return true;
    }
  }

  // If is in speed mode and new speed has changed, return yes
  return (gpsSpeed != oldSpeed && speedMode > 0);
}

bool getTime(){
  // Saving previous speed in memory
  time_t oldTimedate = timedate;

  if(gps.readSensor()) { // If GPS is available
    if (gps.isValidTime()){  // If GPS has a valid time
      // Setting local time from GPS UTC time
      setTime(gps.getHour(), gps.getMin(), gps.getSec(), gps.getDay(), gps.getMonth(), gps.getYear());

      // Converting to local timezone with STD/EST and updating time
      timedate = timezoneFrance.toLocal(now(), &tcr);
      setTime(timedate);

      // We have a valid time, updating last GPS read timestamp
      lastGpsRead = millis();
      validTime = true;
    }else{   // GPS is available BUT has no valid time
      timedate = 0;
      validTime = false;
      return true;
    }
  }
  else // GPS isn't available right now but are we within timeout specs?
  {
    if(millis() - lastGpsRead > gpsTimeout){  // Nope... 
      timedate = 0;
      validTime = false;
      return true;
    }
  }

  // If is in speed mode and new speed has changed, return yes
  return (timedate != oldTimedate) && clockMode;
}

void receiveEvent(int howMany) { 
  // Variables
  bool cmdSkip = true;
  //bool newTempData = false;
  uint8_t _bytesDataLength = 0;
  uint8_t _bytesCmdLength = 0;

  // Nothing updated yet
  updateData = false;

  // Storing command bytes
  while(Wire.available() && cmdSkip) { 
    uint8_t c = Wire.read();                                // Reading byte from bus
    cmdSkip = c >= 0x80;                                    // Checking if this is the last command
    if(bytesCmd[_bytesCmdLength] != c) updateData = true;   // If new data differs from stored data, marking the data as new
    bytesCmd[_bytesCmdLength] = c;                          // Storing command byte
    _bytesCmdLength++;                                      // Incrementing array index
  }
  bytesCmdLength = _bytesCmdLength;

  // Storing data
  while(Wire.available()){
    // Storing the first X bytes in a temporary array
    uint8_t c = Wire.read();
    
    // Is new byte different than the previous one, needing a display update?
    if(tempDataLcds[_bytesDataLength] != c) updateData = true;

    // Storing new byte in temporary array
    tempDataLcds[_bytesDataLength] = c;
    
    // Incrementing index
    _bytesDataLength++;
  }
  
  // Updating array length
  bytesDataLength = _bytesDataLength;

  // Is speed mode on?
  speedMode = tempDataLcds[DATABYTESEL_SPEEDMODE] == maskMPH ? 2 : (tempDataLcds[DATABYTESEL_SPEEDMODE] == maskKPH) ? 1 : 0;

  // Is clock mode on?
  clockMode = ((tempDataLcds[DATABYTESEL_CLOCKMODE] & maskClock) == maskClock)
    && tempDataLcds[DATABYTESEL_CLOCKMODE] != 0xff
    && tempDataLcds[DATABYTESEL_CLOCKMODE - 1] != 0xff
    && tempDataLcds[DATABYTESEL_CLOCKMODE + 1] != 0xff
    && tempDataLcds[DATABYTESEL_CLOCKMODE - 2] != 0xff
    && tempDataLcds[DATABYTESEL_CLOCKMODE + 2] != 0xff;

  // Merging data bytes
  for(uint8_t i = 0; i < bytesDataLength; i++){
    switch(i) {
      case DATABYTE1_LCD1: bytesData[i] = speedMode ? (tempDataLcds[i] & ((~maskDigitsABCD & 0xff00000000) >> 32))                 + (bytesData[i] & (maskDigitsABCD & 0xff00000000) >> 32)        : tempDataLcds[i]; break;
      case DATABYTE2_LCD1: bytesData[i] = speedMode ? (tempDataLcds[i] & ((~maskDigitsABCD & 0x00ff000000) >> 24))                 + (bytesData[i] & (maskDigitsABCD & 0x00ff000000) >> 24)        : tempDataLcds[i]; break;
      case DATABYTE3_LCD1: bytesData[i] = speedMode ? (tempDataLcds[i] & ((~maskDigitsABCD & 0x0000ff0000) >> 16))                 + (bytesData[i] & (maskDigitsABCD & 0x0000ff0000) >> 16)        : tempDataLcds[i]; break;
      case DATABYTE4_LCD1: bytesData[i] = speedMode ? (tempDataLcds[i] & ((~maskDigitsABCD & 0x000000ff00) >> 8))                  + (bytesData[i] & (maskDigitsABCD & 0x000000ff00) >> 8)         : tempDataLcds[i]; break;
      case DATABYTE5_LCD1: 
        //bytesData[i] = speedMode ?                   (tempDataLcds[i] & ((~maskDigitsABCD & 0x00000000ff)))                       + (bytesData[i] & (maskDigitsABCD & 0x00000000ff))              : tempDataLcds[i];
        //bytesData[i] = (tempDataLcds[i] & ~( (maskDigitsABCD & 0x00000000ff) |  ((maskDigits0123 & 0xff00000000000000) >> 56) ) )     + (bytesData[i] & ((maskDigitsABCD & 0x00000000ff) | (maskDigits0123 & 0xff00000000000000) >> 56) )             : (tempDataLcds[i] & ((~maskDigits0123 & 0xff00000000000000) >> 56)) + (bytesData[i] & (maskDigits0123 & 0xff00000000000000) >> 56);
        //bytesData[i] =                               (tempDataLcds[i] & ((~maskDigits0123 & 0xff00000000000000) >> 56)) + (bytesData[i] & (maskDigits0123 & 0xff00000000000000) >> 56);
        if (speedMode && clockMode) {
          bytesData[i] = (tempDataLcds[i] & ~( (maskDigitsABCD & 0x00000000ff) |  ((maskDigits0123 & 0xff00000000000000) >> 56) ) )     + (bytesData[i] & ((maskDigitsABCD & 0x00000000ff) | (maskDigits0123 & 0xff00000000000000) >> 56) );
        }else if(speedMode && !clockMode){
          bytesData[i] = (tempDataLcds[i] & ((~maskDigitsABCD & 0x00000000ff)))                       + (bytesData[i] & (maskDigitsABCD & 0x00000000ff));
        }else if(!speedMode && clockMode){
          bytesData[i] = (tempDataLcds[i] & ((~maskDigits0123 & 0xff00000000000000) >> 56)) + (bytesData[i] & (maskDigits0123 & 0xff00000000000000) >> 56);
        }else{
          bytesData[i] = tempDataLcds[i];
        }
         break;
      case DATABYTE2_LCD2: bytesData[i] = clockMode ? (tempDataLcds[i] & ((~maskDigits0123 & 0x00ff000000000000) >> 48)) + (bytesData[i] & (maskDigits0123 & 0x00ff000000000000) >> 48): tempDataLcds[i]; break;
      case DATABYTE3_LCD2: bytesData[i] = clockMode ? (tempDataLcds[i] & ((~maskDigits0123 & 0x0000ff0000000000) >> 40)) + (bytesData[i] & (maskDigits0123 & 0x0000ff0000000000) >> 40): tempDataLcds[i]; break;
      case DATABYTE4_LCD2: bytesData[i] = clockMode ? (tempDataLcds[i] & ((~maskDigits0123 & 0x000000ff00000000) >> 32)) + (bytesData[i] & (maskDigits0123 & 0x000000ff00000000) >> 32): tempDataLcds[i]; break;
      case DATABYTE5_LCD2: bytesData[i] = clockMode ? (tempDataLcds[i] & ((~maskDigits0123 & 0x00000000ff000000) >> 24)) + (bytesData[i] & (maskDigits0123 & 0x00000000ff000000) >> 24): tempDataLcds[i]; break;
      case DATABYTE6_LCD2: bytesData[i] = clockMode ? (tempDataLcds[i] & ((~maskDigits0123 & 0x0000000000ff0000) >> 16)) + (bytesData[i] & (maskDigits0123 & 0x0000000000ff0000) >> 16): tempDataLcds[i]; break;
      case DATABYTE7_LCD2: bytesData[i] = clockMode ? (tempDataLcds[i] & ((~maskDigits0123 & 0x000000000000ff00) >> 8))  + (bytesData[i] & (maskDigits0123 & 0x000000000000ff00) >> 8): tempDataLcds[i]; break;
      case DATABYTE8_LCD2: bytesData[i] = clockMode ? (tempDataLcds[i] & ((~maskDigits0123 & 0x00000000000000ff)))       + (bytesData[i] & (maskDigits0123 & 0x00000000000000ff)): tempDataLcds[i]; break;

      default: bytesData[i] = tempDataLcds[i]; break;
    }
  }
}

uint64_t getLCDMask(){
  uint64_t temp = 0;
    
    for (uint8_t i = 0; i < 7; i++) {
       temp += digitA[i];
       temp += digitB[i];
       temp += digitC[i];
       temp += digitD[i];
    }
    temp += dpABCD;
    return temp;
}

void updateLCD(){
  SoftWire.beginTransmission(I2C_ADDRESS);      // Opening I2C bus as master
  SoftWire.write(bytesCmd, bytesCmdLength);     // Sending configuration commands
  SoftWire.write(bytesData, bytesDataLength);   // Sending data
  SoftWire.endTransmission();                   // Closing bus
}

void writeSpeed(uint16_t data){
  // Masking data bytes (cleaning the digits only, leaving the rest)
  bytesData[DATABYTE1_LCD1] &= (~maskDigitsABCD & 0xff00000000) >> 32;
  bytesData[DATABYTE2_LCD1] &= (~maskDigitsABCD & 0x00ff000000) >> 24;
  bytesData[DATABYTE3_LCD1] &= (~maskDigitsABCD & 0x0000ff0000) >> 16;
  bytesData[DATABYTE4_LCD1] &= (~maskDigitsABCD & 0x000000ff00) >> 8;
  bytesData[DATABYTE5_LCD1] &= (~maskDigitsABCD & 0x00000000ff);

  // If data == UINT16_MAX then it's an invalid data
  // Building the 4 digits from the float argument
  uint8_t dataA = data == UINT16_MAX ? 0x0A : ((uint16_t)data % 10000) / 1000;
  uint8_t dataB = data == UINT16_MAX ? 0x0A : ((uint16_t)data % 1000) / 100;
  uint8_t dataC = data == UINT16_MAX ? 0x0A : ((uint16_t)data % 100) / 10;
  uint8_t dataD = data == UINT16_MAX ? 0x0A : ((uint16_t)data % 10);
  
  
  // For each digit, flipping segments bits to according to each digit that must be displayed
  uint64_t temp = 0;
  for (uint8_t i = 0; i < 7; i++) {
    if (((truthTable[dataA] >> i) & 0b1) && dataA != 0) temp += digitA[i];
    if (((truthTable[dataB] >> i) & 0b1) && (dataB != 0 || dataA != 0)) temp += digitB[i];
    if ((truthTable[dataC] >> i) & 0b1) temp += digitC[i];
    if ((truthTable[dataD] >> i) & 0b1) temp += digitD[i];
  }
  temp += dpABCD;   // Flipping the decimal point

  // Applying OR maskDigitsABCD on the data bytes, for each byte
  bytesData[DATABYTE1_LCD1] |= (temp & 0xff00000000) >> 32;
  bytesData[DATABYTE2_LCD1] |= (temp & 0x00ff000000) >> 24;
  bytesData[DATABYTE3_LCD1] |= (temp & 0x0000ff0000) >> 16;
  bytesData[DATABYTE4_LCD1] |= (temp & 0x000000ff00) >> 8;
  bytesData[DATABYTE5_LCD1] |= (temp & 0x00000000ff);
}

void writeTime(){
  // Masking data bytes (cleaning the digits only, leaving the rest)
  bytesData[DATABYTE1_LCD2] &=  (~maskDigits0123 & 0xff00000000000000) >> 56;
  bytesData[DATABYTE2_LCD2] &=  (~maskDigits0123 & 0x00ff000000000000) >> 48;
  bytesData[DATABYTE3_LCD2] &=  (~maskDigits0123 & 0x0000ff0000000000) >> 40;
  bytesData[DATABYTE4_LCD2] &=  (~maskDigits0123 & 0x000000ff00000000) >> 32;
  bytesData[DATABYTE5_LCD2] &=  (~maskDigits0123 & 0x00000000ff000000) >> 24;
  bytesData[DATABYTE6_LCD2] &= (~maskDigits0123 & 0x0000000000ff0000) >> 16;
  bytesData[DATABYTE7_LCD2] &= (~maskDigits0123 & 0x000000000000ff00) >> 8;
  bytesData[DATABYTE8_LCD2] &= (~maskDigits0123 & 0x00000000000000ff);

  // If validTime is false then it's an invalid data
  // Building the 4 digits from the time information
  uint8_t dataA = !validTime ? 0x0A : ((uint16_t)hour() % 100) / 10;
  uint8_t dataB = !validTime ? 0x0A : ((uint16_t)hour() % 10);
  uint8_t dataC = !validTime ? 0x0A : ((uint16_t)minute() % 100) / 10;
  uint8_t dataD = !validTime ? 0x0A : ((uint16_t)minute() % 10);
  
  // For each digit, flipping segments bits to according to each digit that must be displayed
  uint64_t temp = 0;
  for (uint8_t i = 0; i < 7; i++) {
    if ((truthTable[dataA] >> i) & 0b1 && dataA != 0) temp += digit0[i];
    if ((truthTable[dataB] >> i) & 0b1) temp += digit1[i];
    if ((truthTable[dataC] >> i) & 0b1) temp += digit2[i];
    if ((truthTable[dataD] >> i) & 0b1) temp += digit3[i];
  }
  
  // Flipping the time separation pixel for even seconds
  if (second() % 2) temp += timeSep;

  // Applying OR maskDigitsABCD on the data bytes, for each byte
  bytesData[DATABYTE1_LCD2] |=  (temp & 0xff00000000000000) >> 56;
  bytesData[DATABYTE2_LCD2] |=  (temp & 0x00ff000000000000) >> 48;
  bytesData[DATABYTE3_LCD2] |=  (temp & 0x0000ff0000000000) >> 40;
  bytesData[DATABYTE4_LCD2] |=  (temp & 0x000000ff00000000) >> 32;
  bytesData[DATABYTE5_LCD2] |=  (temp & 0x00000000ff000000) >> 24;
  bytesData[DATABYTE6_LCD2] |= (temp & 0x0000000000ff0000) >> 16;
  bytesData[DATABYTE7_LCD2] |= (temp & 0x000000000000ff00) >> 8;
  bytesData[DATABYTE8_LCD2] |= (temp & 0x00000000000000ff);
}

