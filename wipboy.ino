//import

// webserver
#include <ESP8266WiFi.h>
#include "./DNSServer.h"                  // Patched lib
#include <ESP8266WebServer.h>
// graphics
#include <Adafruit_GFX.h>       // Core graphics library
#include "./Adafruit_ST7735.h"  // Hardware-specific library - patched for esp
#include <SPI.h>
// physical controls (because the regular arduino stuff sucks)
#include "./Slider.h"
#include "./Button.h"
#include "./Icon.h"

#include "./WipboyNode.h"
/*
 * ESP8266-12        HY-1.8 SPI
 * REST              Pin 06 (RESET)
 * GPIO2             Pin 07 (A0)
 * GPIO13 (HSPID)    Pin 08 (SDA)
 * GPIO14 (HSPICLK)  Pin 09 (SCK)
 * GND (HSPICS)      Pin 10 (CS)
 *
 * GPIO ADC          Control knob
 * GPIO 0            Button1
 * GPIO 4            Button2
 * GPIO 5            Button3
 *
 *  Free IOs:
 *  GPI01   (maybe if we get OTA working) (use as I2C for radio, yeah!)
 *  GPIO3   (maybe if we get OTA working) (use as I2C for radio, yeah!)
 *  GPIO15  (will probably use for LED backlight)
 *  GPIO16  (will probably use for buzzer or sleep mode)
 *  GPIO12  (SPI IN ONLY: can have an SPI MISO here, though it won't have a MOSI.
 *                        example: RFID reader)
  */


// =======================================
// Declare

// ------------------------
// screen
#define TFT_CS     151
#define TFT_RST    150
#define TFT_DC     2
#define TFT_BACKLIGHT 47

uint16_t fgColour = ST7735_GREEN;
uint16_t bgColour = ST7735_BLACK;

uint16_t colours[] = {
  ST7735_BLACK,
  ST7735_WHITE,
  ST7735_RED,
  ST7735_GREEN,
  ST7735_BLUE,
  0x6D45, // green
  0x6E19, // blue
  0xDEAB, // gold
  0xDF7A, 
  0x2965,
  0x10E1,
  //0x31A2,
  0x28C4 // dark red  
};

// TODO: set up an array of all the colours we want to offer.
// then, in the settings, we can just display the colours and
// let them select which one they want.  Ooooh! dynamically
// created icons!  We should do that here!  Wooo :)

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// ------------------------
// text
// This is all the text to display and graphics to show.
// In theory, if we had enough RAM and foreign users, you
// could have different languages for the GUI stored as string
// arrays and a simple choice as to which was being used.
String localisation[] =
{
  "Main Menu",
  "Tracker",
  "Messenger",
  "S.T.A.T.S.",
  "Settings",
  " Select |        |        ",
  "        |MainMenu|        ",
  "fg colour",
  "bg colour",
  "brightness",
  "adminPsswd",
  " Select |MainMenu|        ",
  " Select |MainMenu| Cancel ",
  "Targetting: ",
  "Signal Strength: ",
  "Select Node",
};


#define S_MENU 0
#define S_TRACKER 1
#define S_MESSENGER 2
#define S_STATUS 3
#define S_SETTINGS 4
#define S_BUTTONS1 5
#define S_BUTTONS2 6
#define S_SETTINGS0 7
#define S_SETTINGS1 8
#define S_SETTINGS2 9
#define S_SETTINGS3 10
#define S_BUTTONS3 11
#define S_BUTTONS4 12
#define S_TARGET 13
#define S_TARGETSTRENGTH 14
#define S_CHOOSENODE 15
// ------------------------
// Graphics
// In order to avoid having to use an SD card reader (not enough pins)
// And because this is a monochrome display anyway I am converting
// images to a byte format.  We can then just copy/paste the text
// into here program and display them with the "drawBitmap()" cmd.

// I should set them up like the localisation. Need to think on
// that more

static unsigned char i_pipboy[] = {
  B00000000, B00000000, B00000000, B00000000,
  B00000000, B01010001, B00000000, B00000000,
  B00000000, B00000100, B11000000, B00000000,
  B00000000, B00100000, B00000000, B00000000,
  B00000000, B00001100, B00000000, B00000000,
  B00000000, B00111100, B10000000, B00000000,
  B00000000, B00010101, B10000000, B00000000,
  B00000000, B00101101, B10000000, B00000000,
  B00000000, B00101111, B10000000, B00000000,
  B00000000, B00011101, B11000000, B00000000,
  B00000000, B00100001, B10000000, B00000000,
  B00000000, B00011111, B01000000, B00000000,
  B00000000, B00011110, B00000000, B00000000,
  B00000000, B00000100, B10100000, B00000000,
  B00000000, B00000011, B01000000, B00000000,
  B00000000, B11000100, B11100000, B00000000,
  B00000001, B11101101, B11111000, B00000000,
  B00000010, B11101111, B11111100, B00000000,
  B00000010, B11101000, B00011100, B00000000,
  B00000110, B11101011, B00001110, B00000000,
  B00000111, B01101011, B00000111, B00000000,
  B00001110, B01110000, B00010111, B10000000,
  B00001100, B00110111, B10011011, B10000000,
  B00001101, B00010010, B10011100, B00000000,
  B00110011, B00110010, B01011100, B01000000,
  B00110011, B01110111, B00011100, B01000000,
  B01010000, B00111111, B11000000, B00000000,
  B00100010, B10010111, B11100000, B01000010,
  B00000111, B10011011, B11010000, B00001110,
  B00000000, B10001001, B10011001, B00011110,
  B00000000, B10010001, B10100000, B01111100,
  B00000000, B00110000, B10100001, B11110000,
  B00000000, B11100000, B10111101, B11100000,
  B00000000, B11100000, B00111101, B10000000,
  B00000000, B11100000, B00011101, B00000000,
  B00000000, B11100000, B01000000, B00000000,
  B00000101, B11100000, B01111100, B00000000,
  B00000011, B00000000, B00000100, B00000000,
  B00000000, B00000000, B00111100, B00000000,
  B00000000, B00000000, B00000000, B00000000,
};

static unsigned char i_messages[] = {
  B00000000, B00000000, B00000000, B00000000,
  B00000001, B01000000, B00000000, B00000000,
  B00000001, B01000000, B00000000, B00000000,
  B00100101, B01010010, B00000000, B00000000,
  B00010000, B00000100, B00000000, B00000000,
  B00001001, B11001000, B00000000, B00000000,
  B00110011, B11100110, B00000000, B00000000,
  B00000110, B00110000, B00000000, B00000000,
  B00010101, B11010100, B00000000, B00000000,
  B00000110, B10110000, B00100111, B10000000,
  B00010011, B01100100, B01011111, B11100000,
  B00100011, B01100010, B00000101, B11100000,
  B00001001, B01001000, B00111111, B10100000,
  B00000001, B01000000, B00101101, B01000000,
  B00000000, B00000000, B00110111, B10100000,
  B00000001, B11000000, B00111101, B11000000,
  B00000010, B00000000, B00100011, B11000000,
  B00000001, B11010000, B00011111, B10000000,
  B00000000, B10011111, B10101110, B00000000,
  B00000000, B01111111, B10110001, B01000000,
  B00000000, B00011111, B10111110, B11100000,
  B00000000, B00001111, B11011001, B11110000,
  B00000000, B00000001, B11011011, B11110000,
  B00000000, B00000000, B11011011, B11111000,
  B00000000, B00000000, B01011011, B11111000,
  B00000000, B00000000, B00011000, B00111100,
  B00000000, B00000000, B01111111, B10011100,
  B00000000, B00000000, B01111111, B10011000,
  B00000000, B00000000, B00001100, B00000100,
  B00000000, B00000000, B01100011, B10011100,
  B00000000, B00000000, B01111111, B10001100,
  B00000000, B00000000, B11111111, B10001000,
  B00000000, B00000000, B11110111, B11000000,
  B00000000, B00000000, B11110011, B11000000,
  B00000000, B00000000, B11100011, B11000000,
  B00000000, B00000000, B10000001, B11000000,
  B00000000, B00000000, B01100001, B11000000,
  B00000000, B00000001, B11000001, B10000000,
  B00000000, B00000000, B00000000, B11000000,
  B00000000, B00000000, B00000000, B11000000,
};



// ------------------------
// webserver
// This is the section for defining the webserver and dns server.
// We can also add our HTML webpages in here as strings by
// copying/pasting.

const byte        DNS_PORT = 53;
IPAddress         apIP(10, 10, 10, 1);
DNSServer         dnsServer;
ESP8266WebServer  webServer(80);

String homepageHTML = "<html><head></head><body>Nothing here at the moment.  Coming soon: phone sync'ing</body></html>";


// ------------------------
// buttons/controls
// TODO: update the Slider object to take min/max values
// and to report current raw value.  That way we can calibrate
// per knob.
Slider knob = Slider(A0, 4, 24);
Button b1 = Button(0, true);
Button b2 = Button(4, true);
Button b3 = Button(5, true);



// ------------------------
// icons, menu items, modes

// We use these constants to define how big the
// buffers are for the title bar and the button
// labels.  This is useful later when we have to
// copy strings into them.  It's an imperfect
// system though, due for revision
#define TITLE_BUFFER_SIZE 24
#define LABELS_BUFFER_SIZE 30

// This is a struct we use to just organise all
// the status info on the top and bottom of the
// display.  In theory we could just use global
// variables instead, but I wanted to experiment
// with organising in structs instead.

// TODO: implement the buzzer status/timer stuffs
typedef struct
{
  bool Update = true;
  bool displayTop = true;
  bool displayBottom = true;
  bool msgs = false;
  char title[TITLE_BUFFER_SIZE];
  char buttonLabels[LABELS_BUFFER_SIZE];
} StatusInfo;

StatusInfo statusInfo = StatusInfo{};



// This is a kind of "sprite" struct for the GUI icons.
// It contains the location information and labels/mode,
// which allows us to put our icons anywhere without
// custom coding the selection box.  The selection box is
// drawn with the x,y,w,h  while the icon is drawn with
// x+offsetX, y+offsetY, w, h

// TODO: Store a pointer to the actual icon we're using.
/*
typedef struct
{
  byte x;
  byte y;
  byte w = 32;
  byte h = 40;
  byte sbw = 45;
  byte sbh = 45;
  byte offsetX = 6;
  byte offsetY = 2;
  byte title = S_MENU;
  byte Mode = 102;
  unsigned char *img = i_pipboy;
} Icon;
*/
// create an array of all the icons we need for the main menu.
// TODO: rename it to mainMenuIcons[]

Icon menuIcons[4] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};
Icon settingIcons[4] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};
Icon colourIcons[12] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10),Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10),Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};
/*
Icon menuIcons[4];
Icon settingIcons[4];
Icon colourIcons[12];
*/
// This is so when we change the selection box we know what the
// previous selected icon was. That way we can clear out the old
// one.
byte lastSelected = 0;

// ------------------------
// Modes and States
// The "mode" is the program that's running.  Like the
// tracker or messenger.  The "state" is what state the
// mode is in, from starting to running to needing update
// to shutting down.

#define STOPPED 0
#define STARTING 1
#define RUNNING 2
#define UPDATE 3
#define SHUTDOWN 4
byte state = STOPPED;
int stateDelay = 0;

#define MAINMENU 102
#define TRACKER 103
#define MESSENGER 104
#define SETTINGS 105
#define STATUS 106
#define ADJUSTBRIGHTNESS 107
#define ADJUSTFGCOLOUR 108
#define ADJUSTBGCOLOUR 109
#define SHOWADMINPW  110
#define NODECHOOSER 111
byte Mode = STOPPED;








WipboyNode networkNodes[64] = {
  WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),
  WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),
  WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),
  WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),
  WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),
  WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode(),
  WipboyNode(),WipboyNode(),WipboyNode(),WipboyNode()
};

byte nodeCount = 0;

WipboyNode target = WipboyNode();
bool targetting = false;
long lastTargetUpdate = millis();
String strBuffer;
byte targetOffset;
byte targetCursor;
Icon selectionIcon = Icon(10,10);















// =======================================
// Setup
void setup()
{
  Mode = STARTING;
  state = STARTING;
  Serial.begin(115200);

  // ------------------------
  // webserver
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Target 1425");


  dnsServer.start(DNS_PORT, "*", apIP);

  webServer.onNotFound([]()
  {
    checkWebRequest();
    webServer.send(200, "text/html", homepageHTML);
  });
  webServer.begin();




  // ------------------------
  // screen

  // TODO: wire a transitor up to a remaining pin and
  // PWM the backlight
  //pinMode (TFT_BACKLIGHT, OUTPUT);
  //digitalWrite(TFT_BACKLIGHT, HIGH);

  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.setRotation(1);
  tft.fillScreen(bgColour);
  tft.setTextColor(fgColour, bgColour);
  tft.setTextWrap(false);


  // ------------------------
  // build the different icons.  doing this to move
  // spammy code to bottom
  buildIcons();
  
  // Setup complete.  Load menu
  Mode = MAINMENU;
}








// =======================================
// Loop
void loop()
{
  // run the background updater.  This keeps everything
  // ticking along without clogging the loop() function
  // with code
  backgroundUpdate();

  // run the status bar updater.  This checks to see if
  // we need to redraw the status bar.  It means we don't
  // waste a lot of cycles (and energy) redrawing the screen
  // every tick
  statusBarUpdate();


  // check which mode to run.  This could probably be cleaner
  // if we used a case switch statement thingy, but I just
  // wanted the functionality so it got a quick if tree instead.
  // TODO: change to case/switch statement thingy
  if (Mode == TRACKER)
  {
    runTracker();
  }
  else if (Mode == MESSENGER)
  {
    runMessenger();
  }
  else if (Mode == SETTINGS)
  {
    runSettings();
  }
  else if (Mode == STATUS)
  {
    runStats();
  }
  else if (Mode == ADJUSTFGCOLOUR)
  {
    runFGColour();
  }
  else if (Mode == ADJUSTBGCOLOUR)
  {
    runBGColour();
  }  
  else if (Mode == NODECHOOSER)
  {
    runNodeChooser();
  }
  else
    mainMenu();
}


//=========================================
// Modes
//=========================================

// mainMenu()
// This mode displays the icons the player will choose to enter
// the other modes.  It uses the knob to select and a button to
// execute the mode change.

void mainMenu()
{
  // draw the menu icons when we first load in or
  // if we have to update the screen
  if (state == STARTING)
  {
    state = UPDATE;
    changeTitle(menuIcons[knob.getPos()].title);
    changeButtonLabels(S_BUTTONS1);
    statusInfo.Update = true;
    drawMenu();
  }

  // We only update if the display has changed somehow. stops
  // wasted cpus and flickering
  else if (state == UPDATE)
  {
    state = RUNNING;
    //drawMenu();
    drawSelectionBox(menuIcons[knob.getPos()]);
  }


  else
  {
    // check if the knob has changed, if it has then clear the
    // current box and set the title to the selected
    //tft.setCursor(120, 100);
    //tft.print(knob.getRawPos());
    //tft.print("   ");
    if (knob.hasChanged())
    {
      Serial.println("knob changed");
      clearSelectionBox(menuIcons[lastSelected]);
      lastSelected = knob.getPos();
      changeTitle(menuIcons[knob.getPos()].title);
      statusInfo.Update = true;
      state = UPDATE;
    }
    // check if the select button has been pressed. if so,
    // change the Mode
    if (b1.isPressed() == 1)
    {
      changeMode(menuIcons[lastSelected].Mode);
    }
  }
}


// =======================================
//=========================================
//=========================================
int scanForNodes()
{
  Serial.println("starting scan");
  long delayTest = millis();
  int numSsids = WiFi.scanNetworks();
  if (numSsids > 64)
    numSsids = 64;    // only have a buffer for so many.
  
  if (numSsids > 0)
  {
    for (int i=0; i < numSsids; i++)
    {
      networkNodes[i].node = i;
      networkNodes[i].ssid = WiFi.SSID(i);
      networkNodes[i].rssi = WiFi.RSSI(i);
    }
  }
  Serial.println("finished");
  Serial.println(millis() - delayTest);
  return numSsids;
}

void runNodeChooser()
{
  if (state == STARTING)
  {
    state = UPDATE;
    changeTitle(S_CHOOSENODE);
    changeButtonLabels(S_BUTTONS4);
    
    // populate the list
    // and set the knob to list length while we're at it
    knob.setRange(scanForNodes());

    // cursor = 0 - 9 (only space for 10 slots)
    // offset = true list selection
    // if knob.getPos() < 9:
    //    offset = 0
    //    cursor = knob.getPos()
    //  else
    //    cursor = 9
    //    offset = knob.getPos() - 9
    if (knob.getPos() < 9)
    {
      targetOffset = 0;
      targetCursor = knob.getPos();
    }
    else
    {
      targetCursor = 9;
      targetOffset = knob.getPos() - 9;
    }
  }

  else if (state == UPDATE)
  {
    // redraw the list, from the offset to offset+9
    // draw the selection box (offset Y + (cursor*10))
    byte range;
    if (knob.getRange() < 10)
    {
      range = knob.getRange();
    }
    else
    {
      range = 10;
    }
    for (byte i = 0; i < range; i++)
    {
      tft.setCursor(1, 11+(i*10));
      tft.print(networkNodes[i].ssid);
    }

    
    state = RUNNING; 
  }

  else if (state == RUNNING)
  {
    if (knob.hasChanged())
    {      
      clearSelectionBox(selectionIcon);
      //if changed
      //  clear selectionbox
      //  change = knobpos - cursor
      //  if change < 0:
      //    cursor = 0
      //    offset = knobpos
      //    state = update
      //  else if change > 9
      //    cursor = 9
      //    offset = knobpos - 9
      //    state = update
      //  else
      //    draw selection box (offset Y + (cursor*10)

      int change = knob.getPos() - targetOffset;
      if (change < 0)
      {
        targetCursor = 0;
        targetOffset = knob.getPos();
        state = UPDATE;
      }
      else if (change > 9)
      {
        targetCursor = 9;
        targetOffset = knob.getPos() - 9;
        state = UPDATE;
      }
      else
      {
        targetCursor = change;
        selectionIcon.y = 10 + (targetCursor * 10);
        drawSelectionBox(selectionIcon);
        
      }
    }

    if (b1.isPressed() == 1)
    {
      setTargetInfo(knob.getPos());
      targetting = true;
      changeMode(TRACKER);
    }
    if (b2.isPressed() == 1)
    {
      targetting = false;
      WiFiClient.disconnect();
      changeMode(MAINMENU);
    }
    if (b3.isPressed() == 1)
    {
      changeMode(TRACKER);
    }


    //if button1.isPressed
    //  target = networkNodes[knobpos]
    //  targetting = true
    //  changeMode(tracker)
    //if button2.isPressed
    //  targetting = false
    //  changeMode(mainmenu)   

    
  }
  
}
void setTargetInfo(int t)
{
  target.ssid = networkNodes[t].ssid;
  target.node = networkNodes[t].node;
  target.rssi = networkNodes[t].rssi;
  WiFiClient.connect(networkNodes[t].ssid, "signpost");
}


void updateTargetInfo()
{
  
  if (!targetting)
    return;
  /*
  int numSsids = WiFi.scanNetworks();
  if (numSsids >0)
  {
    for (int i=0; i < numSsids; i++)
    {
      strBuffer = WiFi.SSID(i);
      if (target.ssid == strBuffer)
      { // found target. set info, which isn't much really.
        target.rssi = WiFi.RSSI(i);
        target.node = i;
        lastTargetUpdate = millis();
        break;
      }
    } 
  }
  */
  target.rssi = WiFi.RSSI();
  lastTargetUpdate = millis();
}

void runTracker()
{
  if (state == STARTING)
  {
    state = UPDATE;
    changeTitle(S_TRACKER);
    changeButtonLabels(S_BUTTONS3);
    statusInfo.Update = true;
  }



  else if (state == UPDATE)
  {
    //if targetting
    //  updateTargetInfo
    //  display target info
    if (targetting)
    {
      updateTargetInfo();
      tft.setCursor(10,20);
      tft.print(localisation[S_TARGET]);
      tft.print(target.ssid);
      tft.setCursor(10,30);
      tft.print(localisation[S_TARGETSTRENGTH]);
      tft.print(target.rssi);
      tft.setCursor(10,40);
      long lastUpdate = millis() - lastTargetUpdate;
      if (lastUpdate > 3000)
      { // lost the target temporarily
        tft.print("*Losing Target*");
      }
      if (lastUpdate > 20000)
      { // lost target completely. stop tracking
        tft.setCursor(10,20);
        tft.print("Target Lost!   ");
        targetting = false;
        WiFiClient.disconnect();
      }
    }

    //else
    //  draw splash screen
    else
    {
      tft.setCursor(20,20);
      tft.println("Select a target");
      tft.println("(and localise this)");
      tft.println("(and stick a graphic here)");
    }
    
        
    stateDelay = 250; // update every half second
    state = RUNNING;  
  }




  else if (state == RUNNING)
  {
    if (targetting)
    {
      stateDelay--;
      if (stateDelay <=0)
        state = UPDATE;
    }
    
    if (b1.isPressed() == 1)
    {
      changeMode(NODECHOOSER);
    }
    
    if (b2.isPressed() == 1)
    {
      state = SHUTDOWN;
    }
  }

  else if (state == SHUTDOWN)
  {
    targetting = false;
    WiFiClient.disconnect();
    changeMode(MAINMENU);
  }
}


// This is broken away as we have to do it from multiple
// functions (tracker, sending messages).

/*


void updateTargetList()
{
  for (byte x = 0; x < 30; x++)
  {
    nodes[x].lastUpdate += 1;
    if (nodes[x].lastUpdate > 20)
    {
      // it's probably expired out. 20 seconds since
      // we saw it last.  wipe it
      nodes[x].node = 255;
    }
  }


  if (millis() > updateTargetListCountdown)
  {
    updateTargetListCountdown = millis() + updateTargetListDelay;
    byte numSSIDs = WiFi.scanNetworks();
    bool nodeFound;
    byte i;
    byte x;
    for (i = 0; i < numSSIDs; i++)
    {
      nodeFound = false;
      String SSID = WiFi.SSID(i);
      for (x = 0; x < 30; x++)
      {
        if (nodes[x].SSID == SSID)
        { // it's the node in question. refresh it.
          nodes[x].node = i;
          nodes[x].signalStrength = WiFi.RSSI(i);
          nodes[x].lastUpdate = 0;
          nodeFound = true;
          break;
        }
      }
      if (!nodeFound)
      {
        for (x = 0; x < 30; x++)
        {
          if (nodes[x].node == 255)
          {
            // found an empty spot
            nodes[x].node = i;
            nodes[x].SSID = SSID;
            nodes[x].signalStrength = WiFi.RSSI(i);
            nodes[x].lastUpdate = 0;
            break;
          }
        }
        // if it falls out here then there's no space
        // to add it.  We'll probably have to filter for
        // wipboy-specific nodes in crowded areas.
      }
    }
  }
}
*/

//=========================================
//=========================================
void runStats()
{
  if (state == STARTING)
  {
    tft.setCursor(10, 20);
    tft.print("Feature not enabled");
    state = RUNNING;
    changeTitle(S_STATUS);
    changeButtonLabels(S_BUTTONS2);
    //changeString(S_STATUS, statusInfo.title, sizeof(statusInfo.title));
    //changeString(S_BUTTONS2, statusInfo.buttonLabels,sizeof(statusInfo.buttonLabels));
    statusInfo.Update = true;
  }
  else if (state == RUNNING)
  {

    if (b2.isPressed() == 1)
    {
      state = SHUTDOWN;
    }
  }
  else if (state == SHUTDOWN)
  {

    changeMode(MAINMENU);
  }
}



//=========================================
//=========================================
void runSettings()
{
  // if starting
  //  black out the main area (we'll be altering it)
  //  draw the different options
  //  set the knob to the number of choices
  // if update
  //  this is main update, redraw selection box
  // if running
  //  check for knob move, redraw
  //  check for button presses
  // if brightness
  //  set knob to 100
  //  check for knob move, 
  //  if move
  //    erase old bar
  //    draw new bar
  //    set brightness (write to pin)
  //    if button press
  //      return to settings or menu (based on button)
  // if fgcolour
  //  run submode runFGColourSelect
  // if bgcolour
  //  run submode runBGColourSelect 
  // if adminPassword
  //  print to screen admin password
  
  if (state == STARTING)
  {
    Serial.println("settings starting");
    state = RUNNING;
    changeTitle(S_SETTINGS);
    changeButtonLabels(S_BUTTONS3);
    statusInfo.Update = true;
    // clear edit area. 
    tft.fillRect(8,10,152,109,bgColour);
    // TODO: draw the settings icon
    
    // display selection options
    for (byte i=0; i < 4; i++)
    {
      tft.setCursor(settingIcons[i].x+settingIcons[i].offsetX, settingIcons[i].y+settingIcons[i].offsetY);
      tft.print(localisation[settingIcons[i].title]);
    }
    // TODO: draw the selection box
    knob.setRange(4);
    
    lastSelected = knob.getPos();
    //drawSelectionBox(settingIcons[lastSelected]);
  }
  else if (state == RUNNING)
  {
    // check the knob
    // if hasChanged()
    //  clear selection box
    //  lastSelected
    //  draw selection box
    if (knob.hasChanged())
    {
      Serial.println("Knob changed");
      clearSelectionBox(settingIcons[lastSelected]);
      lastSelected = knob.getPos();
      drawSelectionBox(settingIcons[lastSelected]);
    }
    
    if (b2.isPressed()==1)
    {
      Serial.println("b2 pressed");
      state = SHUTDOWN;
    }
    else if (b1.isPressed()==1)
    {
      Serial.println("b1 pressed");
      Mode = settingIcons[lastSelected].Mode;
      // I'm not using changeMode() because I don't want to undraw the settings window
      state = STARTING;
    }
  }
  else if (state == SHUTDOWN)
  {
    changeMode(MAINMENU);
  }
}


void runFGColour()
{
  // clear the draw box area
  // draw the selections
  // draw the selection box
  if (state == STARTING)
  {
    state = RUNNING;


    for (byte i=0; i < 12; i++)
    {
      
      tft.fillRect(
        colourIcons[i].x + colourIcons[i].offsetX,
        colourIcons[i].y + colourIcons[i].offsetY,
        colourIcons[i].w,
        colourIcons[i].h,
        colours[i]);
    }
    knob.setRange(12);
    lastSelected = knob.getPos();
    
    drawSelectionBox(colourIcons[lastSelected]);
  }
  
  // check the knob move
  // if moved
  //  clear selection box
  //  draw the selection box
  else if (state == RUNNING)
  {
    if (knob.hasChanged())
    {
      clearSelectionBox(colourIcons[lastSelected]);
      lastSelected = knob.getPos();
      drawSelectionBox(colourIcons[lastSelected]);
    }
    
    // if button1 (select)
    //  set the colour to the colours[selected]
    //  changeMode(settings)
    if (b1.isPressed()==1)
    {
      fgColour = colours[lastSelected];
      tft.setTextColor(fgColour, bgColour);
      changeMode(SETTINGS);
    }
    
    // if button2 (mainmenu)
    //  set the colour to the colours[selected]
    //  changeMode(mainmenu)
    else if (b2.isPressed()==1)
    {
      fgColour = colours[lastSelected];
      tft.setTextColor(fgColour, bgColour);
      changeMode(MAINMENU);
    }
    
    // if button3(cancel)
    //  changeMode(settings)
    else if (b3.isPressed())
      changeMode(SETTINGS);
  }
}

void runBGColour()
{
  // clear the draw box area
  // draw the selections
  // draw the selection box
  if (state == STARTING)
  {
    state = RUNNING;


    for (byte i=0; i < 12; i++)
    {
      
      tft.fillRect(
        colourIcons[i].x + colourIcons[i].offsetX,
        colourIcons[i].y + colourIcons[i].offsetY,
        colourIcons[i].w,
        colourIcons[i].h,
        colours[i]);
    }
    knob.setRange(12);
    lastSelected = knob.getPos();
    
    drawSelectionBox(colourIcons[lastSelected]);
  }
  
  // check the knob move
  // if moved
  //  clear selection box
  //  draw the selection box
  else if (state == RUNNING)
  {
    if (knob.hasChanged())
    {
      clearSelectionBox(colourIcons[lastSelected]);
      lastSelected = knob.getPos();
      drawSelectionBox(colourIcons[lastSelected]);
    }
    
    // if button1 (select)
    //  set the colour to the colours[selected]
    //  changeMode(settings)
    if (b1.isPressed()==1)
    {
      bgColour = colours[lastSelected];
      tft.setTextColor(fgColour, bgColour);
      changeMode(SETTINGS);
    }
    
    // if button2 (mainmenu)
    //  set the colour to the colours[selected]
    //  changeMode(mainmenu)
    else if (b2.isPressed()==1)
    {
      bgColour = colours[lastSelected];
      tft.setTextColor(fgColour, bgColour);
      changeMode(MAINMENU);
    }
    
    // if button3(cancel)
    //  changeMode(settings)
    else if (b3.isPressed())
      changeMode(SETTINGS);
  }
}


//=========================================
//=========================================
void runMessenger()
{
  if (state == STARTING)
  {
    tft.setCursor(10, 20);
    tft.print("Feature not enabled");
    state = RUNNING;
    changeTitle(S_MESSENGER);
    changeButtonLabels(S_BUTTONS2);
    //changeString(S_MESSENGER, statusInfo.title, sizeof(statusInfo.title));
    //changeString(S_BUTTONS2, statusInfo.buttonLabels,sizeof(statusInfo.buttonLabels));
    statusInfo.Update = true;
  }
  else if (state == RUNNING)
  {

    if (b2.isPressed() == 1)
    {
      state = SHUTDOWN;
    }
  }
  else if (state == SHUTDOWN)
  {

    changeMode(MAINMENU);
  }
}









// =======================================
// Supporting Functions


void drawMenu()
{
  for (int i = 0; i < 4; i++)
  {
    tft.drawBitmap(menuIcons[i].x + menuIcons[i].offsetX, menuIcons[i].y + menuIcons[i].offsetY, menuIcons[i].img, menuIcons[i].w, menuIcons[i].h, fgColour);
  }
}




void drawSelectionBox(Icon icon)
{
  tft.drawRoundRect(icon.x, icon.y, icon.sbw, icon.sbh, 1, fgColour);

}

void clearSelectionBox(Icon icon)
{
  /*
  byte x = icons[selected].x;
  byte y = icons[selected].y;
  byte w = icons[selected].sbw;
  byte h = icons[selected].sbh;
  */
  tft.drawRoundRect(icon.x, icon.y, icon.sbw, icon.sbh, 1, bgColour);
}









// changeMode()
// This function changes the mode.  It also does any housekeeping
// mode changing entails without having to worry about what that
// is in the other functions.  Not that there is much ATM, but
// enough to make this worthwhile. Plus more in the future

void changeMode(byte newMode)
{
  tft.fillScreen(bgColour);
  Mode = newMode;
  state = STARTING;
}


// backgroundUpdate()
// All the stuff that has to run in the background.
void backgroundUpdate()
{
  dnsServer.processNextRequest();
  webServer.handleClient();

  // TODO: if buzzer is active, count down the delay on it.
  // if the delay reaches 0 then shut the buzzer down.
  // (TBH it may be more of a "clicker" than a "buzzer". we
  //  can't really manage the microseconds to make it buzz.
  //  This may be a motor instead)
}



// checkWebRequest()
// This is a dummy function ATM, but eventually this will
// handle all the coordination between smartphones/other wipboys
// and this one.  They'll all pass info to each other via web
// requests.  Why? Because it's easy.
void checkWebRequest()
{
  if (webServer.hasArg("mypassword"))
  { // change "mypassword" to test against the internal password in Settings
    // only perform actions if the password is supplied

    // ...
    // do other things based on the intput. This would be where
    // we also receive commands to send messages or change settings
  }
  else if (webServer.hasArg("carrierPassword"))
  { // this is the shared password between all the devices. For now
    // we will set this manually.  Eventually we'll have a central
    // hub maybe, or a way of mesh sharing this.

  }
}


// statusBarUpdate()
// This function checks if the status bar needs to be re-rendered.
// By doing this we only render once every few seconds, which
// stops flickering, energy usage, cpu cycles wasted, etc.
// lots of good things accomplished through programmatical laziness.

void statusBarUpdate()
{
  // if we know we need to update
  if (statusInfo.Update)
  {
    // if the status information has changed, re-display top bar
    if (statusInfo.displayTop)
    {
      tft.setCursor(0, 0);
      for (int i = 0; i < sizeof(statusInfo.title) - 1; i++)
      {
        tft.print(statusInfo.title[i]);
      }
    }
    // mail icon
    // TODO BETTER
    if (statusInfo.msgs)
    {
      tft.setCursor(145, 0);
      tft.print("m!");
    }
    else
    {
      tft.setCursor(145, 0);
      tft.print("  ");
    }

    // bottom buttons label
    if (statusInfo.displayTop)
    {
      tft.setCursor(0, 120);
      for (int i = 0; i < sizeof(statusInfo.buttonLabels) - 1; i++)
      {
        tft.print(statusInfo.buttonLabels[i]);
      }
    }
    statusInfo.Update = false;
  }
}

// changeString()
// this is my unhappy implementation of a string combining
// method.  You say what string inside the localisation array
// you want, where you want to copy it to, and how long
// it is, and it'll go to town.

// The reason this isn't more simple is because we have to
// update the buffer space on the screen with blank spaces
// to wipe out previous text.  It's the only way to render
// things without flickering on the ST7735.

void changeString(byte string, char *Buffer, byte len)
{
  localisation[string].toCharArray(Buffer, len);
  for (int i = localisation[string].length(); i < len - 1; i++)
  {
    Buffer[i] = ' ';
  }
}



// changeTitle() and changeButtonLabels()
// These are better shortcuts to update specific buffers. It's
// a bit of hardcoding unpleasantness to solve specific
// instances, but it's done so often it makes it worthwhile.

void changeTitle(byte string)
{
  localisation[string].toCharArray(statusInfo.title, TITLE_BUFFER_SIZE);
  for (int i = localisation[string].length(); i < TITLE_BUFFER_SIZE - 1; i++)
  {
    statusInfo.title[i] = ' ';
  }
  statusInfo.Update = true;
}

void changeButtonLabels(byte string)
{
  localisation[string].toCharArray(statusInfo.buttonLabels, LABELS_BUFFER_SIZE);
  for (int i = localisation[string].length(); i < LABELS_BUFFER_SIZE - 1; i++)
  {
    statusInfo.buttonLabels[i] = ' ';
  }
  statusInfo.Update = true;
}


void buildIcons()
{
  /*
  for (byte i=0; i<4; i++)
  {
    menuIcons[i] = Icon(10,10);
  }*/
  menuIcons[0].x = 10;
  menuIcons[0].y = 10;
  menuIcons[0].Mode = TRACKER;
  menuIcons[0].title = S_TRACKER;
  menuIcons[0].img = i_messages;
  menuIcons[1].x = 60;
  menuIcons[1].y = 10;
  menuIcons[1].Mode = MESSENGER;
  menuIcons[1].title = S_MESSENGER;
  menuIcons[1].img = i_messages;
  menuIcons[2].x = 110;
  menuIcons[2].y = 10;
  menuIcons[2].Mode = STATUS;
  menuIcons[2].title = S_STATUS;
  menuIcons[2].img = i_messages;
  menuIcons[3].x = 10;
  menuIcons[3].y = 68;
  menuIcons[3].Mode = SETTINGS;
  menuIcons[3].title = S_SETTINGS;
  menuIcons[3].img = i_pipboy;

  /*
  for (byte i=0; i<4; i++)
  {
    settingIcons[i] = Icon(10,10);
  }*/
  settingIcons[0].x = 0;
  settingIcons[0].y = 15;
  settingIcons[0].w = 62;
  settingIcons[0].h = 10;
  settingIcons[0].offsetX=1;
  settingIcons[0].offsetY=1;
  settingIcons[0].sbw = 62;
  settingIcons[0].sbh = 10;
  settingIcons[0].title = S_SETTINGS0;
  settingIcons[0].Mode = ADJUSTFGCOLOUR;
  settingIcons[1].x = 0;
  settingIcons[1].y = 25;
  settingIcons[1].w = 62;
  settingIcons[1].h = 10;
  settingIcons[1].offsetX=1;
  settingIcons[1].offsetY=1;
  settingIcons[1].sbw = 62;
  settingIcons[1].sbh = 10;
  settingIcons[1].title = S_SETTINGS1;
  settingIcons[1].Mode = ADJUSTBGCOLOUR;
  settingIcons[2].x = 0;
  settingIcons[2].y = 35;
  settingIcons[2].w = 62;
  settingIcons[2].h = 10;
  settingIcons[2].offsetX=1;
  settingIcons[2].offsetY=1;
  settingIcons[2].sbw = 62;
  settingIcons[2].sbh = 10;
  settingIcons[2].title = S_SETTINGS2;
  settingIcons[2].Mode = ADJUSTFGCOLOUR;
  settingIcons[3].x = 0;
  settingIcons[3].y = 45;
  settingIcons[3].w = 62;
  settingIcons[3].h = 10;
  settingIcons[3].offsetX=1;
  settingIcons[3].offsetY=1;
  settingIcons[3].sbw = 62;
  settingIcons[3].sbh = 10;
  settingIcons[3].title = S_SETTINGS3;
  settingIcons[3].Mode = ADJUSTFGCOLOUR;

  byte x;
  byte y;
  byte c = 0;
  for (y=0; y < 3; y++)
  {
    for (x=0; x < 4; x++)
    {
      //colourIcons[c] = Icon(65+(x*25), 15+(y*25));
      colourIcons[c].x = 65 + (x*20);
      colourIcons[c].y = 15 + (y*20);
      colourIcons[c].offsetX = 2;
      colourIcons[c].offsetY = 2;
      colourIcons[c].w = 15;
      colourIcons[c].h = 15;
      colourIcons[c].sbw = 19;
      colourIcons[c].sbh = 19;
      c++; 
    }
  }

  // target node selector icon.  We use this to just keep track of
  // the selection box position
  selectionIcon.x = 0;
  selectionIcon.y = 10;
  selectionIcon.h = 10;
  selectionIcon.w = 30;
  selectionIcon.sbh = 10;
  selectionIcon.sbw = 160;
}
