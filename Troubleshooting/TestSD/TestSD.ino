/**************************************************************************
    Author:    Bruce E. Hall, w8bh.net
    Date:      07 Jul 2019
    Hardware:  STM32F103C "Blue Pill" with Piezo Buzzer
      Legal:   Copyright (c) 2019  Bruce E. Hall.
               Open Source under the terms of the MIT License. 
    
 Description:  Tests the SD card input. 
               Put textfile "morse.txt" on SD card & insert card. 
               File will play over piezo/speaker on PB12

  Connections:  SD_DC to PA4, 
                SD_MOSI to PA7
                SD_MISO to PA6
                SD_SCK  to PA5 
               .
 **************************************************************************/
#include "SD.h"

#define SD_CS       PA4                           // SD card "CS" pin
#define TEXTFILE        "morse.txt"               // name of file on SD card

#define CODESPEED   13                            // speed in Words per Minute
#define DITPERIOD   1200/CODESPEED
#define PITCH       1000                          // pitch in Hz of morse audio
#define LED         PC13                          // onboard LED pin
#define PIEZO       PB12                          // pin attached to piezo element

byte morse[] = {                                  // Each character is encoded into an 8-bit byte:
  0b01001010,        // ! exclamation        
  0b01101101,        // " quotation          
  0b01010111,        // # pound                   // No Morse, mapped to SK
  0b10110111,        // $ dollar or ~SX
  0b00000000,        // % percent
  0b00111101,        // & ampersand or ~AS
  0b01100001,        // ' apostrophe
  0b00110010,        // ( open paren
  0b01010010,        // ) close paren
  0b0,               // * asterisk                // No Morse
  0b00110101,        // + plus or ~AR
  0b01001100,        // , comma
  0b01011110,        // - hypen
  0b01010101,        // . period
  0b00110110,        // / slant   
  0b00100000,        // 0                         // Read the bits from RIGHT to left,   
  0b00100001,        // 1                         // with a "1"=dit and "0"=dah
  0b00100011,        // 2                         // example: 2 = 11000 or dit-dit-dah-dah-dah
  0b00100111,        // 3                         // the final bit is always 1 = stop bit.
  0b00101111,        // 4                         // see "sendElements" routine for more info.
  0b00111111,        // 5
  0b00111110,        // 6
  0b00111100,        // 7
  0b00111000,        // 8
  0b00110000,        // 9
  0b01111000,        // : colon
  0b01101010,        // ; semicolon
  0b0,               // <                         // No Morse
  0b00101110,        // = equals or ~BT
  0b0,               // >                         // No Morse
  0b01110011,        // ? question
  0b01101001,        // @ at or ~AC
  0b00000101,        // A 
  0b00011110,        // B
  0b00011010,        // C
  0b00001110,        // D
  0b00000011,        // E
  0b00011011,        // F
  0b00001100,        // G
  0b00011111,        // H
  0b00000111,        // I
  0b00010001,        // J
  0b00001010,        // K
  0b00011101,        // L
  0b00000100,        // M
  0b00000110,        // N
  0b00001000,        // O
  0b00011001,        // P
  0b00010100,        // Q
  0b00001101,        // R
  0b00001111,        // S
  0b00000010,        // T
  0b00001011,        // U
  0b00010111,        // V
  0b00001001,        // W
  0b00010110,        // X
  0b00010010,        // Y 
  0b00011100         // Z
};


void ditSpaces(int spaces=1) {                    // user specifies #dits to wait
  for (int i=0; i<spaces; i++)                    // count the dits...
    delay(DITPERIOD);                             // no action, just mark time
}

void characterSpace()
{
  ditSpaces(2);                                  // 3 total (last element includes 1)
}

void wordSpace()
{
  ditSpaces(4);                                   // 7 total (last char includes 3)                      
}

void dit() {
  digitalWrite(LED,0);                            // turn on LED
  tone(PIEZO,PITCH);                              // and turn on sound
  ditSpaces();
  digitalWrite(LED,1);                            // turn off LED
  noTone(PIEZO);                                  // and turn off sound
  ditSpaces();                                    // space between code elements
}

void dah() {
  digitalWrite(LED,0);                            // turn on LED
  tone(PIEZO,PITCH);                              // and turn on sound
  ditSpaces(3);                                   // length of dah = 3 dits
  digitalWrite(LED,1);                            // turn off LED
  noTone(PIEZO);                                  // and turn off sound
  ditSpaces();                                    // space between code elements
}

// the following routine accepts a numberic input, where each bit represents a code element:
// 1=dit and 0=dah.   The elements are read right-to-left.  The left-most bit is a stop bit.
// For example:  5 = binary 0b0101.  The right-most bit is 1 so the first element is a dit.
// The next element to the left is a 0, so a dah follows.  Only a 1 remains after that, so
// the character is complete: dit-dah = morse character 'A'.

void sendElements(int x) {                        // send string of bits as Morse      
  while (x>1) {                                   // stop when value is 1 (or less)
    if (x & 1) dit();                             // right-most bit is 1, so dit
    else dah();                                   // right-most bit is 0, so dah
    x >>= 1;                                      // rotate bits right
  }
  characterSpace();                               // add inter-character spacing
}

void sendCharacter(char c) {                      // send a single ASCII character in Morse
  if (c<32) return;                               // ignore control characters
  if (c>96) c -= 32;                              // convert lower case to upper case
  if (c>90) return;                               // not a character
  if (c==32) wordSpace();                         // space between words 
  else sendElements(morse[c-33]);                 // send the character
}

void sendString (char *ptr) {             
  while (*ptr)                                    // send the entire string
    sendCharacter(*ptr++);                        // one character at a time
}

void sendBook()
{ 
  File book = SD.open(TEXTFILE);                  // look for book on sd card
  if (book) {                                     // find it? 
    while (book.available()) {                    // do for all characters in book:                
      char ch = book.read();                      // get next character
      sendCharacter(ch);                          // and send it
    }
    book.close();                                 // close the file
  } 
  else sendString("CANT OPEN FILE");
}

void setup() {
  pinMode(LED,OUTPUT);
  SD.begin(SD_CS);                                // initialize SD library
  sendBook();
}

void loop() {
}
