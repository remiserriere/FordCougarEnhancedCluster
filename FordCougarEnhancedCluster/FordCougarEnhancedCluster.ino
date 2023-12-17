// LCD digit mapping stuff
const uint8_t truthTable[11] = {    // 7-segment mapping truth table to mask with A B C D
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
const uint64_t A[7] = {0x8000000000, 0x2000000000, 0x4000000000, 0x0000000010, 0x0000000040, 0x0000000080, 0x0000000020};
const uint64_t B[7] = {0x0800000000, 0x0200000000, 0x0400000000, 0x0000000100, 0x0000000400, 0x0000000800, 0x0000000200};
const uint64_t C[7] = {0x0080000000, 0x0020000000, 0x0040000000, 0x0000001000, 0x0000004000, 0x0000008000, 0x0000002000};
const uint64_t D[7] = {0x0008000000, 0x0002000000, 0x0004000000, 0x0000100000, 0x0000400000, 0x0000800000, 0x0000200000};
const uint64_t dp = 0x0000020000;

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
#define DATABYTE1 0   // Digit data byte 1 stored in i2c message byte 0
#define DATABYTE2 1   // Digit data byte 2 stored in i2c message byte 1
#define DATABYTE3 3   // Digit data byte 3 stored in i2c message byte 3
#define DATABYTE4 4   // Digit data byte 4 stored in i2c message byte 4
#define DATABYTE5 5   // Digit data byte 5 stored in i2c message byte 5
#define DATABYTESEL 2 // Speed mode indicator (km/h and mph bits) are stored on i2c message byte 2
#define DATABYTELEN 6
uint8_t tempData[DATABYTELEN];
const uint8_t maskKPH = 0x05;
const uint8_t maskMPH = 0x08;
uint8_t speedMode = 0x00; // 0=off - 1=kph - 2=mph
const uint64_t mask = 0xeeeef2fff0;   // Defining mask to "clean" only bits where LCD digits + dp are located

// GPS module  https://github.com/Jcabza008/UBLOX/
#include "UBLOX.h"
#define gpsTimeout 1500
unsigned long lastGpsRead = millis();
#define GPS_SERIAL Serial
#define GPS_DEFAULT_BAUDRATE 9600L
#define GPS_WANTED_BAUDRATE 115200L
UBLOX gps(GPS_SERIAL,GPS_WANTED_BAUDRATE);
bool gpsSetupDone = false;
float speed = 0;

void setup() {
  Wire.begin(I2C_ADDRESS);        // Joining cluster's i2c bus
  Wire.onReceive(receiveEvent);   // Registering event for receiving data
  
  // Begin GPS
  setupGPS(); 
  GPS_SERIAL.begin(GPS_WANTED_BAUDRATE); 
  gps.begin(); 
  
  // Starting i2c bus with LCD driver
  SoftWire.begin();  

  // Builtin LED will be used for debugging
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (speedMode) {
    // Getting speed from GPS, flagging for update if it has changed
    updateData = getSpeed();

    // Masking numbers on the byte array for LCD 1 (left)
    writeLCD1(speed);
  } else {
    //if (gpsSetupDone) stopGPS();
    //gpsSetupDone = false; // Reseting the GPS as not configured
    speed = INT16_MAX;
  }

  // If there is new data to display
  if(updateData){
    // Debug
    digitalWrite(LED_BUILTIN, HIGH);

    // Sending update to LCD driver's RAM registers
    updateLCD();

    // Wait a bit to avoid blasting the driver, and to have a visual output on the LED
    delay(5);

    // Turning the LED OFF
    digitalWrite(LED_BUILTIN, LOW);
  }
}

bool getSpeed(){
  // Saving previous speed in memory
  float oldSpeed = speed;

  if(gps.readSensor()) { // If GPS is available
    if (gps.isGnssFixOk()){  // If GPS has fix
      if(speedMode == 1){
        speed = gps.getGroundSpeed_ms() * 36;
      }else{
        speed = (gps.getGroundSpeed_fps() / 1.467) * 10;
      }

      // We have a speed with a fix, updating last GPS read timestamp, returning 0
      lastGpsRead = millis();
    }else{   // GPS is available BUT has no fix
      speed = INT16_MAX;
      return true;
    }
  }
  else // GPS isn't available right now but are we within timeout specs?
  {
    if(millis() - lastGpsRead > gpsTimeout){  // Nope... 
      speed = INT16_MAX;
      return true;
    }
  }

  // If is in speed mode and new speed has changed, return yes
  return (speed != oldSpeed && speedMode > 0);
}


void receiveEvent(int howMany) { 
  // Variables
  bool cmdSkip = true;
  bool newTempData = false;
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
    if(_bytesDataLength < DATABYTELEN){
      if(tempData[_bytesDataLength] != c) updateData = true;
      tempData[_bytesDataLength] = c;
    }else{
      if(bytesData[_bytesDataLength] != c) updateData = true;
      bytesData[_bytesDataLength] = c;
    }
    _bytesDataLength++;
  }
  
  // Updating array length
  bytesDataLength = _bytesDataLength;

  // Is speed mode on?
  speedMode = tempData[DATABYTESEL] == maskMPH ? 2 : (tempData[DATABYTESEL] == maskKPH) ? 1 : 0;

  // Merging data bytes
  bytesData[DATABYTE1] = speedMode ? (tempData[DATABYTE1] & ((~mask & 0xff00000000) >> 32)) + (bytesData[DATABYTE1] & (mask & 0xff00000000) >> 32) : tempData[DATABYTE1];
  bytesData[DATABYTE2] = speedMode ? (tempData[DATABYTE2] & ((~mask & 0x00ff000000) >> 24)) + (bytesData[DATABYTE2] & (mask & 0x00ff000000) >> 24) : tempData[DATABYTE2];
  bytesData[DATABYTE3] = speedMode ? (tempData[DATABYTE3] & ((~mask & 0x0000ff0000) >> 16)) + (bytesData[DATABYTE3] & (mask & 0x0000ff0000) >> 16) : tempData[DATABYTE3];
  bytesData[DATABYTE4] = speedMode ? (tempData[DATABYTE4] & ((~mask & 0x000000ff00) >> 8))  + (bytesData[DATABYTE4] & (mask & 0x000000ff00) >> 8)  : tempData[DATABYTE4];
  bytesData[DATABYTE5] = speedMode ? (tempData[DATABYTE5] &  (~mask & 0x00000000ff))        + (bytesData[DATABYTE5] & (mask & 0x00000000ff))       : tempData[DATABYTE5];
  bytesData[DATABYTESEL] = tempData[DATABYTESEL];
}


uint64_t getLCDMask(){
  uint64_t temp = 0;
    
    for (uint8_t i = 0; i < 7; i++) {
       temp += A[i];
       temp += B[i];
       temp += C[i];
       temp += D[i];
    }
    temp += dp;
}

void updateLCD(){
  SoftWire.beginTransmission(I2C_ADDRESS);      // Opening I2C bus as master
  SoftWire.write(bytesCmd, bytesCmdLength);     // Sending configuration commands
  SoftWire.write(bytesData, bytesDataLength);   // Sending data
  SoftWire.endTransmission();                   // Closing bus
}

void writeLCD1(uint16_t data){
  // Masking data bytes (cleaning the digits only, leaving the rest)
  bytesData[DATABYTE1] &= (~mask & 0xff00000000) >> 32;
  bytesData[DATABYTE2] &= (~mask & 0x00ff000000) >> 24;
  bytesData[DATABYTE3] &= (~mask & 0x0000ff0000) >> 16;
  bytesData[DATABYTE4] &= (~mask & 0x000000ff00) >> 8;
  bytesData[DATABYTE5] &= (~mask & 0x00000000ff);

  // If data == INT16_MAX then it's an invalid data
  // Building the 4 digits from the float argument
  uint8_t dataA = data == INT16_MAX ? 0x0A : ((uint16_t)data % 10000) / 1000;
  uint8_t dataB = data == INT16_MAX ? 0x0A : ((uint16_t)data % 1000) / 100;
  uint8_t dataC = data == INT16_MAX ? 0x0A : ((uint16_t)data % 100) / 10;
  uint8_t dataD = data == INT16_MAX ? 0x0A : ((uint16_t)data % 10);
  
  
  // For each digit, flipping segments bits to according to each digit that must be displayed
  uint64_t temp = 0;
  for (uint8_t i = 0; i < 7; i++) {
    if (((truthTable[dataA] >> i) & 0b1) && dataA != 0) temp += A[i];
    if (((truthTable[dataB] >> i) & 0b1) && (dataB != 0 || dataA != 0)) temp += B[i];
    if ((truthTable[dataC] >> i) & 0b1) temp += C[i];
    if ((truthTable[dataD] >> i) & 0b1) temp += D[i];
  }
  temp += dp;   // Flipping the decimal point

  // Applying OR mask on the data bytes, for each byte
  bytesData[DATABYTE1] |= (temp & 0xff00000000) >> 32;
  bytesData[DATABYTE2] |= (temp & 0x00ff000000) >> 24;
  bytesData[DATABYTE3] |= (temp & 0x0000ff0000) >> 16;
  bytesData[DATABYTE4] |= (temp & 0x000000ff00) >> 8;
  bytesData[DATABYTE5] |= (temp & 0x00000000ff);
}

