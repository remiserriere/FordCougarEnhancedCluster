// Send a packet to the receiver to set the refresh rate at 10 Hz (100 ms peasurement period)
const byte setFrequency[] = {0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00}; 

// Send a packet to the receiver to change baudrate to 115200, disable NMEA, UBX protocol only.
const byte setBaudrate[] = {0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x00, 0xC2, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}; 

// Send a set of packets to the receiver to enable AssistNow Autonomous.
const byte enableAssistNow[] = {
        0x06, 0x23, 0x28, 0x00,                         // Class, id, length length
        0x02, 0x00, 0x4C, 0x66, 0xC0, 0x00, 0x00, 0x00, // Payload
        0x00, 0x00, 0x03, 0x20, 0x06, 0x00, 0x00, 0x00, // Payload
        0x00, 0x00, 0x4B, 0x07, 0x00, 0x00, 0x00, 0x00, // Payload
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x64, 0x00, // Payload
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Payload
};

// Send a packet to the receiver to disable unnecessary channels.
const byte disableUnnecessaryChannels[] = {
    0x06, 0x3E, 0x3C, 0x00,                         // Class, id, length length
    0x00, 0x00, 0x20, 0x07, 0x00, 0x08, 0x10, 0x00, // Payload
    0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, // Payload
    0x01, 0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x00, // Payload
    0x01, 0x00, 0x01, 0x01, 0x03, 0x03, 0x08, 0x00, // Payload
    0x00, 0x00, 0x01, 0x01, 0x04, 0x00, 0x08, 0x00, // Payload
    0x01, 0x00, 0x01, 0x01, 0x05, 0x00, 0x03, 0x00, // Payload
    0x01, 0x00, 0x01, 0x01, 0x06, 0x08, 0x0E, 0x00, // Payload
    0x01, 0x00, 0x01, 0x01,                         // Payload
};

// Send a packet to the receiver to enable NAV-PVT messages.
const byte enableNavPvt[] = {0x06, 0x01, 0x03, 0x00, 0x01, 0x07, 0x01}; 

// Send a packet to the receiver to SAVE its configuration to all available memories.
const byte saveConfig[] = {0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17}; 

// Send a packet to the receiver to restore default configuration.
const byte restoreDefaults[] = {0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x17}; 

const uint8_t sync1 = 0xB5;
const uint8_t sync2 = 0x62;

bool setupGPS(){
    // Restore the receiver default configuration ONLY if GPS module is listening at 9600 bauds (default configuration)  
    delay(100); // Little delay before flushing.
    GPS_SERIAL.flush();
    GPS_SERIAL.begin(GPS_DEFAULT_BAUDRATE);    
    sendPacket(restoreDefaults, sizeof(restoreDefaults));
    
    // Enable AssistNow Autonomous
    sendPacket(enableAssistNow, sizeof(enableAssistNow));

    // Increase frequency to 100 ms.
    sendPacket(setFrequency, sizeof(setFrequency));

    // Disable unnecessary channels like SBAS or QZSS.
    sendPacket(disableUnnecessaryChannels, sizeof(disableUnnecessaryChannels));

    // Enable NAV-PVT messages.
    sendPacket(enableNavPvt, sizeof(enableNavPvt));

    // Save configuration
    sendPacket(saveConfig, sizeof(saveConfig));

    delay(100); // Little delay before flushing.
    GPS_SERIAL.flush();

    // Switch the receiver serial to the wanted baudrate and disable NMEA.
    sendPacket(setBaudrate, sizeof(setBaudrate));
    delay(100); // Little delay before flushing.
    GPS_SERIAL.flush();
    GPS_SERIAL.begin(GPS_WANTED_BAUDRATE);
    lastGpsRead = millis();

    return true;
}

// Send the packet specified to the receiver.
void sendPacket(const byte* packet, byte len) {
  // Processing CRC
  uint8_t CK_A = 0;
  uint8_t CK_B = 0;
	for (uint8_t i = 0; i < len; i++) {
		CK_A += packet[i];
		CK_B += CK_A;
	}

  // Sending SYNC bytes
  GPS_SERIAL.write(sync1);
  GPS_SERIAL.write(sync2);

  // Sending data
  for (byte i = 0; i < len; i++)
  {
    GPS_SERIAL.write(packet[i]);
  }
  
  // Sending CRC
  GPS_SERIAL.write(CK_A);
  GPS_SERIAL.write(CK_B);
}
