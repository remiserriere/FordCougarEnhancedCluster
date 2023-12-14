#include <Wire.h>
#include <SoftI2C.h>

const uint8_t truthTable[10] = {
  0b0111111, 
  0b0000110,
  0b1011011,
  0b1001111,
  0b1100110,
  0b1101101,
  0b1111101,
  0b0000111,
  0b1111111,
  0b1101111};
const uint64_t A[7] = {0x8000000000, 0x2000000000, 0x4000000000, 0x0000000010, 0x0000000040, 0x0000000080, 0x0000000020};
const uint64_t B[7] = {0x0800000000, 0x0200000000, 0x0400000000, 0x0000000100, 0x0000000400, 0x0000000800, 0x0000000200};
const uint64_t C[7] = {0x0080000000, 0x0020000000, 0x0040000000, 0x0000001000, 0x0000004000, 0x0000008000, 0x0000002000};
const uint64_t D[7] = {0x0008000000, 0x0002000000, 0x0004000000, 0x0000100000, 0x0000400000, 0x0000800000, 0x0000200000};
const uint64_t dp = 0x0000020000;
const uint64_t mask = 0xeeeef2fff0;   // Defining mask to "clean" only bits where LCD digits + dp are located

SoftI2C SoftWire = SoftI2C(4,5, true); //sda, scl
const uint8_t I2C_ADDRESS = 0x38;

uint8_t bytesCmd[3] = {0xE0, 0xC8, 0xF0};
uint8_t bytesData[32] ;//= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB6, 0x30, 0xC2, 0xF0, 0x6D, 0xFC, 0xFA, 0xD0, 0x64, 0xC6};
//uint8_t bytesData[32] = {0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xC6};
uint8_t bytesDataLength = 17;

void setup() {
  Wire.begin(0x38);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(115200);           // start serial for output

  Serial.println("Ready!");

  SoftWire.begin();  
}

float nbr = 0;

void loop() {

  writeNumber(nbr*10);

  nbr += 0.1;
  if(nbr > 999.9) nbr = 0;

  
  updateLCD();


  delay(5);
}


void receiveEvent(int howMany) { 
  // Variables
  bool cmdSkip = true;
  uint8_t _bytesDataLength = 0;

  // Ignoring command bytes
  while(Wire.available() && cmdSkip) { // loop through all
    uint8_t c = Wire.read();
    cmdSkip = c >= 0x80;

    // We still have to store the last read byte since it contains the first data byte
    if(c < 0x80){
      bytesData[_bytesDataLength] = c;
      _bytesDataLength = 1;
    }
  }

  // Storing data
  while(Wire.available()){
    bytesData[_bytesDataLength] = Wire.read();
    _bytesDataLength++;
  }
  bytesDataLength = _bytesDataLength;
}

void printHex(uint8_t data){
  if (data<0x10) {Serial.print("0");} 
  Serial.print(data,HEX); 
  Serial.print(" ");
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
  SoftWire.write(bytesCmd, 3);                  // Sending configuration commands
  SoftWire.write(bytesData, bytesDataLength);   // Sending data
  SoftWire.endTransmission();                   // Closing bus
  //SoftWire.write(bytesCmd[0]);
  //SoftWire.write(bytesCmd[1]);
  //SoftWire.write(bytesCmd[2]);
  //for(uint8_t i = 0; i < bytesDataLength; i++) SoftWire.write(bytesData[i]);
}

void writeNumber(uint16_t data){
  // We should not treat data greater than 9999 or we could mess things up
  if(data > 9999) data = 9999;

  // Masking data bytes (cleaning the digits only, leaving the rest)
  bytesData[1] &= (~mask & 0xff00000000) >> 32;
  bytesData[2] &= (~mask & 0x00ff000000) >> 24;
  bytesData[4] &= (~mask & 0x0000ff0000) >> 16;
  bytesData[5] &= (~mask & 0x000000ff00) >> 8;
  bytesData[6] &= (~mask & 0x00000000ff);

  // Building the 4 digits from the float argument
  uint8_t dataA = ((uint16_t)data % 10000) / 1000;
  uint8_t dataB = ((uint16_t)data % 1000) / 100;
  uint8_t dataC = ((uint16_t)data % 100) / 10;
  uint8_t dataD = ((uint16_t)data % 10);
  
  
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
  bytesData[1] |= (temp & 0xff00000000) >> 32;
  bytesData[2] |= (temp & 0x00ff000000) >> 24;
  bytesData[4] |= (temp & 0x0000ff0000) >> 16;
  bytesData[5] |= (temp & 0x000000ff00) >> 8;
  bytesData[6] |= (temp & 0x00000000ff);
}

