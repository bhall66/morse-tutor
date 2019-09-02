/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   02 Sep 2019
    Hardware:   STM32F103C "Blue Pill", 
                Adafruit #477 rotary encoder or similar
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
       Legal:   Copyright (c) 2019  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Test your rotary encoder!
                Turning encoder CW will send "short" flashes to LED
                Turning encoder CCW will send "long" flashes to LED
                Pressing the encoder button will double-flash the LED
   
 **************************************************************************/

#define LED               PC13                    // pin for onboard LED

// The following defines specify which pins are attached to the rotary encoder
// Make sure that each of these can be used as an external interrupt.

#define ENCODER_A         PA9                     // Rotary Encoder output A
#define ENCODER_B         PA8                     // Rotary Encoder output B
#define ENCODER_BUTTON    PB15                    // Rotary Encoder switch

#define SHORTFLASH        100                     // duration in mS of short LED flash
#define LONGFLASH         300                     // duration in mS of long LED flash

volatile int      rotaryCounter    = 0;           // "position" of rotary encoder (increments CW) 
volatile boolean  button_pressed   = false;       // true if the button has been pushed
volatile boolean  button_released  = false;       // true if the button has been released (sets button_downtime)
volatile long     button_downtime  = 0L;          // ms the button was pushed before released         

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
  static byte transition = 0;                     // holds current and previous encoder states 
  transition <<= 2;                               // shift previous state up 2 bits
  transition |= (digitalRead(ENCODER_A));         // put encoder_A on bit 0
  transition |= (digitalRead(ENCODER_B) << 1);    // put encoder_B on bit 1
  transition &= 0x0F;                             // zero upper 4 bits                 
  rotaryCounter += states[transition];            // update rotary counter +/- 1
}

/* 
   Rotary Encoder Button Interrupt Service Routine ----------
   Process encoder button presses and releases, including debouncing (extra "presses" from 
   noisy switch contacts). If button is pressed, the button_pressed flag is set to true. If 
   button is released, the button_released flag is set to true, and button_downtime will 
   contain the duration of the button press in ms.  Manually reset flags after handling event. 
*/

void buttonISR()
{  
  static boolean button_state = false;
  static unsigned long start, end;  
  boolean pinState = digitalRead(ENCODER_BUTTON);
  
  if ((pinState==LOW) && (button_state==false))                     
  {                                               // Button was up, but is now down
    start = millis();                             // mark time of button down
    if (start > (end + 10))                       // was button up for 10mS?
    {
      button_state = true;                        // yes, so change state
      button_pressed = true;
    }
  }
  else if ((pinState==HIGH) && (button_state==true))                       
  {                                               // Button was down, but now up
    end = millis();                               // mark time of release
    if (end > (start + 10))                       // was button down for 10mS?
    {
      button_state = false;                       // yes, so change state
      button_released = true;
      button_downtime = end - start;              // and record how long button was down
    }
  }
}

/* 
   readEncoder() returns 0 if no significant encoder movement since last call,
   +1 if clockwise rotation, and -1 for counter-clockwise rotation
*/

int readEncoder(int numTicks = 3) 
{
  static int prevCounter = 0;                     // holds last encoder position
  int change = rotaryCounter - prevCounter;       // how many ticks since last call?
  if (abs(change)<=numTicks)                      // not enough ticks?
    return 0;                                     // so exit with a 0.
  prevCounter = rotaryCounter;                    // enough clicks, so save current counter values
  return (change>0) ? 1:-1;                       // return +1 for CW rotation, -1 for CCW    
}

void initEncoder()
{
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP); 
  pinMode(ENCODER_BUTTON, INPUT_PULLUP); 
  attachInterrupt(digitalPinToInterrupt(ENCODER_A),rotaryISR,CHANGE); 
  attachInterrupt(digitalPinToInterrupt(ENCODER_B),rotaryISR,CHANGE); 
  attachInterrupt(digitalPinToInterrupt(ENCODER_BUTTON),buttonISR,CHANGE);
}

void flashLED(int duration)
{
  digitalWrite(LED,LOW);                          // turn on LED
  delay(duration);
  digitalWrite(LED,HIGH);                         // turn off LED
  delay(duration);
}

void setup() {
  initEncoder();                                  // set up encoder interrupt pins
  pinMode(LED,OUTPUT);                            // set up onboard LED
  digitalWrite(LED,HIGH);                         // and make sure its OFF
}

void loop() { 
  if (button_pressed)                             // was encoder button pressed?
  {
    flashLED(80);                                 // yes, so 2 quick flashes
    flashLED(80);
    button_pressed = false;                       // and reset flag
  }
  int i=readEncoder();                            // was encoder knob turned?
  if (i>0) flashLED(SHORTFLASH);                  // yes (CW) so give a short flash
  if (i<0) flashLED(LONGFLASH);                   // yes (CCW) so give a long flash          
}
