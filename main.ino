//-------------------------------------------------------------------------
//         - Moxl_ Speedometer for Arduino Pro Micro  -
//         - You're free to do whatever with it.      -
//
// - Why Pro Micro ? 
//    --> It's what I had, and it's compact.
//
// - What do I need?
//    --> Arduino Pro Micro (probably each Leonardo will do)
//    --> Hall sensor + magnet (most generic sensor with breakoutboards)
//    --> SSD1306 128*64 I2C screen.
//    --> SPI SD card reader + SD card (mine says HW-125)
//        --> SD card should be formatted in FAT32 (quickformat on windows in fat will do.)
//
// - Why sdFat & SSD1306Ascii libraries?
//    --> The Pro Micro was tad bit overloaded in SRAM.
//    --> They're both lightweight in comparrison.
//
//-------------------------------------------------------------------------
// --  Pins for arduino pro micro
// > SDA   -> 2   - SSD1306 Screen
// > SCL   -> 3   - SSD1306 Screen
// > Hall  -> 7   - Hall sensor
// > MISO  -> 14  - SD card reader
// > MoSi  -> 16  - SD card reader
// > SCK   -> 15  - SD card reader
// > CS    -> 4   - SD card reader
//-------------------------------------------------------------------------

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SdFat.h"
#include "sdios.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"


//pins
const byte interruptPin = 7;    //Hall sensor
const int chipSelect = 4;      //SD


//Screen
// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C
// Define proper RST_PIN if required.
#define RST_PIN -1
SSD1306AsciiAvrI2c oled;

const uint8_t SD_CS_PIN = chipSelect;
// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI)
#endif  // HAS_SDIO_CLASS

SdFat sd;
File file;
File root;

// Store error strings in flash to save RAM.
#define error(s) sd.errorHalt(&Serial, F(s))


//Basics
volatile float trip = 0;
volatile float lastTrip;
volatile float totalTrip;
String odoSD;
String topSD;
volatile float rps = 0;
volatile float rpm = 0;
volatile float prev_time = 0;
int speedkm = 0;
int topSpeed=0;
int topSpeedSD=0;
volatile float wheelCircumference = 2.02; //m
//timers
unsigned long duration = 0;
unsigned long lastTrigger;
unsigned long timer;
unsigned long timepreviouscalc;
unsigned long timecalctimer = 1000; // time for screen refresh
unsigned long speedtimer = 3000; // time for speed reset
unsigned long timereeprom=0; 
unsigned long timepreviouscalceeprom = 0;
unsigned long timecalctimereeprom = 10000;// time for upload to sd
int      allowed=0;
          
//---------------------------------------------------
// Setup
//---------------------------------------------------

void setup() {
  Serial.begin(115200);

//Only needed for Pro Micro /Leonardo
  while (!Serial) ;
  Serial.setTimeout(600000);
//start
  Serial.println("-------------------------------------------");
  Serial.println("| Moxl_ Speedometer for Arduino Pro Micro  |");
  Serial.println("| You're free to do whatever with it.      |");
  Serial.println("-------------------------------------------");
  Serial.println("System Starting");
  Serial.println("-------------------------------------------");
  Serial.println(">>> Starting");

//Set pins
    pinMode(LED_BUILTIN,OUTPUT);
    // when pin D2 goes high, call the rising function
    pinMode(interruptPin, INPUT); 
    // set CS pin for SD card
    pinMode(chipSelect, OUTPUT);

//setup display
    Serial.println("> Setting up Display");
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
    Serial.println("-- SSD1306 allocation ok");
    Serial.println();
    oled.setFont(Adafruit5x7);

    Serial.println("> Logo to screen");
    oled.clear();
    oled.println();
    oled.println();
    oled.println();
    oled.set2X();
    oled.println(" - MoxL -");
    oled.set1X();
    oled.println();
    oled.println("       Speedo");
    delay(1000);

//SD
    Serial.println("> Initialize the SD card.");
    // Initialize the SD card.
    if (!sd.begin(SD_CONFIG)) {
          sd.initErrorHalt(&Serial);
    }else{
        Serial.println("-- SD card ok");
        
    }
//SD size
    uint32_t sized = sd.card()->sectorCount();
    uint32_t sizeMB = 0.000512 * sized + 0.5;
    Serial.print("-- SD card size: ");
    Serial.print(sizeMB);
    Serial.println("Mb.");
    Serial.println();

    
//SD Filesystem check    
    Serial.println("> Checking File system");
    Serial.println("> Folders");
    //checking folders
    
    //system
    if (sd.exists("system")) {
      Serial.println("-- Folder system exists");
    }else{
        if (!sd.mkdir("system")) {
            error("-- Create system failed");
        }else{
          Serial.println("-- Create system ok");
        } 
    }

//checking files
    Serial.println("> Files");
//system/odometer
    if (sd.exists("system/odometer.txt")){
      Serial.println("-- File system/odometer ok");
    }else{
         if (!file.open("system/odometer.txt", O_WRONLY | O_CREAT)) {
                error("-- create system/odometer.txt failed");
          }else{
                file.close();
                Serial.println("-- Created system/odometer.txt\n");  
          }
    }
        Serial.println();

//system/topspeed
    if (sd.exists("system/topspeed.txt")){
      Serial.println("-- File system/topspeed ok");
    }else{
         if (!file.open("system/topspeed.txt", O_WRONLY | O_CREAT)) {
                error("-- create system/topspeed.txt failed");
          }else{
                file.close();
                Serial.println("-- Created system/topspeed.txt\n");  
          }
    }
        Serial.println();
       
        
//Fetching odometer.
    Serial.println("> Fetching odometer");
  char line[25];
  int n;
  // open test file
  SdFile file("system/odometer.txt", O_READ);
 
  // check for open error
  if (!file.isOpen()) error("Cant open odometer.txt");

  // read lines from the file
  while ((n = file.fgets(line, sizeof(line))) > 0) {
    if (line[n - 1] == '\n') {
      // remove '\n'
      line[n-1] = 0;
      // replace next line with LCD call to display line
      odoSD = line;
      totalTrip = odoSD.toFloat();
      Serial.print("-- Data from SD:");
      Serial.print("\t"); 
      Serial.println(odoSD);
      Serial.print("-- Converted data:");
      Serial.print("\t"); 
      Serial.println(totalTrip);
      
    } else {
      Serial.println("Cant read odometer.txt or wrong data, try emptying the file");
    }
  }

//Fetching topspeed.
    Serial.println("> Fetching topspeed");
  char linetop[25];
  int m;
  // open test file
  SdFile file2("system/topspeed.txt", O_READ);
 
  // check for open error
  if (!file2.isOpen()) error("Cant open topspeed.txt");

  // read lines from the file
  while ((m = file2.fgets(linetop, sizeof(linetop))) > 0) {
    if (linetop[m - 1] == '\n') {
      // remove '\n'
      linetop[m-1] = 0;
      // replace next line with LCD call to display line
      topSD = linetop;
      topSpeedSD = topSD.toFloat();
      Serial.print("-- Data from SD:");
      Serial.print("\t"); 
      Serial.println(topSD);
      Serial.print("-- Converted data:");
      Serial.print("\t"); 
      Serial.println(topSpeedSD);
      
    } else {
      Serial.println("Cant read odometer.txt or wrong data, try emptying the file");
    }
  }
  
//attach interrupt  
  Serial.println();
  Serial.println("> attaching interrupts");
  attachInterrupt(digitalPinToInterrupt(interruptPin), rising, RISING);

//restting parms
  rpm=0;
  trip=0;
  lastTrip=0;
  Serial.println("-------------------------------------------");
  Serial.println(">>> System Started");
  Serial.println("-------------------------------------------");
}
//---------------------------------------------------
// Loop
//---------------------------------------------------
void loop() {

//------------
// output to screen
    timer = millis();
    if(timer >= (timepreviouscalc + timecalctimer)==true ){
        timepreviouscalc = millis();
        

            oled.clear();
    
              //oled.set2X();
              //oled.print();
              //oled.println();
              oled.set2X();
              oled.print("   ");
              oled.print(speedkm);
              oled.set1X();
              oled.println("Km/u");
              oled.println();
              oled.println();
              oled.print("Top: ");
              oled.print(topSpeed);
              oled.print(" Hist: ");
              oled.println(topSpeedSD);
              oled.println("___________________");
              
              oled.set1X();
              oled.print("Trip : ");
              oled.print(trip/1000);
              oled.println("Km");

              oled.print("Total: ");
              oled.print(totalTrip/1000); 
              oled.println("Km");



             }
             
//------------
//set TOP speed
      if(speedkm > topSpeed){
                 topSpeed = speedkm;
        }

//------------
//set HIST TOP speed
      if(topSpeed > topSpeedSD){
                 topSpeedSD = topSpeed;
                 Serial.println("> Writing to SD");  
            if (file.open("system/topspeed.txt", FILE_WRITE)){
                file.rewind();
                Serial.println("> File opened");
         
                Serial.print("> buffer to write: ");
                Serial.println(topSpeed);
                file.println(topSpeed);
                if(!file.close()){error("close failed");}
               delay(100);
                Serial.println("-- SD updated!");
            }else{
              Serial.println("error opening file");}
        }

//------------
//set speed to 0 when not getting updates
        if(timer >= (lastTrigger + speedtimer)==true && rps != 0){
              rps=0;
              speedkm=0;
                          allowed=1;
            Serial.println(allowed);
              Serial.println("-- Reset RPS");
        }

//------------
//Update odometer on sd
    
       timereeprom = millis();
        if(((timereeprom>=(timepreviouscalceeprom+timecalctimereeprom))==true) && allowed==1){
            timecalctimereeprom = millis();
            allowed=0;
       
            Serial.println("> Writing to SD");  
            if (file.open("system/odometer.txt", FILE_WRITE)){
                file.rewind();
                Serial.println("> File opened");
                volatile float i = totalTrip + trip;
                Serial.print("> buffer to write: ");
                Serial.println(i);
                file.println(i);
                if(!file.close()){error("close failed");}
               delay(100);
                Serial.println("-- SD updated!");
            }else{
              Serial.println("error opening file");}
        //end of timereeprom    
          }
//------------
// end of loop
}
//---------------------------------------------------
// Interrupt
//---------------------------------------------------
 
void rising() {
duration = millis() - lastTrigger;

  if (duration > 50 && duration < 2000) {
    trip = trip + wheelCircumference; // Increment odometer
    rps = 1000.0 / duration; // Update rotations per second
           rpm = rps * 60;
    speedkm = (((rpm * wheelCircumference)* 60)/1000);
    
  }    
lastTrigger = millis();
  
}
