const byte disableNmeaMessages[][2] = {
        {0xF0, 0x0A},
        {0xF0, 0x09},
        {0xF0, 0x00},
        {0xF0, 0x00},
        {0xF0, 0x00},
        {0xF0, 0x00},
        {0xF0, 0x01},
        {0xF0, 0x0D},
        {0xF0, 0x06},
        {0xF0, 0x02},
        {0xF0, 0x07},
        {0xF0, 0x03},
        {0xF0, 0x04},
        //  {0xF0, 0x0E},
        {0xF0, 0x0F},
        {0xF0, 0x05},
        {0xF0, 0x08},
        {0xF1, 0x00},
        {0xF1, 0x01},
        {0xF1, 0x03},
        {0xF1, 0x04},
        {0xF1, 0x05},
        {0xF1, 0x06},
        {0xF0, 0x00},
        {0xF0, 0x00},
        {0xF0, 0x00},
};

void setupGPS(){
    // Restore the receiver default configuration ONLY if GPS module is listening at 9600 bauds (default configuration)  
    delay(100); // Little delay before flushing.
    GPS_SERIAL.flush();
    GPS_SERIAL.begin(GPS_DEFAULT_BAUDRATE);
    restoreDefaults();
    //sendPacket((byte*)restoreDefaults);

    // Disable NMEA messages by sending appropriate packets.
    disableNmea();
    
    // Enable AssistNow Autonomous
    enableAssistNow();
    //sendPacket((byte*)enableAssistNow);

    // Increase frequency to 100 ms.
    changeFrequency();
    //sendPacket((byte*)changeFrequency);

    // Disable unnecessary channels like SBAS or QZSS.
    disableUnnecessaryChannels();
    //sendPacket((byte*)disableUnnecessaryChannels);

    // Enable NAV-PVT messages.
    enableNavPvt();
    //sendPacket((byte*)enableNavPvt);

    // Save configuration
    saveConfig();
    //sendPacket((byte*)saveConfig);

    delay(100); // Little delay before flushing.
    GPS_SERIAL.flush();

    // Switch the receiver serial to the wanted baudrate.
    changeBaudrate();
    //sendPacket((byte*)changeBaudrate);
    delay(100); // Little delay before flushing.
    GPS_SERIAL.flush();
    GPS_SERIAL.begin(GPS_WANTED_BAUDRATE);
}

// Send the packet specified to the receiver.
void sendPacket(byte* packet) {
    int len = sizeof(packet) / sizeof(packet[0]);
    for (byte i = 0; i < len; i++)
    {
        GPS_SERIAL.write(pgm_read_byte(&packet[i]));
    }
}
void sendPacket(const byte* packet, byte len) {
    for (byte i = 0; i < len; i++)
    {
        GPS_SERIAL.write(packet[i]);
    }
}

// Send a set of packets to the receiver to disable NMEA messages.
void disableNmea(){
    // Array of two bytes for CFG-MSG packets payload.
    

    // CFG-MSG packet buffer.
    byte packet[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x01, // id
        0x03, // length
        0x00, // length
        0x00, // payload (first byte from messages array element)
        0x00, // payload (second byte from messages array element)
        0x00, // payload (not changed in the case)
        0x00, // CK_A
        0x00, // CK_B
    };
    byte packetSize = sizeof(packet);

    // Offset to the place where payload starts.
    byte payloadOffset = 6;

    // Iterate over the messages array.
    for (byte i = 0; i < sizeof(disableNmeaMessages) / sizeof(*disableNmeaMessages); i++)
    {
        // Copy two bytes of payload to the packet buffer.
        for (byte j = 0; j < sizeof(*disableNmeaMessages); j++)
        {
            packet[payloadOffset + j] = disableNmeaMessages[i][j];
        }

        // Set checksum bytes to the null.
        packet[packetSize - 2] = 0x00;
        packet[packetSize - 1] = 0x00;

        // Calculate checksum over the packet buffer excluding sync (first two) and checksum chars (last two).
        for (byte j = 0; j < packetSize - 4; j++)
        {
            packet[packetSize - 2] += packet[2 + j];
            packet[packetSize - 1] += packet[packetSize - 2];
        }

        sendPacket(packet, packetSize);
    }
}

// Send a packet to the receiver to change frequency to 100 ms.
const byte packetchangeFrequency[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x08, // id
        0x06, // length
        0x00, // length
        0x64, // payload
        0x00, // payload
        0x01, // payload
        0x00, // payload
        0x01, // payload
        0x00, // payload
        0x7A, // CK_A
        0x12, // CK_B
};
void changeFrequency(){
    // CFG-RATE packet.
    /*byte packet[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x08, // id
        0x06, // length
        0x00, // length
        0x64, // payload
        0x00, // payload
        0x01, // payload
        0x00, // payload
        0x01, // payload
        0x00, // payload
        0x7A, // CK_A
        0x12, // CK_B
    };*/

    sendPacket(packetchangeFrequency, sizeof(packetchangeFrequency));
}

// Send a packet to the receiver to change baudrate to 115200.
const byte packetchangeBaudrate[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x00, // id
        0x14, // length
        0x00, // length
        0x01, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xD0, // payload
        0x08, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xC2, // payload
        0x01, // payload
        0x00, // payload
        0x07, // payload
        0x00, // payload
        0x03, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xC0, // CK_A
        0x7E, // CK_B
};
void changeBaudrate(){
    // CFG-PRT packet.
    /*byte packet[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x00, // id
        0x14, // length
        0x00, // length
        0x01, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xD0, // payload
        0x08, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xC2, // payload
        0x01, // payload
        0x00, // payload
        0x07, // payload
        0x00, // payload
        0x03, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xC0, // CK_A
        0x7E, // CK_B
    };*/

    sendPacket(packetchangeBaudrate, sizeof(packetchangeBaudrate));
}

// Send a set of packets to the receiver to enable AssistNow Autonomous.
const byte packetenableAssistNow[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x23, // id
        0x28, // length
        0x00, // length

        0x02, 0x00, 0x4C, 0x66, 0xC0, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0x20, 0x06, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x4B, 0x07, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x64, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

        0xA5, // CK_A
        0x6B, // CK_B
};
void enableAssistNow(){
    // CFG-NAVX5 packet.
    /*byte packet[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x23, // id
        0x28, // length
        0x00, // length

        0x02, 0x00, 0x4C, 0x66, 0xC0, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x03, 0x20, 0x06, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x4B, 0x07, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x64, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        
        0xA5, // CK_A
        0x6B, // CK_B
    };*/

    sendPacket(packetenableAssistNow, sizeof(packetenableAssistNow));
}

// Send a packet to the receiver to SAVE its configuration to all available memories.
const byte packetsaveConfig[] = { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x31, 0xBF, };
void saveConfig(){
    // CFG-CFG packet.
    //byte packet[] = {0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x31, 0xBF,};

    sendPacket(packetsaveConfig, sizeof(packetsaveConfig));
}

// Send a packet to the receiver to restore default configuration.
const byte packetrestoreDefaults[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x09, // id
        0x0D, // length
        0x00, // length
        0xFF, // payload
        0xFF, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xFF, // payload
        0xFF, // payload
        0x00, // payload
        0x00, // payload
        0x17, // payload
        0x2F, // CK_A
        0xAE, // CK_B
};
void restoreDefaults(){
    // CFG-CFG packet.
    /*byte packet[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x09, // id
        0x0D, // length
        0x00, // length
        0xFF, // payload
        0xFF, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0x00, // payload
        0xFF, // payload
        0xFF, // payload
        0x00, // payload
        0x00, // payload
        0x17, // payload
        0x2F, // CK_A
        0xAE, // CK_B
    };*/

    sendPacket(packetrestoreDefaults, sizeof(packetrestoreDefaults));
}

// Send a packet to the receiver to disable unnecessary channels.
const byte packetdisableUnnecessaryChannels[] = {
    0xB5, // sync char 1
    0x62, // sync char 2
    0x06, // class
    0x3E, // id
    0x3C, // length
    0x00, // length

    0x00, 0x00, 0x20, 0x07, 0x00, 0x08, 0x10, 0x00, // payload
    0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, // payload
    0x01, 0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x00, // payload
    0x01, 0x00, 0x01, 0x01, 0x03, 0x03, 0x08, 0x00, // payload
    0x00, 0x00, 0x01, 0x01, 0x04, 0x00, 0x08, 0x00, // payload
    0x01, 0x00, 0x01, 0x01, 0x05, 0x00, 0x03, 0x00, // payload
    0x01, 0x00, 0x01, 0x01, 0x06, 0x08, 0x0E, 0x00, // payload
    0x01, 0x00, 0x01, 0x01,             // payload

    0x24, // CK_A
    0x36, // CK_B
};
void disableUnnecessaryChannels(){
    // CFG-GNSS packet.
    /*byte packet[] = {
        0xB5, // sync char 1
        0x62, // sync char 2
        0x06, // class
        0x3E, // id
        0x3C, // length
        0x00, // length

        0x00, 0x00, 0x20, 0x07, 0x00, 0x08, 0x10, 0x00, // payload
        0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, // payload
        0x01, 0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x00, // payload
        0x01, 0x00, 0x01, 0x01, 0x03, 0x03, 0x08, 0x00, // payload
        0x00, 0x00, 0x01, 0x01, 0x04, 0x00, 0x08, 0x00, // payload
        0x01, 0x00, 0x01, 0x01, 0x05, 0x00, 0x03, 0x00, // payload
        0x01, 0x00, 0x01, 0x01, 0x06, 0x08, 0x0E, 0x00, // payload
        0x01, 0x00, 0x01, 0x01,             // payload

        0x24, // CK_A
        0x36, // CK_B
    };*/

    sendPacket(packetdisableUnnecessaryChannels, sizeof(packetdisableUnnecessaryChannels));
}

// Send a packet to the receiver to enable NAV-PVT messages.
const byte packetenableNavPvt[] = {
    0xB5, // sync char 1
    0x62, // sync char 2
    0x06, // class
    0x01, // id
    0x03, // length
    0x00, // length
    0x01, // payload
    0x07, // payload
    0x01, // payload
    0x13, // CK_A
    0x51, // CK_B
};
void enableNavPvt(){
    // CFG-MSG packet.
    /*byte packet[] = {
    0xB5, // sync char 1
    0x62, // sync char 2
    0x06, // class
    0x01, // id
    0x03, // length
    0x00, // length
    0x01, // payload
    0x07, // payload
    0x01, // payload
    0x13, // CK_A
    0x51, // CK_B
    };*/

    sendPacket(packetenableNavPvt, sizeof(packetenableNavPvt));
}