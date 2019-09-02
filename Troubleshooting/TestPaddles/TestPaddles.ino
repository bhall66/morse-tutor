/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   02 Sep 2019
    Hardware:   STM32F103C "Blue Pill" with Piezo Buzzer
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
       Legal:   Copyright (c) 2019  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Use Paddles to send Morse Code.
                Connect piezo to PA2.  Other lead to ground.
                Connect paddles to PB8 & PB9.  Other lead to ground.
   
 **************************************************************************/

#define PADDLE_A          PB8                // Morse Paddle "dit"
#define PADDLE_B          PB7                // Morse Paddle "dah"
#define AUDIO             PA2                // Audio output
#define LED               PC13               // onboard LED pin
#define CODESPEED         13                 // speed in Words per Minute
#define PITCH             1000               // pitch in Hz of morse audio
#define DITPERIOD         1200/CODESPEED     // period of dit, in milliseconds


void ditSpaces(int spaces=1) {               // user specifies #dits to wait
  for (int i=0; i<spaces; i++)               // count the dits...
    delay(DITPERIOD);                        // no action, just mark time
}

void dit() {
  digitalWrite(LED,0);                       // turn on LED
  tone(AUDIO,PITCH);                         // and turn on sound
  ditSpaces();                               // wait for period of 1 dit
  digitalWrite(LED,1);                       // turn off LED
  noTone(AUDIO);                             // and turn off sound
  ditSpaces();                               // space between code elements
}

void dah() {
  digitalWrite(LED,0);                       // turn on LED
  tone(AUDIO,PITCH);                         // and turn on sound
  ditSpaces(3);                              // length of dah = 3 dits
  digitalWrite(LED,1);                       // turn off LED
  noTone(AUDIO);                             // and turn off sound
  ditSpaces();                               // space between code elements
}

bool ditPressed()
{
  return (digitalRead(PADDLE_A)==0);         // pin is active low
}

bool dahPressed()
{
  return (digitalRead(PADDLE_B)==0);         // pin is active low
}

void setup() {
  pinMode(LED,OUTPUT);
  pinMode(PADDLE_A, INPUT_PULLUP);           // two paddle inputs, both active low
  pinMode(PADDLE_B, INPUT_PULLUP);                            
}

void loop()                                  // start using those paddles!
{   
  if (ditPressed())  dit();                  // user wants a dit, so do it.
  if (dahPressed())  dah();                  // user wants a dah, so do it.                             
}
