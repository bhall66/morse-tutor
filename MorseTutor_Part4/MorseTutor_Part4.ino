/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   11 Jul 2019
    Hardware:   STM32F103C "Blue Pill", Piezo Buzzer, 2.2" ILI9341 LCD
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
       Legal:   Copyright (c) 2019  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Part 4 of the tutorial at w8bh.net
                Practice sending & receiving morse code
                Inspired by Jack Purdum's "Morse Code Tutor"
   
 **************************************************************************/

//===================================  INCLUDES ========================================= 
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

//===================================  Hardware Connections =============================
#define TFT_DC            PA0                     // LCD "DC" pin
#define TFT_CS            PA1                     // LCD "CS" pin
#define TFT_RST           PA2                     // LCD "RST" pin
#define LED              PC13                     // onboard LED pin
#define PADDLE_A         PB14                     // Morse Paddle "dit"
#define PADDLE_B         PB13                     // Morse Paddle "dah"
#define PIEZO            PB12                     // pin attached to piezo element

//===================================  Morse Code Constants =============================
#define CODESPEED   13                            // speed in Words per Minute
#define PITCH       1200                          // pitch in Hz of morse audio
#define DITPERIOD   1200/CODESPEED                // period of dit, in milliseconds
#define WORDSIZE    5                             // number of chars per random word
#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))

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
#define ROWSPACING         25                     // Height in pixels for each text row
#define COLSPACING         12                     // Width in pixels for each text character
#define MAXCOL   DISPLAYWIDTH/COLSPACING          // Number of characters per row    
#define MAXROW   DISPLAYHEIGHT/ROWSPACING         // Number of text-rows per screen
#define TEXTCOLOR      YELLOW                     // Default non-menu text color


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
int textRow=0, textCol=0;

char *greetings   = "Greetings de W8BH = Name is Bruce = +";
char *commonWords = "a an the this these that some all any every who which what such other I me my we us our you your he him his she her it its they them their man men people time work well May will can one two great little first at by on upon over before to from with in into out for of about up when then now how so like as well very only no not more there than and or if but be am is are was were been has have had may can could will would shall should must say said like go come do made work";
char *hamWords[]  = {"DE", "TNX FER", "BT", "WX", "HR", "TEMP", "ES", "RIG", "ANT", "DIPOLE", "VERTICAL", // 0-10
                    "BEAM", "HW", "CPI", "WARM", "SUNNY", "CLOUDY", "COLD", "RAIN", "SNOW", "FOG",       // 11-20
                    "RPT", "NAME", "QTH", "CALL", "UR", "SLD", "FB", "RST"                               // 21-28
                    };
char prefix[]     = {'A', 'W', 'K', 'N'};

const byte morse[] = {                       // Each character is encoded into an 8-bit byte:
  0b01001010,        // ! exclamation        // Some punctuation not commonly used
  0b01101101,        // " quotation          // and taken for wikipedia: International Morse Code
  0b0,               // # pound
  0b10110111,        // $ dollar
  0b00000000,        // % percent
  0b00111101,        // & ampersand
  0b01100001,        // ' apostrophe
  0b00110010,        // ( open paren
  0b01010010,        // ) close paren
  0b0,               // * asterisk
  0b00110101,        // + plus or ~AR
  0b01001100,        // , comma
  0b01011110,        // - hypen
  0b01010101,        // . period
  0b00110110,        // / slant   
  0b00100000,        // 0                    // Read the bits from RIGHT to left,   
  0b00100001,        // 1                    // with a "1"=dit and "0"=dah
  0b00100011,        // 2                    // example: 2 = 11000 or dit-dit-dah-dah-dah
  0b00100111,        // 3                    // the final bit is always 1 = stop bit.
  0b00101111,        // 4                    // see "sendElements" routine for more info.
  0b00111111,        // 5
  0b00111110,        // 6
  0b00111100,        // 7
  0b00111000,        // 8
  0b00110000,        // 9
  0b01111000,        // : colon
  0b01101010,        // ; semicolon
  0b0,               // <
  0b00101110,        // = equals or ~BT
  0b0,               // >
  0b01110011,        // ? question
  0b01101001,        // @ at
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

void ditSpaces(int spaces=1) {               // user specifies #dits to wait
  for (int i=0; i<spaces; i++)               // count the dits...
    delay(DITPERIOD);                        // no action, just mark time
}

void wordSpace() {
  ditSpaces(4);                              // 7 total (3 in char)
}

void characterSpace() {
  ditSpaces(2);                              // 3 total (1 in last element)
}


void dit() {
  digitalWrite(LED,0);                       // turn on LED
  tone(PIEZO,PITCH);                         // and turn on sound
  ditSpaces();                               // wait for period of 1 dit
  digitalWrite(LED,1);                       // turn off LED
  noTone(PIEZO);                             // and turn off sound
  ditSpaces();                               // space between code elements
}

void dah() {
  digitalWrite(LED,0);                       // turn on LED
  tone(PIEZO,PITCH);                         // and turn on sound
  ditSpaces(3);                              // length of dah = 3 dits
  digitalWrite(LED,1);                       // turn off LED
  noTone(PIEZO);                             // and turn off sound
  ditSpaces();                               // space between code elements
}

// the following routine accepts a numberic input, where each bit represents a code element:
// 1=dit and 0=dah.   The elements are read right-to-left.  The left-most bit is a stop bit.
// For example:  5 = binary 0b0101.  The right-most bit is 1 so the first element is a dit.
// The next element to the left is a 0, so a dah follows.  Only a 1 remains after that, so
// the character is complete: dit-dah = morse character 'A'.

void sendElements(int x) {                   // send string of bits as Morse      
  while (x>1) {                              // stop when value is 1 (or less)
    if (x & 1) dit();                        // right-most bit is 1, so dit
    else dah();                              // right-most bit is 0, so dah
    x >>= 1;                                 // rotate bits right
  }
  characterSpace();                          // add inter-character spacing
}

void sendCharacter(char c) {                 // send a single ASCII character in Morse
  if (c<32) return;                          // ignore control characters
  if (c>96) c -= 32;                         // convert lower case to upper case
  if (c>90) return;                          // not a character
  addCharacter(c);                           // display character on LCD
  if (c==32) wordSpace();                    // space between words 
  else sendElements(morse[c-33]);            // send the character
}

void sendString (char *ptr) {             
  while (*ptr)                               // send the entire string
    sendCharacter(*ptr++);                   // one character at a time
}

void addChar (char* str, char ch)            // adds 1 character to end of string
{                                            
  char c[2] = " ";                           // happy hacking: char into string
  c[0] = ch;                                 // change char'A' to string"A"
  strcat(str,c);                             // and add it to end of string
}

char randomLetter()                          // returns a random uppercase letter
{
  return 'A'+random(0,26);
}

char randomNumber()                          // returns a random single-digit # 0-9
{
  return '0'+random(0,10);
}


void createCallsign(char* call)              // returns with random US callsign in "call"
{  
  strcpy(call,"");                           // start with empty callsign                      
  int i = random(0, 4);                      // 4 possible start letters for US                                       
  char c = prefix[i];                        // Get first letter of prefix
  addChar(call,c);                           // and add it to callsign
  i = random(0,3);                           // single or double-letter prefix?
  if ((i==2) or (c=='A'))                    // do a double-letter prefix
  {                                          // allowed combinations are:
    if (c=='A') i=random(0,12);              // AA-AL, or
    else i=random(0,26);                     // KA-KZ, NA-NZ, WA-WZ 
    addChar(call,'A'+i);                     // add second char to prefix                                                         
  }
  addChar(call,randomNumber());              // add zone number to callsign
  for (int i=0; i<random(1, 4); i++)         // Suffix contains 1-3 letters
    addChar(call,randomLetter());            // add suffix letter(s) to call
}

void sendNumbers()                           // send random numbers forever...
{ 
  while (true) {
    for (int i=0; i<WORDSIZE; i++)           // break them up into "words"
      sendCharacter(randomNumber());         // send a number
    sendCharacter(' ');                      // send a space between number groups
  }
}

void sendLetters()                           // send random letters forever...
{ 
  while (true) {
    for (int i=0; i<WORDSIZE; i++)           // break them up into "words"
      sendCharacter(randomLetter());         // send the letter 
    sendCharacter(' ');                      // send a space between words
  }
}

void sendMixedChars()                        // send letter/number groups...
{ 
  while (true) {                            
    for (int i=0; i<WORDSIZE; i++)           // break them up into "words"
    {
      int c = '0' + random(43);              // pick a random character
      sendCharacter(c);                      // and send it
    }
    sendCharacter(' ');                      // send a space between words
  }
}

void sendHamWords()                          // send some common ham words
{ 
  while (true) {
    int index=random(0, ELEMENTS(hamWords)); // eeny, meany, miney, moe
    sendString(hamWords[index]);             // send the word
    sendCharacter(' ');                      // and a space between words
  }
}

void sendCommonWords()
{ 
  while (true) {
    sendString(commonWords);                 // one long string of 100 words
  }
}

void sendCallsigns()                         // send random US callsigns
{ 
  char call[8];                              // need string to stuff callsign into
  while (true) {
    createCallsign(call);                    // make it
    sendString(call);                        // and send it
    sendCharacter(' ');
  }
}

bool ditPressed()
{
  return (digitalRead(PADDLE_A)==0);         // pin is active low
}

bool dahPressed()
{
  return (digitalRead(PADDLE_B)==0);         // pin is active low
}

void doPaddles()
{
  while (true) {
    if (ditPressed())  dit();                // user wants a dit, so do it.
    if (dahPressed())  dah();                // user wants a dah, so do it.
  }
}

void clearScreen() {
  tft.fillScreen(BLACK);                         // yes, so clear screen
  textRow=0; textCol=0;                          // and start on first row  
}

void showCharacter(char c, int row, int col)      // display a character at given row & column
{
  int x = col * COLSPACING;                       // convert column to x coordinate
  int y = row * ROWSPACING;                       // convert row to y coordinate
  tft.setCursor(x,y);                             // position character on screen
  tft.print(c);                                   // and display it 
}

void addCharacter(char c)
{
  showCharacter(c,textRow,textCol);               // display character & current row/col position
  textCol++;                                      // go to next position on the current row
  if ((textCol>=MAXCOL) ||                        // are we at end of the row?
     ((c==' ') && (textCol>MAXCOL-7)))            // or at a wordspace thats near end of row?
  {
    textRow++; textCol=0;                         // yes, so advance to beginning of next row
    if (textRow >= MAXROW)                        // but have we run out of rows?
    {
      clearScreen();                              // start at top again
    }
  }
}

void addBunchOfCharacters()                       // just for testing...
{
  for (int i=0; i<200; i++)
    addCharacter(randomLetter());
}

void setup() {
  pinMode(LED,OUTPUT);
  pinMode(PADDLE_A, INPUT_PULLUP);                // two paddle inputs, both active low
  pinMode(PADDLE_B, INPUT_PULLUP);
  tft.begin();                                    // initialize screen object
  tft.setRotation(3);                             // landscape mode: use '1' or '3'
  tft.fillScreen(BLACK);                          // start with blank screen
  tft.setTextSize(2);                             // small text but readable
  tft.setTextColor(TEXTCOLOR,BLACK);
  //tft.print("Hello from W8BH");                 // for testing only

  //uncomment any of the routines below to try them out...
  
  //sendWords();                               
  //sendLetters();
  //sendNumbers();
  //sendMixedChars();
  //sendCallsigns();
  //sendCommonWords();
  //addBunchOfCharacters();
}

void loop() {
  sendString(greetings);                     // just in case you didn't try anything above.
  delay(5000);
  clearScreen();                                 // start again!
}
