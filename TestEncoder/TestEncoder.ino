/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   26 Jun 2019
    Hardware:   STM32F103C "Blue Pill", 2.2" ILI9341 LCD,
                Adafruit #477 rotary encoder or similar
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
    
 Description:   Tests action of attached rotary encoder
   
 **************************************************************************/

#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"


#define ENCODER_A         PA8                     // Rotary Encoder output A
#define ENCODER_B         PA9                     // Rotary Encoder output A
#define ENCODER_BUTTON    PA4                     // Rotary Encoder switch
#define TFT_DC            PA0
#define TFT_CS            PA1
#define TFT_RST           PA2


//===================================  Color Constants ==================================
#define BLACK          0x0000
#define BLUE           0x001F
#define RED            0xF800
#define GREEN          0x07E0
#define CYAN           0x07FF
#define MAGENTA        0xF81F
#define YELLOW         0xFFE0
#define WHITE          0xFFFF
#define DKGREEN        0x03E0

// ==================================  Display Constants =================================
#define DISPLAYWIDTH      320                     // Number of LCD pixels in long-axis
#define DISPLAYHEIGHT     240                     // Number of LCD pixels in short-axis
#define TEXTCOLOR      YELLOW                     // Default non-menu text color

//===================================  Rotary Encoder Variables =========================
volatile int      rotaryCounter   = 0;           // "position" of rotary encoder (increments CW) 
volatile int      rotaryCW        = 0;           //  number of CW rotations
volatile int      rotaryCCW       = 0;           //  number of CCW rotations
volatile int      rotaryInvalid   = 0;           //  number of invalid transitions
volatile int      rotaryTicks     = 0;           //  total number of times ISR called
volatile byte     transition      = 0;           //  current transition state              

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

/* 
   Rotary Encoder Interrupt Service Routine ---------------
   This function runs when either encoder pin A or B changes state.
   The states array maps each transition 0000..1111 into CW/CCW rotation (or invalid).
   The rotary "position" is held in rotary_counter, increasing for CW rotation, decreasing 
   for CCW rotation. If the position changes, rotary_change will be set true. 
   You should set this to false after handling the change.
   To implement, attachInterrupts to encoder pin A *and* pin B 
*/

 
void rotaryISR()
{
  const int states[] = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0}; 

  transition <<= 2;                               // shift previous state up 2 bits
  transition |= (digitalRead(ENCODER_A));         // put encoder_A on bit 0
  transition |= (digitalRead(ENCODER_B) << 1);    // put encoder_B on bit 1
  transition &= 0x0F;                             // zero upper 4 bits

  int change = states[transition];                // map transition to CW vs CCW rotation
  if (change==0) rotaryInvalid++;
  if (change<0)  rotaryCCW++;
  if (change>0)  rotaryCW++;
  if (change!=0)                                  // if transition is valid,                      
    rotaryCounter += change;                      // update rotary counter +/- 1
}

void initEncoder()
{
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);  
  attachInterrupt(digitalPinToInterrupt(ENCODER_A),rotaryISR,CHANGE); 
  attachInterrupt(digitalPinToInterrupt(ENCODER_B),rotaryISR,CHANGE); 
}

void setup() {
  initEncoder();
  tft.begin();                                    // initialize screen object
  tft.setRotation(1);                             // landscape mode
  tft.setTextSize(3);                             // small text but readable
  tft.setTextColor(YELLOW,BLACK);
  tft.fillScreen(BLACK);                          // start with blank screen
}

void loop() {                              

  //tft.setCursor(0,50);     tft.print("Trans:"); 
  //tft.setCursor(130,50);   tft.print(transition,BIN);

  tft.setCursor(0,75);     tft.print("Count:"); 
  tft.setCursor(130,75);   tft.print(rotaryCounter);

  tft.setCursor(0,100);    tft.print("CW:"); 
  tft.setCursor(130,100);  tft.print(rotaryCW); 
  
  tft.setCursor(0,125);    tft.print("CCW:"); 
  tft.setCursor(130,125);  tft.print(rotaryCCW);

  tft.setCursor(0,150);    tft.print("Bad:"); 
  tft.setCursor(130,150);  tft.print(rotaryInvalid);  
  
  delay(1000);
  tft.fillRect(0,75,200,175,BLACK);
}
