
######          Moxl_ Speedometer for Arduino Pro Micro  
######          You're free to do whatever with it.      

 ###### Why Pro Micro ? 
    * It's what I had, and it's compact.

 ###### What do I need?
    * Arduino Pro Micro (probably each Leonardo will do)
    * Hall sensor + magnet (most generic sensor with breakoutboards)
    * SSD1306 128*64 I2C screen.
    * SPI SD card reader + SD card (mine says HW-125)
        * SD card should be formatted in FAT32 (quickformat on windows in fat will do.)

 ###### Why sdFat & SSD1306Ascii libraries?
    * The Pro Micro was tad bit overloaded in SRAM.
    * They're both lightweight in comparrison.

> Sketch uses 26090 bytes (90%) of program storage space. Maximum is 28672 bytes.
> Global variables use 2016 bytes (78%) of dynamic memory, leaving 544 bytes for local variables. Maximum is 2560 bytes.
> Low memory available, stability problems may occur.


 ###  Pins for arduino pro micro
 * SDA   -> 2   - SSD1306 Screen
 * SCL   -> 3   - SSD1306 Screen
 * Hall  -> 7   - Hall sensor
 * MISO  -> 14  - SD card reader
 * MoSi  -> 16  - SD card reader
 * SCK   -> 15  - SD card reader
 * CS    -> 4   - SD card reader


