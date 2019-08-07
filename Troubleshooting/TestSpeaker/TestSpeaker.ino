/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   07/09/2019
    Hardware:   STM32F103C "Blue Pill", 
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
       Legal:   Copyright (c) 2019  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
                    
 Description:   Test your speaker.
                This simple sketch outputs a 1000 Hz tone, pulsing at
                a rate of 1 Hz (500mS on, 500mS off).
   
 **************************************************************************/

#define LED                PC13                   // pin for onboard LED
#define SPEAKER            PB12                   // pin attached to speaker
#define PITCH              1000                   // tone pitch, in Hz
#define DURATION            500                   // tone duration, in mS


void beep()
{
  digitalWrite(LED,LOW);                          // LED indicates tone is on.
  tone(SPEAKER,PITCH);                            // Start the tone
  delay(DURATION);                                // keep it on for a while... 
  noTone(SPEAKER);                                // then turn it off
  digitalWrite(LED,HIGH);                         // and LED, too.           
  delay(DURATION);                                // Keep it off for a while
}

void setup() {
  pinMode(LED,OUTPUT);                            // set up onboard LED
}

void loop() { 
    beep();                                       // beep forever.   
}
