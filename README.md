# What is this project?
**Ford Cougar Enhanced CLuster** is a "spin-off" of a similar but way more complex project I am developping dedicated to the Chrysler LH platform. This project can be used to interface an Arduino or similar AVR microcontroller with the LCD driver of a 1998-2002 Ford/Mercury Cougar.

The heart of the 1998-2002 Cougar's cluster is a Motorola MC68HC11 microcontroller. It uses I2C to communicate with the LCD driver manufactured by Philips, which unfortunately has a custom part number. After some reverse ingeneering and comparing datasheet, I made an educated guess and figured out the driver was a PCF8576. 

My goal was to display the GPS speed of the car in place of the useless average speed than can be displayed on the trip computer. 
Cut the I2C traces between the MC68HC11 and the PCF8576, connect an Arduino in-between (I used a Nano for testing, and a Pro Mini in "production"), add a U-Blox based GPS receiver such as a Beitian BN-180, pwoer everything from the fused ignition +12 volts supply from the cluster itself, and you get yoursel a GPS speedometer.

**Be carefull! This mod involves cutting very thin traces on your cluster motherboard! This could kill it!**

# I2C protocol
The I2C bus is "mastered" my the MC68HC11 with internal pullup resistors. The slave LCD driver responds on address 0x38. Everytime the MC68HC11 wants to update data on the LCD, it sends a message over the I2C bus made of commands first, and then data to display. The commands are well explained in the PCF8576 datasheet at page 23. 

Technically, the master can send up to the 5 available commands to the LCD driver... So it needs to know where the commands stop and where the display data start in the message which could potentially be 26 bytes long! For that, it will look at the first byte with its most significant bit set to 0 (in other words, is the byte less than 0x80?), this byte will be the last command sent. All the rest will be display data.

In practice, only 16 bytes of data will be used on the cluster because some outputs of the driver are not used. The LCDs have less inputs than the driver has ouputs. It took me a long time, but I was able to map all the pixels on both the 2 LCDs...
![PXL_20231211_234623849](https://github.com/remiserriere/FordCougarEnhancedCluster/assets/31062140/866eccbc-3c53-400a-8361-2db07f56fa59)

# What's in there?
- Basic example of how you could interface an Arduino with your cluster's LCDs if you need more tha an GPS speedometer.
- Full mapping of LCD pixels vs message byte value.
- PCF8576 datasheet
- Shematics

# TODO: Where to cut traces on the PCB
With pictures
