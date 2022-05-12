/**************************************************************************
       Title:   Morse Tutor ESP32						   
      Author:   Bruce E. Hall, w8bh.net
        Date:   07 Jan 2022
    Hardware:   ESP32 DevBoard "HiLetGo", ILI9341 TFT display
    Software:   Arduino IDE 1.8.19
                ESP32 by Expressif Systems 1.0.6
                Adafruit GFX Library 1.5.3
                Adafruit ILI9341 1.5.6
       Legal:   Copyright (c) 2020  Bruce E. Hall.
                Open Source under the terms of the MIT License. 
    
 Description:   Practice sending & receiving morse code
                Inspired by Jack Purdum's "Morse Code Tutor"
                
                >> THIS VERSION UNDER DEVELOPMENT <<<

 **************************************************************************/


//===================================  INCLUDES ========================================= 
#include "Adafruit_GFX.h"                         // Version 1.5.3
#include "Adafruit_ILI9341.h"                     // Version 1.5.6
#include "EEPROM.h"
#include "SD.h"
#include "esp_now.h"
#include "WiFi.h"

//===================================  Hardware Connections =============================
#define TFT_DC             21                     // Display "DC" pin
#define TFT_CS             05                     // Display "CS" pin
#define SD_CS              17                     // SD card "CS" pin
#define ENCODER_A          16                     // Rotary Encoder output A
#define ENCODER_B           4                     // Rotary Encoder output B
#define ENCODER_BUTTON     15                     // Rotary Encoder switch
#define PADDLE_A           32                     // Morse Paddle "dit"
#define PADDLE_B           33                     // Morse Paddle "dah"
#define AUDIO              13                     // Audio output
#define LED                 2                     // onboard LED pin
#define SCREEN_ROTATION     3                     // landscape mode: use '1' or '3'

//===================================  Wireless Constants ===============================
#define CHANNEL             1                     // Wifi channel number
#define WIFI_SSID      "W8BH Tutor"               // Wifi network name
#define WIFI_PWD       "9372947313"               // Wifi password
#define MAXBUFLEN         100                     // size of incoming character buffer
#define CMD_ADDME        0x11                     // request to add this unit as a peer
#define CMD_LEAVING      0x12                     // flag this unit as leaving

//===================================  Morse Code Constants =============================
#define DEFAULTSPEED       13                     // character speed in Words per Minute
#define MAXSPEED           50                     // fastest morse speed in WPM
#define MINSPEED            3                     // slowest morse speed in WPM
#define DEFAULTPITCH     1200                     // default pitch in Hz of morse audio
#define MAXPITCH         2800                     // highest allowed pitch
#define MINPITCH          300                     // how low can you go
#define WORDSIZE            5                     // number of chars per random word
#define MAXWORDSPACES      99                     // maximum word delay, in spaces
#define FLASHCARDDELAY   2000                     // wait in mS between cards
#define ENCODER_TICKS       3                     // Ticks required to register movement
#define FNAMESIZE          15                     // max size of a filename
#define MAXFILES           20                     // max number of SD files recognized
#define IAMBIC_A            1                     // Iambic Keyer Mode B 
#define IAMBIC_B            2                     // Iambic Keyer Mode A
#define LONGPRESS        1000                     // hold-down time for long press, in mSec
#define SUPPRESSLED     false                     // if true, do not flash diagnostic LED

//===================================  Color Constants ==================================
#define BLACK          0x0000
#define BLUE           0x001F
#define NAVY           0x000F               
#define RED            0xF800
#define MAROON         0x7800 
#define GREEN          0x07E0
#define LIME           0x0400
#define CYAN           0x07FF
#define TEAL           0x03EF
#define PURPLE         0x780F                
#define PINK           0xF81F
#define YELLOW         0xFFE0
#define ORANGE         0xFD20
#define BROWN          0x79E0
#define WHITE          0xFFFF
#define OLIVE          0x7BE0
#define SILVER         0xC618        
#define GRAY           0x7BEF
#define DARKGRAY       0x1208

const word colors[] = {BLACK,BLUE,NAVY,RED,MAROON,GREEN,LIME,CYAN,TEAL,PURPLE,
  PINK,YELLOW,ORANGE,BROWN,WHITE,OLIVE,SILVER,GRAY,DARKGRAY};

// ==================================  Menu Constants ===================================
#define DISPLAYWIDTH      320                     // Number of LCD pixels in long-axis
#define DISPLAYHEIGHT     240                     // Number of LCD pixels in short-axis
#define TOPMARGIN          30                     // All submenus appear below top line
#define MENUSPACING       100                     // Width in pixels for each menu column
#define ROWSPACING         23                     // Height in pixels for each text row
#define COLSPACING         12                     // Width in pixels for each text character
#define MAXCOL   DISPLAYWIDTH/COLSPACING          // Number of characters per row    
#define MAXROW  (DISPLAYHEIGHT-TOPMARGIN)/ROWSPACING  // Number of text-rows per screen
#define FG              GREEN                     // Menu foreground color 
#define BG              BLACK                     // App background color
#define SELECTFG         BLUE                     // Selected Menu foreground color
#define SELECTBG        WHITE                     // Selected Menu background color
#define TEXTCOLOR      YELLOW                     // Default non-menu text color
#define TXCOLOR         WHITE                     // color of outgoing (sending) text
#define RXCOLOR         GREEN                     // color of incoming (received) text
#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))    // Handy macro for determining array sizes

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

//===================================  Rotary Encoder Variables =========================
volatile int      rotaryCounter    = 0;           // "position" of rotary encoder (increments CW) 
volatile boolean  button_pressed   = false;       // true if the button has been pushed
volatile boolean  button_released  = false;       // true if the button has been released (sets button_downtime)
volatile long     button_downtime  = 0L;          // ms the button was pushed before released

//===================================  Morse Code Variables =============================

                    // The following is a list of the 100 most-common English words, in frequency order.
                    // See: https://www.dictionary.com/e/common-words/
                    // from The Brown Corpus Standard Sample of Present-Day American English 
                    // (Providence, RI: Brown University Press, 1979)
                    
char *words[]     = {"THE", "OF", "AND", "TO", "A", "IN", "THAT", "IS", "WAS", "HE", 
                     "FOR", "IT", "WITH", "AS", "HIS", "ON", "BE", "AT", "BY", "I", 
                     "THIS", "HAD", "NOT", "ARE", "BUT", "FROM", "OR", "HAVE", "AN", "THEY", 
                     "WHICH", "ONE", "YOU", "WERE", "ALL", "HER", "SHE", "THERE", "WOULD", "THEIR", 
                     "WE", "HIM", "BEEN", "HAS", "WHEN", "WHO", "WILL", "NO", "MORE", "IF", 
                     "OUT", "SO", "UP", "SAID", "WHAT", "ITS", "ABOUT", "THAN", "INTO", "THEM", 
                     "CAN", "ONLY", "OTHER", "TIME", "NEW", "SOME", "COULD", "THESE", "TWO", "MAY", 
                     "FIRST", "THEN", "DO", "ANY", "LIKE", "MY", "NOW", "OVER", "SUCH", "OUR", 
                     "MAN", "ME", "EVEN", "MOST", "MADE", "AFTER", "ALSO", "DID", "MANY", "OFF", 
                     "BEFORE", "MUST", "WELL", "BACK", "THROUGH", "YEARS", "MUCH", "WHERE", "YOUR", "WAY"  
                    };
char *antenna[]   = {"YAGI", "DIPOLE", "VERTICAL", "HEXBEAM", "MAGLOOP"};
char *weather[]   = {"HOT", "SUNNY", "WARM", "CLOUDY", "RAINY", "COLD", "SNOWY", "CHILLY", "WINDY", "FOGGY"};
char *names[]     = {"WAYNE", "TYE", "DARREN", "MICHAEL", "SARAH", "DOUG", "FERNANDO", "CHARLIE", "HOLLY",
                     "KEN", "SCOTT", "DAN", "ERVIN", "GENE", "PAUL", "VINCENT"};
char *cities[]    = {"DAYTON, OH", "HADDONFIELD, NJ", "MURRYSVILLE, PA", "BALTIMORE, MD", "ANN ARBOR, MI", 
                     "BOULDER, CO", "BILLINGS, MT", "SANIBEL, FL", "CIMMARON, NM", "TYLER, TX", "OLYMPIA, WA"};
char *rigs[]      = {"YAESU FT101", "KENWOOD 780", "ELECRAFT K3", "HOMEBREW", "QRPLABS QCX", "ICOM 7410", "FLEX 6400"};
char punctuation[]= "!@$&()-+=,.:;'/";
char prefix[]     = {'A', 'W', 'K', 'N'};
char koch[]       = "KMRSUAPTLOWI.NJEF0Y,VG5/Q9ZH38B?427C1D6X";
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

int charSpeed   = DEFAULTSPEED;                   // speed at which characters are sent, in WPM
int codeSpeed   = DEFAULTSPEED;                   // overall code speed, in WPM
int ditPeriod   = 100;                            // length of a dit, in milliseconds
int ditPaddle   = PADDLE_A;                       // digital pin attached to dit paddle
int dahPaddle   = PADDLE_B;                       // digital pin attached to dah paddle
int pitch       = DEFAULTPITCH;                   // frequency of audio output, in Hz
int kochLevel   = 1;                              // current Koch lesson #
int score       = 0;                              // copy challange score
int hits        = 0;                              // copy challange correct #
int misses      = 0;                              // copy channange incorrect #
int xWordSpaces = 0;                              // extra spaces between words
int keyerMode   = IAMBIC_B;                       // current keyer mode
bool usePaddles = false;                          // if true, using paddles; if false, straight key
bool paused     = false;                          // if true, morse output is paused
bool ditRequest = false;                          // dit memory for iambic sending
bool dahRequest = false;                          // dah memory for iambic sending
bool inStartup  = true;                           // startup flag
char myCall[10] = "W8BH";
int textColor   = TEXTCOLOR;                      // foreground (text) color
int bgColor     = BG;                             // background (screen) color
int brightness  = 100;                            // backlight level (range 0-100%)
int startItem   = 0;                              // startup activity.  0 = main menu



//===================================  Menu Variables ===================================
int  menuCol=0, textRow=0, textCol=0;
char *mainMenu[] = {" Receive ", "  Send  ", "Config "};        
char *menu0[]    = {" Koch    ", " Letters ", " Words   ", " Numbers ", " Mixed   ", " SD Card ", " QSO     ", " Callsign", " Exit    "};
char *menu1[]    = {" Practice", " Copy One", " Copy Two", " Cpy Word", " Cpy Call", " Flashcrd", " Head Cpy", " Two-Way ", " Exit    "};
char *menu2[]    = {" Speed   ", " Chk Spd ", " Tone    ", " Key     ", " Callsign", " Screen  ", " Defaults", " Exit    "};


//===================================  Wireless Code  ===================================

esp_now_peer_info_t peer;                        // holds information about peer
char buf[MAXBUFLEN];                             // buffer for incoming characters
int inPtr = 0, outPtr = 0;                       // pointer to location of next character                                  

void enQueue(char ch)
{
  buf[inPtr++] = ch;                             // add character to buffer, increment pointer
  if (inPtr==MAXBUFLEN) inPtr=0;                 // circularize it!
}

char deQueue()
{
  char ch = 0;
  if (outPtr!=inPtr)                             // return 0 if no characters available 
    ch = buf[outPtr++];                          // get character and increment pointer
  if (outPtr==MAXBUFLEN) outPtr=0;               // circularize it!
  return ch;
}

void initESPNow()
{
  if (esp_now_init() != ESP_OK)                  // try to initialize ESPNow protocol
    ESP.restart();                               // if it didn't work, try again.
  esp_now_register_send_cb(onDataSent);          // set dataSent callback
  esp_now_register_recv_cb(onDataRecv);          // set dataRecv callback
}

void configDeviceAP()
{
  char* SSID = WIFI_SSID;                         // access point name for this device
  char* Password = WIFI_PWD;                      // password
  Serial.print("Starting Soft AP: ");
  Serial.println(WiFi.softAP(SSID,Password,CHANNEL,0)?
    "Ready": "FAILED");                           // set up WiFi access point
}

void setStatusLED (int color)
{
  const int size=20;                              // size of square LED indicator
  const int xPos=DISPLAYWIDTH-size, yPos=0;       // location = screen top-right
  tft.fillRect(xPos,yPos,size,size,color);        // fill it in with desired color
}

// callback when data is sent
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status == ESP_NOW_SEND_SUCCESS)             // was data recieved correctly?
    setStatusLED(GREEN);                          // yes, so turn LED green
  else setStatusLED(RED);                         // no, so turn LED red
}

// callback when data is received
void onDataRecv(const uint8_t *mac_add, const uint8_t *data, int data_len)
{
  if (data_len!=1) return;                        // only accept one-byte packets
  setStatusLED(GREEN);                            // green status LED for data received
  Serial.print("Received data: ");
  Serial.print(*data,HEX);
  Serial.print(", '");
  Serial.println((char)*data);
  if (*data==CMD_ADDME)                           // command from unit #2: add me as a peer
    addPeer2(mac_add);                            // add unit #2 as a peer
  else if (*data==CMD_LEAVING)                    // status command from other unit: quitting
  {
    setStatusLED(RED);                            // so let user know
    Serial.println("Peer just left network");
  }
  else enQueue(*data);                            // put recieved data into queue
}

bool networkFound()                               // Scan for peers in AP mode
{
  bool result = false;
  int8_t count = WiFi.scanNetworks();             // get a list of all available networks
  for (int i = 0; i<count; i++)                   // look at each one
  {
    String SSID = WiFi.SSID(i);                   // get network's name
    String BSSIDstr = WiFi.BSSIDstr(i);           // and its Mac address
    if (SSID.indexOf("W8BH") == 0)                // is it our W8BH network?
    { 
      int mac[6];                                 // Yes, so save its information
      sscanf(BSSIDstr.c_str(),                    // parse mac address into componentss
        "%x:%x:%x:%x:%x:%x%c",  
        &mac[0], &mac[1], &mac[2], &mac[3], 
        &mac[4], &mac[5]);
      for(int j = 0; j < 6; j++)
        peer.peer_addr[j] = (uint8_t)mac[j];      // set mac address components
      peer.channel = CHANNEL;                     // set the communication channel
      peer.encrypt = 0;                           // set the encryption (none)
      result = true;
    }
  }
  WiFi.scanDelete();                              // clean up ram 
  return result;   
}

// Adds second unit as a peer, in response to AddPeer command 
void addPeer2(const uint8_t *peerMacAddress)
{
  Serial.print("Received request to add peer: ");
  peer.channel = CHANNEL;
  peer.ifidx = WIFI_IF_AP;
  peer.encrypt = 0;
  memcpy(peer.peer_addr, peerMacAddress, 6);
  if (esp_now_add_peer(&peer) == ESP_OK)          // try to add unit #2 as a peer 
  {
     setStatusLED(GREEN);                         // it worked, so turn LED green
     Serial.println("SUCCESS");   
  }
  else
  {
     setStatusLED(RED);                           // couldn't add unit #2  
     Serial.println("FAILED");  
  }
}

void addPeer()
{
  Serial.print("Attempting to join network: ");
  const uint8_t *peer_addr = peer.peer_addr;
  bool exists = esp_now_is_peer_exist(peer_addr);
  if(!exists) 
  {
    esp_err_t result = esp_now_add_peer(&peer);                      // attempt pair with unit #1
    if (result==ESP_OK) Serial.println("SUCCESS");
    else if (result==ESP_ERR_ESPNOW_NOT_FOUND) Serial.println("NOT FOUND");
    else Serial.println((int)result);
  }

}

void sendWireless(uint8_t data)
{
  esp_err_t result;
  Serial.print("Sending data: ");
  Serial.print(data,HEX);
  Serial.print(", '");
  Serial.print((char)data);
  const uint8_t *peer_addr = peer.peer_addr;
  result = esp_now_send(peer_addr, &data, sizeof(data));   // send data to peer
  Serial.print(" - result ");
  Serial.println((int)result,HEX);
}

void sendAddPeerCmd()
{
  Serial.println("Now asking peer to add me");
  delay(500); 
  sendWireless(CMD_ADDME);                        // send message to other unit:
  delay(100);                                     // add me as a network peer
}

void closeWireless()
{
  setStatusLED(BLACK);                            // erase two-way status LED
  Serial.println("Telling peer I am closing");
  sendWireless(CMD_LEAVING);                      // message other unit: I am leaving
  esp_now_deinit();                               // quit esp_now
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  Serial.println("Wireless now closed.\n");
}

void initWireless()
{
  Serial.println("Initializing wireless now");
  Serial.print("My MAC = ");                    
  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_MODE_STA);                       // set to station mode
  WiFi.disconnect();                              // disconnect from any prior APs
  setStatusLED(GRAY);                             // signal we are starting wireless                              
  if (networkFound())                             // any W8BH network?
  {                                               // yes, so we are unit #2:
    Serial.println("W8BH network found");
    initESPNow();                                 // start ESP-NOW protocol
    addPeer();                                    // add unit #1 as a peer
    Serial.print("Peer MAC = ");
    for(int j = 0; j < 6; j++)
    {
      Serial.print((uint8_t) peer.peer_addr[j], HEX);
      if (j != 5) Serial.print(":");
    }
    Serial.println(); 
    sendAddPeerCmd();                             // tell unit #1 to add me
  }
  else                                            // no, so we are unit #1:
  {  
    Serial.println("No network found");
    WiFi.mode(WIFI_AP);                           // set device in AP mode
    configDeviceAP();                             // set AP name & password
    initESPNow();                                 // and set up ESP-NOW
    Serial.println("Waiting for connection");
  }
}  



//===================================  Rotary Encoder Code  =============================

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
   Rotary Encoder Interrupt Service Routine ---------------
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
  static byte transition = 0;                     // holds current and previous encoder states   
  transition <<= 2;                               // shift previous state up 2 bits
  transition |= (digitalRead(ENCODER_A));         // put encoder_A on bit 0
  transition |= (digitalRead(ENCODER_B) << 1);    // put encoder_B on bit 1
  transition &= 0x0F;                             // zero upper 4 bits
  rotaryCounter += states[transition];            // update counter +/- 1 based on rotation
}

boolean buttonDown()                              // check CURRENT state of button
{
  return (digitalRead(ENCODER_BUTTON)==LOW);
}

void waitForButtonRelease()
{
  if (buttonDown())                               // make sure button is currently pressed.
  {
    button_released = false;                      // reset flag
    while (!button_released) ;                    // and wait for release                     
  }
}

void waitForButtonPress()
{
  if (!buttonDown())                              // make sure button is not pressed.
  {
    while (!button_pressed) ;                     // wait for press
    button_pressed = false;                       // and reset flag
  }  
}

bool longPress()
{
  waitForButtonRelease();                         // wait until button is released
  return (button_downtime > LONGPRESS);           // then check time is was held down
}

/* 
   readEncoder() returns 0 if no significant encoder movement since last call,
   +1 if clockwise rotation, and -1 for counter-clockwise rotation
*/

int readEncoder(int numTicks = ENCODER_TICKS) 
{
  static int prevCounter = 0;                     // holds last encoder position
  int change = rotaryCounter - prevCounter;       // how many ticks since last call?
  if (abs(change)<=numTicks)                      // not enough ticks?
    return 0;                                     // so exit with a 0.
  prevCounter = rotaryCounter;                    // enough clicks, so save current counter values
  return (change>0) ? 1:-1;                       // return +1 for CW rotation, -1 for CCW   
}


//===================================  Morse Routines ===================================

void keyUp()                                      // device-dependent actions 
{                                                 // when key is up:
  digitalWrite(LED,0);                            // turn off LED
  ledcWrite(0,0);                                 // and turn off sound
}

void keyDown()                                    // device-dependent actions
{                                                 // when key is down:
  if (!SUPPRESSLED) digitalWrite(LED,1);          // turn on LED
  ledcWriteTone(0,pitch);                         // and turn on sound
}

bool ditPressed()
{
  return (digitalRead(ditPaddle)==0);             // pin is active low
}

bool dahPressed()
{
  return (digitalRead(dahPaddle)==0);             // pin is active low
}

int extracharDit()
{
  return (3158/codeSpeed) - (1958/charSpeed); 
}

int intracharDit()
{
  return (1200/charSpeed);
}

void characterSpace()
{
  int fudge = (charSpeed/codeSpeed)-1;            // number of fast dits needed to convert
  delay(fudge*ditPeriod);                         // single intrachar dit to slower extrachar dit
  delay(2*extracharDit());                        // 3 total (last element includes 1)
}

void wordSpace()
{
  delay(4*extracharDit());                        // 7 total (last char includes 3)
  if (xWordSpaces)                                // any user-specified delays?
    delay(7*extracharDit()*xWordSpaces);          // yes, so additional wait between words
}

void dit() {                                      // send a dit
  ditRequest = false;                             // clear any pending request
  keyDown();                                      // turN on sound & led
  int finished = millis()+ditPeriod;              // wait for duration of dit
  while (millis()<finished) {                     // check dah paddle while dit sounding 
    if ((keyerMode==IAMBIC_B) && dahPressed())    // if iambic B & dah was pressed,    
      dahRequest = true;                          // request it for next element
  }
  keyUp();                                        // turn off sound & led
  finished = millis()+ditPeriod;                  // wait before next element sent
  while (millis()<finished) {                     // check dah paddle while waiting
    if (keyerMode && dahPressed())                // if iambic A or B & dah was pressed,
      dahRequest = true;                          // request it for next element
  }
}

void dah() {                                      // send a dah
  dahRequest = false;                             // clear any pending request
  keyDown();                                      // turn on sound & led
  int finished = millis() + ditPeriod*3;          // wait for duration of dah
  while (millis()<finished) {                     // check dit paddle while dah sounding
    if ((keyerMode==IAMBIC_B) && ditPressed())    // if iambic B & dit was pressed    
      ditRequest = true;                          // request it for next element 
  }
  keyUp();                                        // turn off sound & led
  finished = millis()+ditPeriod;                  // wait before next element sent
  while (millis()<finished) {                     // check dit paddle while waiting
    if (keyerMode && ditPressed())                // if iambic A or B & dit was pressed,
      ditRequest = true;                          // request it for next element
  }
}

void sendElements(int x) {                        // send string of bits as Morse      
  while (x>1) {                                   // stop when value is 1 (or less)
    if (x & 1) dit();                             // right-most bit is 1, so dit
    else dah();                                   // right-most bit is 0, so dah
    x >>= 1;                                      // rotate bits right
  }
  characterSpace();                               // add inter-character spacing
}

void roger() {
  dit(); dah(); dit();                            // audible (only) confirmation
}

/* 
   The following routine accepts a numberic input, where each bit represents a code element:
   1=dit and 0=dah.   The elements are read right-to-left.  The left-most bit is a stop bit.
   For example:  5 = binary 0b0101.  The right-most bit is 1 so the first element is a dit.
   The next element to the left is a 0, so a dah follows.  Only a 1 remains after that, so
   the character is complete: dit-dah = morse character 'A'. 
*/

void sendCharacter(char c) {                      // send a single ASCII character in Morse
  if (button_pressed) return;                     // user wants to quit, so vamoose
  if (c<32) return;                               // ignore control characters
  if (c>96) c -= 32;                              // convert lower case to upper case
  if (c>90) return;                               // not a character
  addCharacter(c);                                // display character on LCD
  if (c==32) wordSpace();                         // space between words 
  else sendElements(morse[c-33]);                 // send the character
  checkForSpeedChange();                          // allow change in speed while sending
  do {
    checkPause();
  } while (paused);                               // allow user to pause morse output
}

void sendString (char *ptr) {             
  while (*ptr)                                    // send the entire string
    sendCharacter(*ptr++);                        // one character at a time
}

void sendMorseWord (char *ptr) {             
  while (*ptr)                                    // send the entire word
    sendElements(morse[*ptr++ - 33]);             // each char in Morse - no display
}

void displayWPM ()
{
  const int x=290,y=200;
  tft.fillRect(x,y,24,20,BLACK);
  tft.setCursor(x,y);
  tft.print(codeSpeed); 
}

void checkForSpeedChange()
{
  int dir = readEncoder();
  if (dir!=0)
  {
    bool farnsworth=(codeSpeed!=charSpeed);
    codeSpeed += dir;
    if (codeSpeed<MINSPEED) 
      codeSpeed=MINSPEED;
    if (farnsworth)
    {
      if (codeSpeed>=charSpeed) 
        codeSpeed=charSpeed-1;
    } else {
      if (codeSpeed>MAXSPEED) 
        codeSpeed=MAXSPEED;
      charSpeed = codeSpeed;      
    }
    displayWPM();
    ditPeriod = intracharDit();
  }  
}

void checkPause()
{
  bool userKeyed=(ditPressed() ^ dahPressed());   // did user press a key (not both)
  if (!paused && userKeyed)                       // was it during sending?
  {
    paused = true;                                // yes, so pause output
    delay(ditPeriod*10);                          // for at least for 10 dits
  }
  else if ((paused && userKeyed)                  // did user key during pause
  || button_pressed)                              // or cancel?
    paused = false;                               // yes, so resume output
}


//===================================  Koch Method  =====================================

void sendKochLesson(int lesson)                   // send letter/number groups...
{ 
  const int maxCount = 175;                       // full screen = 20 x 9
  int charCount = 0;
  newScreen();                                    // start with empty screen
  while (!button_pressed && 
  (charCount < maxCount))                         // full screen = 1 lesson 
  {                            
    for (int i=0; i<WORDSIZE; i++)                // break them up into "words"
    {
      int c = koch[random(lesson+1)];             // pick a random character
      sendCharacter(c);                           // and send it
      charCount ++;                               // keep track of #chars sent
    }
    sendCharacter(' ');                           // send a space between words
  }
}

void introLesson(int lesson)                      // helper fn for getLessonNumber()
{
  newScreen();                                    // start with clean screen
  tft.print("You are in lesson ");
  tft.println(lesson);                            // show lesson number
  tft.println("\nCharacters: ");                  
  tft.setTextColor(CYAN);
  for (int i=0; i<=lesson; i++)                   // show characters in this lession
  {
    tft.print(koch[i]);
    tft.print(" ");
  }
  tft.setTextColor(textColor);
  tft.println("\n\nPress <dit> to begin");  
}

int getLessonNumber()
{
  int lesson = kochLevel;                         // start at current level
  introLesson(lesson);                            // display lesson number
  while (!button_pressed && !ditPressed())
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      lesson += dir;                              // ...so change speed up/down 
      if (lesson<1) lesson = 1;                   // dont go below 1 
      if (lesson>kochLevel) lesson = kochLevel;   // dont go above maximum
      introLesson(lesson);                        // show new lesson number
    }
  }
  return lesson;
}

void sendKoch()
{
  while (!button_pressed) {
    setTopMenu("Koch lesson");
    int lesson = getLessonNumber();               // allow user to select lesson
    if (button_pressed) return;                   // user quit, so sad
    sendKochLesson(lesson);                       // do the lesson                      
    setTopMenu("Get 90%? Dit=YES, Dah=NO");       // ask user to score lesson
    while (!button_pressed) {                     // wait for user response
       if (ditPressed())                          // dit = user advances to next level
       {  
         roger();                                 // acknowledge success
         if (kochLevel<ELEMENTS(koch))        
           kochLevel++;                           // advance to next level
         saveConfig();                            // save it in EEPROM
         delay(1000);                             // give time for user to release dit
         break;                                   // go to next lesson
       }
       if (dahPressed()) break;                   // dah = repeat same lesson
    }
  }
}


//===================================  Receive Menu  ====================================


void addChar (char* str, char ch)                 // adds 1 character to end of string
{                                            
  char c[2] = " ";                                // happy hacking: char into string
  c[0] = ch;                                      // change char 'A' to string "A"
  strcat(str,c);                                  // and add it to end of string
}

char randomLetter()                               // returns a random uppercase letter
{
  return 'A'+random(0,26);
}

char randomNumber()                               // returns a random single-digit # 0-9
{
  return '0'+random(0,10);
}

void randomCallsign(char* call)                   // returns with random US callsign in "call"
{  
  strcpy(call,"");                                // start with empty callsign                      
  int i = random(0, 4);                           // 4 possible start letters for US                                       
  char c = prefix[i];                             // Get first letter of prefix
  addChar(call,c);                                // and add it to callsign
  i = random(0,3);                                // single or double-letter prefix?
  if ((i==2) or (c=='A'))                         // do a double-letter prefix
  {                                               // allowed combinations are:
    if (c=='A') i=random(0,12);                   // AA-AL, or
    else i=random(0,26);                          // KA-KZ, NA-NZ, WA-WZ 
    addChar(call,'A'+i);                          // add second char to prefix                                                         
  }
  addChar(call,randomNumber());                   // add zone number to callsign
  for (int i=0; i<random(1, 4); i++)              // Suffix contains 1-3 letters
    addChar(call,randomLetter());                 // add suffix letter(s) to call
}

void randomRST(char* rst)
{
  strcpy(rst,"");                                 // start with empty string                     
  addChar(rst,'0'+random(3,6));                   // readability 3-5                      
  addChar(rst,'0'+random(5,10));                  // strength: 6-9 
  addChar(rst,'9');                               // tone usually 9
}

void sendNumbers()                                
{ 
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)                // break them up into "words"
      sendCharacter(randomNumber());              // send a number
    sendCharacter(' ');                           // send a space between number groups
  }
}

void sendLetters()                                
{ 
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)                // group letters into "words"
      sendCharacter(randomLetter());              // send the letter 
    sendCharacter(' ');                           // send a space between words
  }
}

void sendMixedChars()                             // send letter/number groups...
{ 
  while (!button_pressed) {                            
    for (int i=0; i<WORDSIZE; i++)                // break them up into "words"
    {
      int c = '0' + random(43);                   // pick a random character
      sendCharacter(c);                           // and send it
    }
    sendCharacter(' ');                           // send a space between words
  }
}

void sendPunctuation()
{
  while (!button_pressed) {
    for (int i=0; i<WORDSIZE; i++)                // break them up into "words"
    {
      int j=random(0,ELEMENTS(punctuation));      // select a punctuation char
      sendCharacter(punctuation[j]);              // and send it.
    }
    sendCharacter(' ');                           // send a space between words
  }
}


void sendWords()
{ 
  while (!button_pressed) {
    int index=random(0, ELEMENTS(words));         // eeny, meany, miney, moe
    sendString(words[index]);                     // send the word
    sendCharacter(' ');                           // and a space between words
  }
}

void sendCallsigns()                              // send random US callsigns
{ 
  char call[8];                                   // need string to stuff callsign into
  while (!button_pressed) {
    randomCallsign(call);                         // make it
    sendString(call);                             // and send it
    sendCharacter(' ');                           // with a space between callsigns         
  }
}

void sendQSO()
{
  char otherCall[8];
  char temp[20];                              
  char qso[300];                                  // string to hold entire QSO
  randomCallsign(otherCall);
  strcpy(qso,myCall);                             // start of QSO
  strcat(qso," de ");
  strcat(qso,otherCall);                          // another ham is calling you
  strcat(qso," K  TNX FER CALL= UR RST ");
  randomRST(temp);                                // add RST
  strcat(qso,temp);
  strcat(qso," ");
  strcat(qso,temp);                               
  strcat(qso,"= NAME HERE IS ");
  strcpy(temp,names[random(0,ELEMENTS(names))]); 
  strcat(qso,temp);                               // add name
  strcat(qso," ? ");
  strcat(qso,temp);                               // add name again
  strcat(qso,"= QTH IS ");
  strcpy(temp,cities[random(0,ELEMENTS(cities))]);
  strcat(qso,temp);                               // add QTH
  strcat(qso,"= RIG HR IS ");
  strcpy(temp,rigs[random(0,ELEMENTS(rigs))]);    // add rig
  strcat(qso,temp);                               
  strcat(qso," ES ANT IS ");                      // add antenna
  strcat(qso,antenna[random(0,ELEMENTS(antenna))]);             
  strcat(qso,"== WX HERE IS ");                   // add weather
  strcat(qso,weather[random(0,ELEMENTS(weather))]);     
  strcat(qso,"= SO HW CPY? ");                    // back to other ham
  strcat(qso,myCall);
  strcat(qso," de ");
  strcat(qso,otherCall);
  strcat(qso," KN");
  sendString(qso);                                // send entire QSO
}

int getFileList  (char list[][FNAMESIZE])         // gets list of files on SD card
{
  File root = SD.open("/");                       // open root directory on the SD card
  int count=0;                                    // count the number of files
  while (count < MAXFILES)                        // only room for so many!
  {
    File entry = root.openNextFile();             // get next file in the SD root directory
    if (!entry) break;                            // leave if there aren't any more
    if (!entry.isDirectory() &&                   // ignore directory names
    (entry.name()[0] != '_'))                     // ignore hidden "_name" Mac files
      strcpy(list[count++],&entry.name()[1]);     // add SD file to the list (ESP32: remove '/')
    entry.close();                                // close the file
  }
  root.close(); 
  return count; 
}

void displayFiles(char menu[][FNAMESIZE], int top, int itemCount)
{
  int x = 30;                                     // x-coordinate of this menu
  newScreen();                                    // clear screen below menu
  for (int i = 0; i < MAXROW; i++)                // for all items in the frame
  {
     int y = TOPMARGIN + i*ROWSPACING;            // calculate y coordinate
     int item = top + i;
     if (item<itemCount)                          // make sure item exists
       showMenuItem(menu[item],x,y,FG,BG);        // and show the item.
  } 
}

int fileMenu(char menu[][FNAMESIZE], int itemCount) // Display list of files & get user selection
{
  int index=0,top=0,pos=0,x=30,y; 
  button_pressed = false;                         // reset button flag
  displayFiles(menu,0,itemCount);                 // display as many files as possible
  showMenuItem(menu[0],x,TOPMARGIN,               // highlight first item
    SELECTFG,SELECTBG);
  while (!button_pressed)                         // exit on button press
  {
    int dir = readEncoder();                      // check for encoder movement
    if (dir)                                      // it moved!    
    {
      if ((dir>0) && (index==(itemCount-1)))      // dont try to go below last item
        continue;
      if ((dir<0) && (index==0))                  // dont try to go above first item
        continue;
      if ((dir>0) && (pos==(MAXROW-1)))           // does the frame need to move down?
      {                              
        top++;                                    // yes: move frame down,
        displayFiles(menu,top,itemCount);         // display it,
        index++;                                  // and select next item
      }
      else if ((dir<0) && (pos==0))               // does the frame need to move up?
      {
        top--;                                    // yes: move frame up,
        displayFiles(menu,top,itemCount);         // display it,
        index--;                                  // and select previous item
      }
      else                                        // we must be moving within the frame
      {
        y = TOPMARGIN + pos*ROWSPACING;           // calc y-coord of current item
        showMenuItem(menu[index],x,y,FG,BG);      // deselect current item
        index += dir;                             // go to next/prev item
      }
      pos = index-top;                            // posn of selected item in visible list
      y = TOPMARGIN + pos*ROWSPACING;             // calc y-coord of new item
      showMenuItem(menu[index],x,y,
        SELECTFG,SELECTBG);                       // select new item
    }
  }
  return index;  
}

void sendFile(char* filename)                     // output a file to screen & morse
{
  char s[FNAMESIZE] = "/";                        // ESP32: need space for whole filename
  strcat(s,filename);                             // ESP32: prepend filename with slash
  const int pageSkip = 250;                       // number of characters to skip, if asked to
  newScreen();                                    // clear screen below menu
  bool wireless = longPress();                    // if long button press, send file wirelessly
  if (wireless) initWireless();                   // start wireless transmission
  button_pressed = false;                         // reset flag for new presses
  File book = SD.open(s);                         // look for book on sd card
  if (book) {                                     // find it? 
    while (!button_pressed && book.available())   // do for all characters in book:
    {                                    
      char ch = book.read();                      // get next character
      if (ch=='\n') ch = ' ';                     // convert LN to a space
      sendCharacter(ch);                          // and send it
      if (wireless) sendWireless(ch);
      if (ditPressed() && dahPressed())           // user wants to 'skip' ahead:
      {
        sendString("=  ");                        // acknowledge the skip with ~BT
        for (int i=0; i<pageSkip; i++)
          book.read();                            // skip a bunch of text!
      }
    }
    book.close();                                 // close the file
  } 
  if (wireless) closeWireless();                  // close wireless transmission
}

void sendFromSD()                                 // show files on SD card, get user selection & send it.
{
  char list[MAXFILES][FNAMESIZE];                 // hold list of SD filenames (DOS 8.3 format, 13 char)
  int count = getFileList(list);                  // get list of files on the SD card
  int choice = fileMenu(list,count);              // display list & let user choose one
  sendFile(list[choice]);                         // output text & morse until user quits
}


//===================================  Send Menu  =======================================

void checkSpeed()
{
  long start=millis();
  sendString("PARIS ");                           // send "PARIS"                                     
  long elapsed=millis()-start;                    // see how long it took
  dit();                                          // sound out the end.
  float wpm = 60000.0/elapsed;                    // convert time to WPM
  tft.setTextSize(3);
  tft.setCursor(100,80);
  tft.print(wpm);                                 // display result
  tft.print(" WPM");
  while (!button_pressed) ;                       // wait for user
}

int decode(int code)                              // convert code to morse table index
{
  int i=0;                                        
  while (i<ELEMENTS(morse) && (morse[i]!=code))   // search table for the code
    i++;                                          // ...from beginning to end. 
  if (i<ELEMENTS(morse)) return i;                // found the code, so return index
  else return -1;                                 // didn't find the code, return -1
}

char paddleInput()                                // monitor paddles & return a decoded char                          
{
  int bit=0, code=0;
  unsigned long start = millis();
  while (!button_pressed)
  {
    if (ditRequest ||                             // dit was requested
      (ditPressed()&&!dahRequest))                // or user now pressing it             
    {
      dit();                                      // so sound it out
      code += (1<<bit++);                         // add a '1' element to code
      start = millis();                           // and reset timeout.
    }
    if (dahRequest ||                             // dah was requested
      (dahPressed()&&!ditRequest))                // or user now pressing it
    {
      dah();                                      // so sound it tout
      bit++;                                      // add '0' element to code
      start = millis();                           // and reset the timeout.
    }
    int wait = millis()-start;
    if (bit && (wait > ditPeriod))                // waited for more than a dit
    {                                             // so that must be end of character:
      code += (1<<bit);                           // add stop bit
      int result = decode(code);                  // look up code in morse array
      if (result<0) return ' ';                   // oops, didn't find it
      else return '!'+result;                     // found it! return result
    }
    if (wait > 5*ditPeriod) return ' ';           // long pause = word space                                
  }
  return ' ';                                     // return on button press
}

char straightKeyInput()                           // decode straight key input
{
  int bit=0, code=0;
  bool keying = false;
  unsigned long start,end,timeUp,timeDown,timer;
  timer = millis();                               // start character timer
  while (!button_pressed)
  {
    if (ditPressed()) timer = millis();           // dont count keydown time
    if (ditPressed() && (!keying))                // new key_down event
    {  
      start = millis();                           // mark time of key down
      timeUp = start-end;
      if (timeUp > 10)                            // was key up for 10mS?   
      {                                     
        keying = true;                            // mark key as down
        keyDown();                                // turn on sound & led 
      }
    }
    else if (!ditPressed() && keying)             // new key_up event
    {
      end = millis();                             // get time of release
      timeDown = end-start;                       // how long was key down?
      if (timeDown > 10)                          // was key down for 10mS?
      {
        keying = false;                           // not just noise: mark key as up
        keyUp();                                  // turn off sound & led
        if (timeDown > 2*ditPeriod)               // if longer than 2 dits, call it dah                  
          bit++;                                  // dah: add '0' element to code
        else code += (1<<bit++);                  // dit: add '1' element to code
      }
    }
    int wait = millis() - timer;                  // time since last element was sent
    if (bit && (wait > ditPeriod*2))              // waited for more than 2 dits
    {                                             // so that must be end of character:
      code += (1<<bit);                           // add stop bit
      int result = decode(code);                  // look up code in morse array
      if (result<0) return ' ';                   // oops, didn't find it
      else return '!'+result;                     // found it! return result
    }
    if (wait > ditPeriod*5) return ' ';           // long pause = word space                               
  }
  return ' ';                                     // return on button press   
}

char morseInput()                                 // get & decode user input from key 
{
  if (usePaddles) return paddleInput();           // it can be either paddle input
  else return straightKeyInput();                 // or straight key, depending on setting
}

void practice()                                   // get Morse from user & display it
{
  char oldCh = ' ';                            
  while (!button_pressed) 
  {
    char ch = morseInput();                       // get a morse character from user
    if (!((ch==' ') && (oldCh==' ')))             // only 1 word space at a time.
      addCharacter(ch);                           // show character on display
    oldCh = ch;                                   // and remember it 
  }
}

void copyCallsigns()                              // show a callsign & see if user can copy it
{
  char call[8];
  while (!button_pressed)                      
  {  
    randomCallsign(call);                         // make a random callsign       
    mimic(call);
  }
}

void copyOneChar()
{
  char text[8];  
  while (!button_pressed)                      
  {
    strcpy(text,"");                              // start with empty string  
    addChar(text,randomLetter());                 // add a random letter
    mimic(text);                                  // compare it to user input
  }
}

void copyTwoChars()
{
  char text[8];   
  while (!button_pressed)                      
  {
    strcpy(text,"");                              // start with empty string  
    addChar(text,randomLetter());                 // add a random letter
    addChar(text,randomLetter());                 // make that two random letters
    mimic(text);                                  // compare it to user input
  }
}

void copyWords()                                  // show a callsign & see if user can copy it
{
  char text[10]; 
  while (!button_pressed)                      
  { 
    int index=random(0, ELEMENTS(words));         // eeny, meany, miney, moe
    strcpy(text,words[index]);                    // pick a random word       
    mimic(text);                                  // and ask user to copy it
  }
}

void encourageUser()                              // helper fn for showScore()
{
  char *phrases[]= {"Good Job", "Keep Going",     // list of phrases to display
    "Amazing","Dont Stop","Impressive"};
  if (score==0) return;                           // 0 is not an encouraging score
  if (score%25) return;                           // set interval at every 25 points
  textCol=0; textRow=6;                           // place display below challange
  int choice = random(0,ELEMENTS(phrases));       // pick a random message
  sendString(phrases[choice]);                    // show it and send it  
}

void displayNumber(int num, int color,            // show large number on screen
    int x, int y, int wd, int ht)                 // specify x,y and width,height of box
{
  int textSize = (num>99)?5:6;                    // use smaller text for 3 digit scores
  tft.setCursor(x+15,y+20);                       // position text within colored box
  tft.setTextSize(textSize);                      // use big text,
  tft.setTextColor(BLACK,color);                  // in inverted font,
  tft.fillRect(x,y,wd,ht,color);                  // on selected background
  tft.print(num);                                 // show the number
  tft.setTextSize(2);                             // resume usual size
  tft.setTextColor(textColor,BLACK);              // resume usual colors  
}
  
void showScore()                                  // helper fn for mimic()
{
  const int x=200,y=50,wd=105,ht=80;              // posn & size of scorecard
  int bkColor = (score>0)?GREEN:RED;              // back-color green unless score 0
  displayNumber(score,bkColor,x,y,wd,ht);         // show it!
  encourageUser();                                // show encouraging message periodically                                
}

void mimic1(char *text)
{
  char ch, response[20];                                                          
  textRow=1; textCol=6;                           // set position of text 
  sendString(text);                               // display text & morse it
  strcpy(response,"");                            // start with empty response
  textRow=2; textCol=6;                           // set position of response
  while (!button_pressed && !ditPressed()         // wait until user is ready
    && !dahPressed()) ;
  do {                                            // user has started keying...
    ch = morseInput();                            // get a character
    if (ch!=' ') addChar(response,ch);            // add it to the response
    addCharacter(ch);                             // and put it on screen
  } while (ch!=' ');                              // space = word timeout
  if (button_pressed) return;                     // leave without scoring
  if (!strcmp(text,response))                     // did user match the text?
    score++;                                      // yes, so increment score
  else score = 0;                                 // no, so reset score to 0
  showScore();                                    // display score for user
  delay(FLASHCARDDELAY);                          // wait between attempts
  newScreen();                                    // erase screen for next attempt
}

void showHitsAndMisses(int hits, int misses)      // helper fn for mimic2()
{                      
  const int x=200,y1=50,y2=140,wd=105,ht=80;      // posn & size of scorecard             
  displayNumber(hits,GREEN,x,y1,wd,ht);           // show the hits in green
  displayNumber(misses,RED,x,y2,wd,ht);           // show misses in red                               
}

void headCopy()                                  // show a callsign & see if user can copy it
{
  char text[10]; 
  while (!button_pressed)                      
  { 
    int index=random(0, ELEMENTS(words));         // eeny, meany, miney, moe
    strcpy(text,words[index]);                    // pick a random word       
    mimic2(text);                                 // and ask user to copy it
  }
}

void hitTone()
{
  ledcWriteTone(0,440); delay(150);               // first tone 
  ledcWriteTone(0,600); delay(200);               // second tone
  ledcWrite(0,0);                                 // audio off
}

void missTone()
{
  ledcWriteTone(0,200); delay(200);               // single tone
  ledcWrite(0,0);                                 // audio off
}

void mimic2(char *text)                           // used by head-copy feature
{
  char ch, response[20]; bool correct,leave;   
  do {                                            // repeat same word until correct                                            
    sendMorseWord(text);                          // morse the text, NO DISPLAY
    strcpy(response,"");                          // start with empty response
    textRow=2; textCol=6;                         // set position of response
    while (!button_pressed && !ditPressed()       // wait until user is ready
      && !dahPressed()) ;
    do {                                          // user has started keying...
      ch = morseInput();                          // get a character
      if (ch!=' ') addChar(response,ch);          // add it to the response
      addCharacter(ch);                           // and put it on screen
    } while (ch!=' ');                            // space = word timeout
    if (button_pressed) return;                   // leave without scoring
    correct = !strcmp(text,response);             // did user match the text?
    leave   = !strcmp(response,"=");              // user entered BT/break to skip
    if (correct) {                                // did user match the text?
      hits++;                                     // got it right!
      hitTone();
    }
    else {
      misses++;                                   // user muffed it
      missTone();  
    }
    showHitsAndMisses(hits,misses);               // display scores for user
    delay(1000);                                  // wait a sec, then
    tft.fillRect(50,50,120,80,BLACK);             // erase answer
  } while (!correct && !leave);                   // repeat until correct or user breaks
}

void mimic (char* text)
{
  if (button_downtime > LONGPRESS)                // did user hold button down > 1 sec?
    mimic2 (text);                                // yes, so use alternate scoring mode 
  else mimic1 (text);                             // no, so use original scoring mode
}

void flashcards()
{
  tft.setTextSize(7);
  while (!button_pressed)
  {
     int index = random(0,ELEMENTS(morse));       // get a random character
     int code = morse[index];                     // convert to morse code
     if (!code) continue;                         // only do valid morse chars
     sendElements(code);                          // sound it out
     delay(1000);                                 // wait for user to guess
     tft.setCursor(120,70);
     tft.print(char('!'+index));                  // show the answer
     delay(FLASHCARDDELAY);                       // wait a little
     newScreen();                                 // and start over.
  }
}

void twoWay()                                     // wireless QSO between units
{
  initWireless();                                 // look for another unit & connect
  char oldCh = ' ';                               
  while (!button_pressed) 
  {
    char ch = morseInput();                       // get a morse character from user
    if (!((ch==' ') && (oldCh==' ')))             // only 1 word space at a time.
    {
      tft.setTextColor(TXCOLOR);
      addCharacter(ch);                           // show character on display
      sendWireless(ch);                           // send it to other device 
    }
    oldCh = ch;                                   // and remember it 

    while (ch=deQueue())                          // any characters to receive?
    {
      tft.setTextColor(RXCOLOR);                  // change text color
      sendCharacter(ch);                          // sound it out and show it.
    }
  }
  tft.setTextColor(TEXTCOLOR);
  closeWireless();
}

//===================================  Config Menu  =====================================

void printConfig()                                // debugging only; not called
{
  Serial.println("EEPROM contents");
  for (int i=0; i<25; i++) {
    int value = EEPROM.read(i);
    Serial.print(i); Serial.print(": ");
    Serial.println(value,HEX);
  }
  Serial.println();
}

void saveConfig()
{
  EEPROM.write(0,42);                             // the answer to everything
  EEPROM.write(1,charSpeed);                      // save the character speed in wpm
  EEPROM.write(2,codeSpeed);                      // save overall code speed in wpm
  EEPROM.write(3,pitch/10);                       // save pitch as 1/10 of value
  EEPROM.write(4,ditPaddle);                      // save pin corresponding to 'dit'
  EEPROM.write(5,kochLevel);                      // save current Koch lesson #
  EEPROM.write(6,usePaddles);                     // save key type
  EEPROM.write(7,xWordSpaces);                    // save extra word spaces
  for (int i=0; i<10; i++)                        // save callsign,
    EEPROM.write(8+i,myCall[i]);                  // one letter at a time
  EEPROM.write(18,keyerMode);                     // save keyer mode (1=A, 2=B)
  EEPROM.write(19,startItem);                     // save startup activity
  EEPROM.write(20,brightness);                    // save screen brightness
  EEPROM.write(21,highByte(textColor));           // save text color
  EEPROM.write(22,lowByte(textColor));
  EEPROM.write(23,highByte(bgColor));             // save background color
  EEPROM.write(24,lowByte(bgColor));
  EEPROM.commit();                                // ESP32 only
}

void loadConfig()
{
  int flag = EEPROM.read(0);                      // saved values been saved before?
  if (flag==42)                                   // yes, so load saved parameters
  {
     charSpeed   = EEPROM.read(1);
     codeSpeed   = EEPROM.read(2);
     pitch       = EEPROM.read(3)*10;
     ditPaddle   = EEPROM.read(4);
     kochLevel   = EEPROM.read(5);
     usePaddles  = EEPROM.read(6);
     xWordSpaces = EEPROM.read(7);
     for (int i=0; i<10; i++)
       myCall[i] = EEPROM.read(8+i); 
     keyerMode   = EEPROM.read(18);
     startItem   = EEPROM.read(19);
     brightness  = EEPROM.read(20);
     textColor   =(EEPROM.read(21)<<8)            // add color high byte
                 + EEPROM.read(22);               // and color low byte
     bgColor     =(EEPROM.read(23)<<8)
                 + EEPROM.read(24);
     checkConfig();                               // ensure loaded settings are valid   
  } 
}

void checkConfig()                                // ensure config settings are valid
{
  if ((charSpeed<MINSPEED)||(charSpeed>MAXSPEED)) // validate character speed
    charSpeed = DEFAULTSPEED;                     
  if ((codeSpeed<MINSPEED)||(codeSpeed>MAXSPEED)) // validate code speed
    codeSpeed = DEFAULTSPEED;
  if ((pitch<MINPITCH)||(pitch>MAXPITCH))         // validate pitch
    pitch = DEFAULTPITCH;
  if ((kochLevel<0)||(kochLevel>ELEMENTS(koch)))  // validate koch lesson number
    kochLevel = 0;
  if (xWordSpaces>MAXWORDSPACES)                  // validate word spaces
     xWordSpaces = 0;
  if (!isAlphaNumeric(myCall[0]))                 // validate callsign
     strcpy(myCall,"W8BH");
  if ((keyerMode<0)||(keyerMode>2))               // validate keyer mode
    keyerMode = IAMBIC_B;
  if ((brightness<10)||(brightness>100)) 
    brightness=100;                               // validate screen brightness
  if (textColor==bgColor)                         // text&bg colors must be different
  {
     textColor = CYAN;
     bgColor = BLACK;    
  }
  if ((startItem<-1)||(startItem>27))
    startItem = -1;                               // validate startup screen
}

void useDefaults()                                // if things get messed up...
{
  charSpeed   = DEFAULTSPEED;
  codeSpeed   = DEFAULTSPEED;
  pitch       = DEFAULTPITCH;
  ditPaddle   = PADDLE_A;
  dahPaddle   = PADDLE_B;
  kochLevel   = 1;
  usePaddles  = true;
  xWordSpaces = 0;
  strcpy(myCall,"W8BH");
  keyerMode   = IAMBIC_B;
  brightness  = 100;
  textColor   = TEXTCOLOR;
  bgColor     = BG;
  startItem   = -1;
  saveConfig();
  roger();
}

char *ltrim(char *str) {                          // quick n' dirty trim routine
  while (*str==' ') str++;                        // dangerous but efficient
  return str;
}

void selectionString(int choice, char *str)       // return menu item in str
{
  strcpy(str,"");
  if (choice<0) return;                           // validate input value      
  int i = choice/10;                              // get menu from selection
  int j = choice%10;                              // get menu item from selection
  switch (i) 
  {
    case 0: strcpy(str,ltrim(menu0[j])); break;   // show choice from menu0
    case 1: strcpy(str,ltrim(menu1[j])); break;   // show choice from menu1
    case 2: strcpy(str,ltrim(menu2[j])); break;   // show choice from menu2
    default: ;                                    // oopsie
  }
}

void showMenuChoice(int choice)
{
  char str[10];
  const int x=180,y=40;                           // screen posn for startup display
  tft.fillRect(x,y,130,32,bgColor);               // erase any prior entry
  tft.setCursor(x,y);
  if (choice<0) tft.print("Main Menu");            
  else {
    tft.println(ltrim(mainMenu[choice/10]));      // show top menu choice on line 1
    tft.setCursor(x,y+16);                        // go to next line
    selectionString(choice,str);                  // get menu item
    tft.print(str);                               // and show it
  }
}

void changeStartup()                              // choose startup activity
{
  const int LASTITEM = 27;                        // currenly 27 choices
  tft.setTextSize(2);
  tft.println("\nStartup:");
  int i = startItem;
  if ((i<0)||(i>LASTITEM)) i=-1;                  // dont wig out on bad value
  showMenuChoice(i);                              // show current startup choice
  button_pressed = false;
  while (!button_pressed)
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      i += dir;                                   // change choice up/down
      i = constrain(i,-1,LASTITEM);               // keep within bounds
      showMenuChoice(i);                          // display new startup choice
    }
  }
  startItem = i;                                  // update startup choice  
}


void changeBackground()
{
  const int x=180,y=150;                          // screen posn for text display
  tft.setTextSize(2);
  tft.println("\n\n\nBackground:");
  tft.drawRect(x-6,y-6,134,49,WHITE);             // draw box around background
  button_pressed = false;
  int i = 0;
  tft.fillRect(x-5,y-5,131,46,colors[i]);         // show current background color
  while (!button_pressed)
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      i += dir;                                   // change color up/down
      i = constrain(i,0,ELEMENTS(colors)-1);      // keep within given colors
      tft.fillRect(x-5,y-5,131,46,colors[i]);     // show new background color
    }
  }
  bgColor = colors[i];                            // save background color
}

void changeTextColor()
{
  const char sample[] = "ABCDE";
  const int x=180,y=150;                          // screen posn for text display
  tft.setTextSize(2);
  tft.println("Text Color:");
  tft.setTextSize(4);
  tft.setCursor(x,y);
  int i = 7;                                      // start with cyan for fun
  tft.setTextColor(colors[i]);
  tft.print(sample);                              // display text in cyan
  button_pressed = false;
  while (!button_pressed)
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      i += dir;                                   // change color up/down
      i = constrain(i,0,ELEMENTS(colors)-1);      // keep within given colors
      tft.setCursor(x,y);
      tft.setTextColor(colors[i],bgColor);        // show text in new color
      tft.print(sample);
    }
  }
  textColor = colors[i];                          // update text color
}

void changeBrightness()
{
  const int x=180,y=100;                           // screen position
  tft.println("\n\n\nBrightness:");
  tft.setTextSize(4);
  tft.setCursor(x,y);
  tft.print(brightness);                          // show current brightness
  button_pressed = true;                          // on ESP32, inhibit user input                 
  while (!button_pressed)
  {
    int dir = readEncoder(2);
    if (dir!=0)                                   // user rotated encoder knob
    {
      brightness += dir*5;                        // so change level up/down, 5% increments
      brightness = constrain(brightness,10,100);  // stay in range
      tft.fillRect(x,y,100,40,bgColor);           // erase old value
      tft.setCursor(x,y);
      tft.print(brightness);                      // and display new value
      setBrightness(brightness);                  // update screen brightness
    }
  }
}

void setScreen()
{
  changeStartup();                                // set startup screen
  changeBrightness();                             // set display brightness
  changeBackground();                             // set background color
  changeTextColor();                              // set text color
  saveConfig();                                   // save settings 
  roger();                                        // and acknowledge
  clearScreen();
}

void setCodeSpeed()
{
  const int x=240,y=50;                           // screen posn for speed display
  tft.println("\nEnter");
  tft.print("Code Speed (WPM):");
  tft.setTextSize(4);
  tft.setCursor(x,y);
  tft.print(charSpeed);                           // display current speed
  button_pressed = false;
  while (!button_pressed)
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      charSpeed += dir;                           // ...so change speed up/down
      charSpeed = constrain(charSpeed,
        MINSPEED,MAXSPEED);
      tft.fillRect(x,y,50,50,bgColor);            // erase old speed
      tft.setCursor(x,y);
      tft.print(charSpeed);                       // and show new speed 
    }
  }
  ditPeriod = intracharDit();                     // adjust dit length
}

void setFarnsworth()
{
  const int x=240,y=100;                          // screen posn for speed display
  if (codeSpeed>charSpeed) 
     codeSpeed = charSpeed;                       // dont go above charSpeed
  tft.setTextSize(2);
  tft.println("\n\n\nFarnsworth");
  tft.print("Speed (WPM):");
  tft.setTextSize(4);
  tft.setCursor(x,y);
  tft.print(codeSpeed);                           // display current speed
  button_pressed = false;
  while (!button_pressed)
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      codeSpeed += dir;                           // ...so change speed up/down 
      codeSpeed = constrain(codeSpeed,            // dont go below minimum 
        MINSPEED,charSpeed);                      // and dont exceed charSpeed
      tft.fillRect(x,y,50,50,bgColor);            // erase old speed
      tft.setCursor(x,y);
      tft.print(codeSpeed);                       // and show new speed    
    }
  }                
  ditPeriod = intracharDit();                     // adjust dit length
}

void setExtraWordDelay()                          // add extra word spacing
{
  const int x=240,y=150;                          // screen posn for speed display
  tft.setTextSize(2);
  tft.println("\n\n\nExtra Word Delay");
  tft.print("(Spaces):");
  tft.setTextSize(4);
  tft.setCursor(x,y);
  tft.print(xWordSpaces);                         // display current space
  button_pressed = false;
  while (!button_pressed)
  {
    int dir = readEncoder();
    if (dir!=0)                                   // user rotated encoder knob:
    {
      xWordSpaces += dir;                         // ...so change value up/down 
      xWordSpaces = constrain(xWordSpaces,        // stay in range 0..MAX
        0,MAXWORDSPACES);         
      tft.fillRect(x,y,50,50,bgColor);            // erase old value
      tft.setCursor(x,y);
      tft.print(xWordSpaces);                     // and show new value  
    }
  }                
}

void setSpeed()
{
  setCodeSpeed();                                 // get code speed
  setFarnsworth();                                // get farnsworth delay
  setExtraWordDelay();                            // Thank you Mark AJ6CU!
  saveConfig();                                   // save the new speed values
  roger();                                        // and acknowledge  
}

void setPitch()
{
  const int x=120,y=80;                           // screen posn for pitch display
  tft.print("Tone Frequency (Hz)");
  tft.setTextSize(4);
  tft.setCursor(x,y);
  tft.print(pitch);                               // show current pitch                 
  while (!button_pressed)
  {
    int dir = readEncoder(2);
    if (dir!=0)                                   // user rotated encoder knob
    {
      pitch += dir*50;                            // so change pitch up/down, 50Hz increments
      pitch=constrain(pitch,MINPITCH,MAXPITCH);   // stay in range
      tft.fillRect(x,y,100,40,bgColor);           // erase old value
      tft.setCursor(x,y);
      tft.print(pitch);                           // and display new value
      dit();                                      // let user hear new pitch
    }
  }
  saveConfig();                                   // save the new pitch 
  roger();                                        // and acknowledge
}

void configKey()
{
  tft.print("Currently: ");                       // show current settings:
  if (!usePaddles)                                // using paddles?
    tft.print("Straight Key");                    // not on your life!
  else {
    tft.print("Iambic Mode ");                    // of course paddles, but which keyer mode?
    if (keyerMode==IAMBIC_B) tft.print("B");      // show iambic B or iambic A
    else tft.print("A");
  }  
  tft.setCursor(0,60);
  tft.println("Send a dit NOW");                  // first, determine which input is dit
  while (!button_pressed && 
    !ditPressed() && !dahPressed()) ;             // wait for user input
  if (button_pressed) return;                     // user wants to exit
  ditPaddle = !digitalRead(PADDLE_A)? 
    PADDLE_A: PADDLE_B;                           // dit = whatever pin went low.
  dahPaddle = !digitalRead(PADDLE_A)? 
    PADDLE_B: PADDLE_A;                           // dah = opposite of dit.
  tft.println("OK.\n");
  saveConfig();                                   // save it
  roger();                                        // and acknowledge.

  tft.println("Send Dit again for Key");          // now, select key or paddle input
  tft.println("Send Dah for Paddles");
  while (!button_pressed && 
    !ditPressed() && !dahPressed()) ;             // wait for user input
  if (button_pressed) return;                     // user wants to exit
  usePaddles = dahPressed();                      // dah = paddle input
  tft.println("OK.\n");
  saveConfig();                                   // save it
  roger(); 

  if (!usePaddles) return;
  tft.println("Send Dit for Iambic A");           // now, select keyer mode
  tft.println("Send Dah for Iambic B");
  while (!button_pressed && 
    !ditPressed() && !dahPressed()) ;             // wait for user input
  if (button_pressed) return;                     // user wants to exit
  if (ditPressed()) keyerMode = IAMBIC_A;
  else keyerMode = IAMBIC_B;
  tft.println("OK.\n");
  saveConfig();                                   // save it
  if (keyerMode==IAMBIC_A) sendCharacter('A');
  else sendCharacter('B');
}

void setCallsign() {
  char ch, response[20];   
  tft.print("\n Enter Callsign:");
  tft.setTextColor(CYAN);                                                      
  strcpy(response,"");                            // start with empty response
  textRow=2; textCol=8;                           // set position of response
  while (!button_pressed && !ditPressed()         // wait until user is ready
    && !dahPressed()) ;
  do {                                            // user has started keying...
    ch = morseInput();                            // get a character
    if (ch!=' ') addChar(response,ch);            // add it to the response
    addCharacter(ch);                             // and put it on screen
  } while (ch!=' ');                              // space = word timeout
  if (button_pressed) return;                     // leave without saving
  strcpy(myCall,response);                        // copy user input to callsign
  delay(1000);                                    // allow user to see result
  saveConfig();                                   // save it
  roger();  
}

//===================================  Screen Routines ====================================

//  The screen is divided into 3 areas:  Menu, Icons, and Body
//
//  [------------------------------------------------------]
//  [     Menu                                      Icons  ]
//  [------------------------------------------------------]
//  [                                                      ]
//  [     Body                                             ]
//  [                                                      ]
//  [------------------------------------------------------]
//
//  "Menu" is where the main menu is displayed
//  "Icons" is for battery icon and other special flags (30px)
//  "Body" is the writable area of the screen  

void clearMenu()
{
  tft.fillRect(0,0,DISPLAYWIDTH-30,ROWSPACING,bgColor);   
}

void clearBody()
{
  tft.fillRect(0, TOPMARGIN, DISPLAYWIDTH, DISPLAYHEIGHT, bgColor);  
}

void clearScreen()
{
  tft.fillScreen(bgColor);                        // fill screen with background color
  tft.drawLine(0,TOPMARGIN-6,DISPLAYWIDTH,
    TOPMARGIN-6, YELLOW);                         // draw horizontal menu line  
} 

void newScreen()                                  // prepare display for new text.  Menu not distrubed
{
  clearBody();                                    // clear screen below menu
  tft.setTextColor(textColor,bgColor);            // set text foreground & background color
  tft.setCursor(0,TOPMARGIN);                     // position graphics cursor
  textRow=0; textCol=0;                           // position text cursor below the top menu
}


//===================================  Menu Routines ====================================

void showCharacter(char c, int row, int col)      // display a character at given row & column
{
  int x = col * COLSPACING;                       // convert column to x coordinate
  int y = TOPMARGIN + (row * ROWSPACING);         // convert row to y coordinate
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
    if (textRow >= MAXROW) newScreen();           // if no more rows, clear & start at top.
  }
}

int getMenuSelection()                            // Display menu system & get user selection
{
  int item;
  newScreen();                                    // start with fresh screen
  menuCol = topMenu(mainMenu,ELEMENTS(mainMenu)); // show horiz menu & get user choice
  switch (menuCol)                                // now show menu that user selected:
  {
   case 0: 
     item = subMenu(menu0,ELEMENTS(menu0));       // "receive" dropdown menu
     break;
   case 1: 
     item = subMenu(menu1,ELEMENTS(menu1));       // "send" dropdown menu
     break;
   case 2: 
     item = subMenu(menu2,ELEMENTS(menu2));       // "config" dropdown menu
  }
  return (menuCol*10 + item);                     // return user's selection
}

void setTopMenu(char *str)                        // erase menu & replace with new text
{
  clearMenu();
  showMenuItem(str,0,0,FG,bgColor);
}

void showSelection(int choice)                    // display current activity on top menu
{
  char str[10];                                   // reserve room for string
  selectionString(choice,str);                    // get menu item that user chose
  setTopMenu(str);                                // and show it in menu bar
}

void showMenuItem(char *item, int x, int y, int fgColor, int bgColor)
{
  tft.setTextSize(2);                             // sets menu text size
  tft.setCursor(x,y);
  tft.setTextColor(fgColor, bgColor);
  tft.print(item);                              
}

int topMenu(char *menu[], int itemCount)          // Display a horiz menu & return user selection
{
  int index = menuCol;                            // start w/ current row
  button_pressed = false;                         // reset button flag
  for (int i = 0; i < itemCount; i++)             // for each item in menu                         
    showMenuItem(menu[i],i*MENUSPACING,0,FG,bgColor);  // display it 
  showMenuItem(menu[index],index*MENUSPACING,     // highlight current item
    0,SELECTFG,SELECTBG);

  while (!button_pressed)                         // loop for user input:
  {
    int dir = readEncoder();                      // check encoder
    if (dir) {                                    // did it move?
      showMenuItem(menu[index],index*MENUSPACING,
        0, FG,bgColor);                           // deselect current item
      index += dir;                               // go to next/prev item
      if (index > itemCount-1) index=0;           // dont go beyond last item
      if (index < 0) index = itemCount-1;         // dont go before first item
      showMenuItem(menu[index],index*MENUSPACING,
        0, SELECTFG,SELECTBG);                    // select new item         
    }  
  }
  return index;  
}

void displayFrame(char *menu[], int top, int itemCount)
{
  int x = menuCol * MENUSPACING;                  // x-coordinate of this menu
  for (int i = 0; i < MAXROW; i++)                // for all items in the frame
  {
     int y = TOPMARGIN + i*ROWSPACING;            // calculate y coordinate
     int item = top + i;
     if (item<itemCount)                          // make sure item exists
       showMenuItem(menu[item],x,y,FG,bgColor);   // and show the item.
  } 
}

int subMenu(char *menu[], int itemCount)          // Display drop-down menu & return user selection
{
  int index=0,top=0,pos=0,x,y; 
  button_pressed = false;                         // reset button flag
  x = menuCol * MENUSPACING;                      // x-coordinate of this menu
  displayFrame(menu,0,itemCount);                 // display as many menu items as possible
  showMenuItem(menu[0],x,TOPMARGIN,               // highlight first item
    SELECTFG,SELECTBG);

  while (!button_pressed)                         // exit on button press
  {
    int dir = readEncoder();                      // check for encoder movement
    if (dir)                                      // it moved!    
    {
      if ((dir>0) && (index==(itemCount-1)))      // dont try to go below last item
        continue;
      if ((dir<0) && (index==0))                  // dont try to go above first item
        continue;
      if ((dir>0) && (pos==(MAXROW-1)))           // does the frame need to move down?
      {                              
        top++;                                    // yes: move frame down,
        displayFrame(menu,top,itemCount);         // display it,
        index++;                                  // and select next item
      }
      else if ((dir<0) && (pos==0))               // does the frame need to move up?
      {
        top--;                                    // yes: move frame up,
        displayFrame(menu,top,itemCount);         // display it,
        index--;                                  // and select previous item
      }
      else                                        // we must be moving within the frame
      {
        y = TOPMARGIN + pos*ROWSPACING;           // calc y-coord of current item
        showMenuItem(menu[index],x,y,FG,bgColor); // deselect current item
        index += dir;                             // go to next/prev item
      }
      pos = index-top;                            // posn of selected item in visible list
      y = TOPMARGIN + pos*ROWSPACING;             // calc y-coord of new item
      showMenuItem(menu[index],x,y,
        SELECTFG,SELECTBG);                       // select new item
    }
  }
  return index;  
}


//================================  Main Program Code ================================

void setBrightness(int level)                     // level 0 (off) to 100 (full on)       
{
   // not implemented in ESP32 version at this time 
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

void initMorse()
{
  ledcSetup(0,1E5,12);                            // smoke & mirrors for ESP32
  ledcAttachPin(AUDIO,0);                         // since tone() not yet supported
  pinMode(LED,OUTPUT);                            // LED, but could be keyer output instead
  pinMode(PADDLE_A, INPUT_PULLUP);                // two paddle inputs, both active low
  pinMode(PADDLE_B, INPUT_PULLUP);
  ditPeriod = intracharDit();                     // set up character timing from WPM 
  dahPaddle = (ditPaddle==PADDLE_A)?              // make dahPaddle opposite of dit
     PADDLE_B: PADDLE_A;
  roger();                                        // acknowledge morse startup with an 'R'                
}

void initSD()
{
  delay(200);                                     // dont rush things on power-up
  SD.begin(SD_CS);                                // initialize SD library
  delay(100);                                     // takin' it easy
}

void initScreen()
{
  tft.begin();                                    // initialize screen object
  tft.setRotation(SCREEN_ROTATION);               // landscape mode: use '1' or '3'
  tft.fillScreen(BLACK);                          // start with blank screen
  setBrightness(100);                             // start screen full brighness
}

void splashScreen()                               // not splashy at all!
{                                                 // have fun sprucing it up.
  tft.setTextSize(3);
  tft.setTextColor(YELLOW);
  tft.setCursor(100, 50); 
  tft.print(myCall);                              // display user's callsign
  tft.setTextColor(CYAN);
  tft.setCursor(15, 90);      
  tft.print("Morse Code Tutor");                  // add title
  tft.setTextSize(1);
  tft.setCursor(50,220);
  tft.setTextColor(WHITE);
  tft.print("Copyright (c) 2020, Bruce E. Hall"); // legal small print
  tft.setTextSize(2);
}


void setup() 
{
  Serial.begin(115200);                           // for debugging only 
  initScreen();                                   // blank screen in landscape mode
  EEPROM.begin(32);                               // ESP32 specific for 32 bytes Flash
  initSD();                                       // initialize SD library
  loadConfig();                                   // get saved values from EEPROM
  splashScreen();                                 // show we are ready
  initEncoder();                                  // attach encoder interrupts
  initMorse();                                    // attach paddles & adjust speed
  delay(2000);                                    // keep splash screen on for a while
  clearScreen();
}


void loop()
{
  int selection = startItem;                      // start with user specified startup screen
  if (!inStartup || (startItem<0))                // but, if there isn't one,                 
    selection = getMenuSelection();               // get menu selection from user instead
  inStartup = false;                              // reset startup flag
  showSelection(selection);                       // display users selection at top of screen
  newScreen();                                    // and clear the screen below it.
  button_pressed = false;                         // reset flag for new presses
  randomSeed(millis());                           // randomize!
  score=0; hits=0; misses=0;                      // restart score for copy challenges  
  switch(selection)                               // do action requested by user
  {
    case 00: sendKoch(); break;
    case 01: sendLetters(); break;
    case 02: sendWords(); break;
    case 03: sendNumbers(); break;
    case 04: sendMixedChars(); break;
    case 05: sendFromSD(); break;
    case 06: sendQSO(); break;
    case 07: sendCallsigns(); break;

    case 10: practice(); break;
    case 11: copyOneChar(); break;
    case 12: copyTwoChars(); break;
    case 13: copyWords(); break;
    case 14: copyCallsigns(); break;
    case 15: flashcards(); break;
    case 16: headCopy(); break;
    case 17: twoWay(); break;
    
    case 20: setSpeed(); break;
    case 21: checkSpeed(); break;
    case 22: setPitch(); break;
    case 23: configKey(); break;
    case 24: setCallsign(); break;
    case 25: setScreen(); break;
    case 26: useDefaults(); break;
    default: ;
  }  
}
