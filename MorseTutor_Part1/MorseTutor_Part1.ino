/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   30 Jun 2019
    Hardware:   STM32F103C "Blue Pill",
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
    
 Description:   Basic Blink sketch, adopted for the Blue Pill
   
 **************************************************************************/

#define LED   PC13

void setup() {
  pinMode(LED, OUTPUT);
}

void loop() {
  digitalWrite(LED, LOW);            // turn the LED on
  delay(500);                        // wait half a second
  digitalWrite(LED, HIGH);           // turn the LED off
  delay(500);                        // wait half a second
}
