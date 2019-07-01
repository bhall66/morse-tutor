/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   26 Jun 2019
    Hardware:   STM32F103C "Blue Pill", 2.2" ILI9341 LCD
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
    
 Description:   Very Simple code to test the display

  LCD DISPLAY CONNECTIONS:
  pin1 "VCC" to 3.3V    pin6 "MOSI" to PA7
  pin2 "GND" to gnd     pin7 "SCK " to PA5
  pin3 "CS " to PA1     pin8 "LED " to 3.3V
  pin4 "RST" to PA2     pin9 "MISO" to PA6
  pin5 "DC"  to PA0

   
 **************************************************************************/

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

#define TFT_DC         PA0
#define TFT_CS         PA1
#define TFT_RST        PA2

//===================================  Color Constants ==================================
#define BLACK          0x0000
#define MAGENTA        0xF81F
#define YELLOW         0xFFE0
#define WHITE          0xFFFF

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  tft.begin();                                    // initialize screen object
  tft.setRotation(1);                             // landscape mode
  tft.fillScreen(BLACK);                          // start with blank screen
  tft.setTextSize(3);                             // medium size font
  tft.setTextColor(YELLOW,BLACK);
  
  for (int i=0; i<5; i++) {
    tft.setCursor(i*25,i*25);
    tft.print("Hello World");                     // write to the display
  }
  for (int i=5; i<9; i++) {
    tft.setCursor((9-i)*25,i*25);
    tft.print("Hello World");                     // and write some more.
  }
}

void loop() {
}
