#include <Wire.h>

// 0xE0, 0xC8, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0xB6, 0x30, 0xC2, 0xF0, 0x6D, 0xFC, 0xFA, 0xD0, 0x64, 0xC6
uint8_t bytes[20] = {0xE0, 0xC8, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6};

void setup()
{
  Serial.begin(115200);
  Serial.println("Address search");
  Serial.println("-------------------------");

  Wire.begin();
  int i2c = 0x00;
  for (int i = i2c; i <= 0xFF; i++)
  {
    
    Wire.beginTransmission(i2c);
    int result = Wire.endTransmission();
    
    if (result == 0)
    {
      Serial.print("Search at [");
      Serial.print(i2c, HEX);
      Serial.print("]: ");
    
      Serial.println("FOUND!");
    }
    else
    {
      //Serial.println("not found");
    }
    i2c++;
  }

}

void loop()
{
  //demoLoop();
  //return;

  if(Serial.available() > 0)
  {
    uint8_t b = Serial.parseInt(SKIP_ALL, '\n');

    bytes[4] = b;
    bytes[5] = b;
    bytes[6] = b;
    bytes[7] = b;
    bytes[8] = b;

    bytes[9] = b;

    //b = 255;
    bytes[10] = b;
    bytes[11] = b;
    bytes[12] = b;
    bytes[13] = b;
    bytes[14] = b;
    bytes[15] = b;
    bytes[16] = b;
    bytes[17] = b;
    bytes[18] = b;
    
    Wire.beginTransmission(0xb8);   // 0x38 - 0xb8
    for(uint8_t i = 0; i < 19; i++){
      Wire.write(bytes[i]);
    }
    Wire.endTransmission();

    Serial.println(b, HEX);
    
  }
  
  


//delay(1000);

}


void demoLoop(){
  uint8_t val = 0;
  uint8_t list[9] = {0, 1, 2, 4, 8, 16, 32, 64, 128};
  for(uint8_t i = 0; i < 9; i++){
  //for(uint8_t i = 0; i <= 255; i++){

    Wire.beginTransmission(0x38);   // 0x38 - 0xb8
    
    Wire.write(0xE0);
    Wire.write(0xC8);
    Wire.write(0xF0);
    Wire.write(0x00);

    //val = list[i];
    val = val + list[i];
    Serial.println(val, HEX);

    for(uint8_t cnt = 4; cnt < 19; cnt++){
      Wire.write(val);
    }

    Wire.write(0xC6);

    Wire.endTransmission();
    
    delay(1000);
  }
}
