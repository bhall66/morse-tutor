/**************************************************************************
      Author:   Bruce E. Hall, w8bh.net
        Date:   02 Sep 2019
    Hardware:   STM32F103C "Blue Pill", Piezo Buzzer, 2.2" ILI9341 LCD,
                Adafruit #477 rotary encoder or similar
    Software:   Arduino IDE 1.8.9; stm32duino package @ dan.drown.org
       Legal:   Copyright (c) 2019  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Part 5 of the tutorial at w8bh.net
                Practice sending & receiving morse code
                Inspired by Jack Purdum's "Morse Code Tutor"
                
 **************************************************************************/

//===================================  INCLUDES ========================================= 
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

//===================================  Hardware Connections =============================
#define TFT_DC            PA0                     // Display "DC" pin
#define TFT_CS            PA1                     // Display "CS" pin
#define TFT_MOSI          PA7                     // Display "MOSI" pin
#define TFT_SCK           PA5                     // Display "SCK" pin    
#define ENCODER_A         PA9                     // Rotary Encoder output A
#define ENCODER_B         PA8                     // Rotary Encoder output B
#define ENCODER_BUTTON    PB15                    // Rotary Encoder switch
#define PADDLE_A          PB8                     // Morse Paddle "dit"
#define PADDLE_B          PB7                     // Morse Paddle "dah"
#define AUDIO             PA2                     // Audio output
#define LED               PC13                    // onboard LED pin

//===================================  Morse Code Constants =============================
#define CODESPEED          13                     // speed in Words per Minute
#define PITCH            1200                     // pitch in Hz of morse audio
#define DITPERIOD   1200/CODESPEED                // period of dit, in milliseconds
#define WORDSIZE            5                     // number of chars per random word
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

//===================================  Rotary Encoder Variables =========================
volatile int      rotary_counter   = 0;           // "position" of rotary encoder (increments CW) 
volatile boolean  rotary_changed   = false;       // true if rotary_counter has changed
volatile boolean  button_pressed   = false;       // true if the button has been pushed
volatile boolean  button_released  = false;       // true if the button has been released (sets button_downtime)
volatile long     button_downtime  = 0L;          // ms the button was pushed before released


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
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
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)           // break them up into "words"
      sendCharacter(randomNumber());         // send a number
    sendCharacter(' ');                      // send a space between number groups
  }
}

void sendLetters()                           // send random letters forever...
{ 
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)           // break them up into "words"
      sendCharacter(randomLetter());         // send the letter 
    sendCharacter(' ');                      // send a space between words
  }
}

void sendMixedChars()                        // send letter/number groups...
{ 
  while (!button_pressed) {                            
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
  while (!button_pressed) {
    int index=random(0, ELEMENTS(hamWords)); // eeny, meany, miney, moe
    sendString(hamWords[index]);             // send the word
    sendCharacter(' ');                      // and a space between words
  }
}

void sendCommonWords()
{ 
  while (!button_pressed) {
    sendString(commonWords);                 // one long string of 100 words
  }
}

void sendCallsigns()                         // send random US callsigns
{ 
  char call[8];                              // need string to stuff callsign into
  while (!button_pressed) {
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
   Rotary Encoder "A" Interrupt Service Routine ---------------
   This function will runs when encoder pin A changes state. The rotary "position" is held 
   in rotary_counter, increasing for CW rotation, decreasing for CCW rotation. If the 
   position changes, rotary_change will be set true.  
*/
  
void rotaryISR_A()
{
  static uint8_t rotary_state = 0;                // holds current and previous encoder states   

  rotary_state <<= 2;                             // shift previous state up 2 bits
  rotary_state |= (digitalRead(ENCODER_A));       // put encoder_A on bit 0
  rotary_state |= (digitalRead(ENCODER_B) << 1);  // put encoder_B on bit 1
  rotary_state &= 0x0F;                           // zero upper 4 bits

  if ((rotary_state == 0x09) ||                   // 9 = binary 1001 = 01 to 10 transition.
     (rotary_state == 0x06))                      // 6 = binary 0110 = 10 to 01 transition.
  {
    rotary_counter++;                             // CW rotation: increment counter 
    rotary_changed = true;
  }
  else if ((rotary_state == 0x03) ||              // 3 = binary 0011 = 11 to 00 transition.
      (rotary_state == 0x0C))                     // C = binary 1100 = 00 to 11 transition.
  {
    rotary_counter--;                             // CCW rotation: decrement counter
    rotary_changed = true;
  }
}

/* 
   Rotary Encoder "AB" Interrupt Service Routine ---------------
   This is an alternative to rotaryISR() above.
   It gives twice the resolution at a cost of using an additional interrupt line
   This function will runs when either encoder pin A or B changes state.
   The states array maps each transition 0000..1111 into CW/CCW rotation (or invalid).
   The rotary "position" is held in rotary_counter, increasing for CW rotation, decreasing 
   for CCW rotation. If the position changes, rotary_change will be set true. 
   You should set this to false after handling the change.
   To implement, attachInterrupts to encoder pin A *and* pin B 
*/

 
void rotaryISR()
{
  const int states[] = {0,1,-1,0,-1,0,0,1,1,0,0,-1,0,-1,1,0};
  static byte rotary_state = 0;                   // holds current and previous encoder states   

  rotary_state <<= 2;                             // shift previous state up 2 bits
  rotary_state |= (digitalRead(ENCODER_A));       // put encoder_A on bit 0
  rotary_state |= (digitalRead(ENCODER_B) << 1);  // put encoder_B on bit 1
  rotary_state &= 0x0F;                           // zero upper 4 bits

  int change = states[rotary_state];              // map transition to CW vs CCW rotation
  if (change!=0)                                  // make sure transition is valid
  {
    rotary_changed = true;                          
    rotary_counter += change;                     // update rotary counter +/- 1
  }
}

void initEncoder()
{
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_BUTTON),buttonISR,CHANGE);   
  attachInterrupt(digitalPinToInterrupt(ENCODER_A),rotaryISR,CHANGE); 
  attachInterrupt(digitalPinToInterrupt(ENCODER_B),rotaryISR,CHANGE); 
}

void setup() {
  initEncoder();
  pinMode(LED,OUTPUT);
  pinMode(PADDLE_A, INPUT_PULLUP);                // two paddle inputs, both active low
  pinMode(PADDLE_B, INPUT_PULLUP);
  tft.begin();                                    // initialize screen object
  tft.setRotation(3);                             // landscape mode: use '1' or '3'
  tft.fillScreen(BLACK);                          // start with blank screen
  tft.setTextSize(2);                             // small text but readable
  tft.setTextColor(TEXTCOLOR,BLACK);
}

void loop() {                              
  sendLetters();      button_pressed = false; 
  sendNumbers();      button_pressed = false;
  sendHamWords();     button_pressed = false;  
  sendMixedChars();   button_pressed = false; 
  sendCallsigns();    button_pressed = false;  
}
