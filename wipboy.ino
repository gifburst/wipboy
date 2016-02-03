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
#include "./Wipboy_Gfx.h"
#include "./Constants.h"

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
#define TFT_CS     151 // The CS is tied to gnd and
#define TFT_RST    150 // the RST is tied to the ESP's RST.  It's easier to just give the library fake pins
#define TFT_DC     2   // than to edit the library.
#define TFT_BACKLIGHT 47  // TODO: wire backlight when we get the transitors come in

uint16_t fgColour = 0x6D45;
uint16_t bgColour = ST7735_BLACK;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);



// ------------------------
// webserver
// This is the section for defining the webserver and dns server.
// We can also add our HTML webpages in here as strings by
// copying/pasting.

#define ADMIN_PW "12345"
char WIFI_PW[] = "asdf";

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
// statusinfo
// This is a struct we use to just organise all
// the status info on the top and bottom of the
// display.  In theory we could just use global
// variables instead, but I wanted to experiment
// with organising in structs instead.

// TODO: implement the buzzer/motor status/timer stuffs
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



// Ok, instantiating arrays of objects shouldn't be this hard.  This is just dumb.
// TODO: fix this
Icon menuIcons[4] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};
Icon settingIcons[4] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};
Icon colourIcons[12] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10),Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10),Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};

// This is so when we change the selection box we know what the
// previous selected icon was. That way we can clear out the old
// one.
byte lastSelected = 0;


// ------------------------
// Modes and States
byte state = STOPPED;
int stateDelay = 0;   // stateDelay is a counter in case we need to pause for a bit
byte Mode = STOPPED;



// ------------------------
// This is just clearing enough memory to scan up to 64 networks.  Yeah, that's a lot.  
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

String strBuffer;  // these are used to draw the list to the screen.  I wanted it scrollable so am
byte targetOffset; // using a trick from video gaming and cameras to do it.
byte targetCursor;

Icon selectionIcon = Icon(10,10); // This is so we have a simple way of rendering a selection box

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
  Mode = STATUS;
  setMenuState();
}



/*
 * if state == MENUSTATE // This state is when they choose the menu.  check the knob, change if need be
 *  if knob.changed
 *    changemode(newMode) // this will run the "starting" section of the mode.  which will then drop itself back into MENUSTATE probably
 *  // if it's not menustate, we're still running whatever mode we like, and it can manage on its own then toggle back into menustate when it's done updating
 * 
 * |------------------------|
 * STATS|Trkr|Msgs|CFG|Radio
 */

byte modePos = 0;
void loop()
{
  backgroundUpdate();
  statusBarUpdate();

  if (state == MENUSTATE)
  {
    if (knob.hasChanged())
    {
      changeMode(ModesRef[knob.getPos()]);
      modePos = knob.getPos();
      statusInfo.Update = true;
    }
  }

  switch (Mode) // you know, this didn't give me anything better than the if statements in terms of space saving.
  {             // I'm going to convert it back.
    case TRACKER:
      runTracker();
    break;
    case MESSENGER:
      runMessenger();
    break;
    case STATUS:
      runStats();
    break;
    case SETTINGS:
      runSettings();
    break;
    case ADJUSTFGCOLOUR:
      runFGColour();
    break;
    case ADJUSTBGCOLOUR:
      runBGColour();
    break;
    case NODECHOOSER:
      runNodeChooser();
    break;
    case SHOWADMINPW:
      runAdminPW();
    break;
  }

}
// =======================================
// Loop
void oldloop()
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
  else if (Mode == SHOWADMINPW)
  {
    runAdminPW();
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
    knob.setRange(4);
    lastSelected = knob.getPos();
    state = UPDATE;
    changeTitle(menuIcons[knob.getPos()].title);
    statusInfo.Update = true;
    tft.drawBitmap(80,15, g_logo, 72, 40, fgColour);
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
        Serial.println("runNodeChooser running");
        drawSelectionBox(selectionIcon);
        
      }
    }

    if (b1.isPressed() == 1)
    {
      setTargetInfo(knob.getPos());
      targetting = true;
      changeMode(TRACKER);
    }    
  }
}


void setTargetInfo(int t)
{
  target.ssid = networkNodes[t].ssid;
  target.node = networkNodes[t].node;
  target.rssi = networkNodes[t].rssi;
  lastTargetUpdate = millis();
  
  char buf[31];
  target.ssid.toCharArray(buf, 31);  // there's gotta be a better way to do this
  WiFi.begin(buf, WIFI_PW);
  
  tft.setCursor(10,0);
  tft.print(" Locking On ");
  
  bool connecting = true;
  while (connecting)
  {
    delay(250);
    if (WiFi.status() == WL_CONNECTED)
    {
      connecting = false;
      lastTargetUpdate = millis();
      targetting = true;
    }
    else if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_CONNECTION_LOST)
    {
      tft.setCursor(10,0);
      tft.print(" Lock Failed ");
      delay(1000);
      connecting = false;
      targetting = false;      
    }
    else if (millis() - lastTargetUpdate > 30000)
    {
      tft.setCursor(10,0);
      tft.print(" Lock Timeout ");      
      delay(1000);
      connecting = false;
    } 
  }
}

void setMenuState()
{
  state = MENUSTATE;
  knob.setRange(5);
  lastSelected = knob.getPos();
}

void updateTargetInfo()
{
  if (!targetting)
    return;
  
  if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_CONNECTION_LOST)
  { // lockon failed
    lastTargetUpdate = millis() - 20001;
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    target.rssi = WiFi.RSSI();
    lastTargetUpdate = millis();
  }
}



void runTracker()
{
  if (state == STARTING)
  {
    state = UPDATE;
    changeTitle(S_TRACKER);

    statusInfo.Update = true;
    tft.drawBitmap(7, 25, g_targetting, 64, 74, fgColour);
    tft.drawLine(80, 25, 80, 103, fgColour);
    tft.setCursor(85,20);
    tft.print(localisation[S_TARGET]);
    tft.setCursor(85,40);
    tft.print(localisation[S_TARGETSTRENGTH]);


  }



  else if (state == UPDATE)
  {
    //if targetting
    //  updateTargetInfo
    //  display target info
    if (targetting)
    {
      updateTargetInfo();
      tft.setCursor(85,30);
      tft.print(target.ssid);
      tft.setCursor(85, 50);
      if (target.rssi < 0)
      {
        tft.print(target.rssi);
      }
      tft.setCursor(85, 70);
      long lastUpdate = millis() - lastTargetUpdate;

      
      if (lastUpdate > 20000)
      { // lost target completely. stop tracking
        tft.print("TARGET LOST!");
        targetting = false;
        WiFi.disconnect();
        //WiFiClient.disconnect();
      }
      else if (lastUpdate > 3000)
      { // lost the target temporarily
        tft.print("*Losing Tgt*");
      }
      else
      {
        tft.print("         ");     
      }
    }

    //else
    //  draw splash screen
    
        
    stateDelay = 500; // update every half second
    setMenuState();
  }




  else if (state == MENUSTATE)
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
  }
}



//=========================================
//=========================================
void runStats()
{
  if (state == STARTING)
  {
    tft.setCursor(10, 20);
    tft.print("Feature not enabled");
    setMenuState();
    changeTitle(S_STATUS);

    //changeString(S_STATUS, statusInfo.title, sizeof(statusInfo.title));
    //changeString(S_BUTTONS2, statusInfo.buttonLabels,sizeof(statusInfo.buttonLabels));
    statusInfo.Update = true;
    tft.drawBitmap(64, 30, g_stats, 32, 76, fgColour);
  }
  else if (state == MENUSTATE)
  {

    if (b1.isPressed() == 1)
    {
      state = SHUTDOWN;
    }
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
    setMenuState();
    changeTitle(S_SETTINGS);

    // clear edit area. 
    tft.fillRect(76,10,152,109,bgColour);
    // TODO: draw the settings icon
    tft.drawBitmap(87, 25, g_settings, 56, 66, fgColour);
    tft.drawLine(75,12, 75, 113, fgColour);
    
    // display selection options
    for (byte i=0; i < 4; i++)
    {
      tft.setCursor(settingIcons[i].x+settingIcons[i].offsetX, settingIcons[i].y+settingIcons[i].offsetY);
      tft.print(localisation[settingIcons[i].title]);
    }
  }

  else if (state == MENUSTATE)
  {
    if (b1.isPressed() == 1)
    {
      // we've entered the selection window.  change the state to stop menu choosing and set the knob.
      knob.setRange(4);
      lastSelected = knob.getPos();
      drawSelectionBox(settingIcons[lastSelected]);
      state = INMODE;
    }
  }
  
  else if (state == INMODE)
  {
    // check the knob
    // if hasChanged()
    //  clear selection box
    //  lastSelected
    //  draw selection box
    if (knob.hasChanged())
    {
      clearSelectionBox(settingIcons[lastSelected]);
      lastSelected = knob.getPos();
      drawSelectionBox(settingIcons[lastSelected]);
    }
    
    else if (b1.isPressed()==1)
    {
      Serial.println("b1 pressed");
      Mode = settingIcons[lastSelected].Mode;
      // I'm not using changeMode() because I don't want to undraw the settings window
      state = STARTING;
    }
  }
}


void runAdminPW()
{
  if (state == STARTING)
  {
    tft.setCursor(80, 100);
    tft.print(ADMIN_PW);
    setMenuState();
    Mode = SETTINGS;
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
    tft.fillRect(76,10,152,109,bgColour);

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

    Serial.println("runFGColour starting");
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
      lastSelected = 4;
      changeMode(SETTINGS);
    }
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
    tft.fillRect(76,10,152,109,bgColour);

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
      Serial.println("runBGColour running");
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
    setMenuState();
    changeTitle(S_MESSENGER);
    statusInfo.Update = true;
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
  Serial.println("in drawSelectionBox");
  Serial.println(icon.x);
  Serial.println(icon.y);
  Serial.println(icon.sbw);
  Serial.println(icon.sbh);
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
  
  if (Mode == TRACKER) // this is a bad place to have this. we need to objectize the modes! TODO!
  {
    targetting = false;
    WiFi.disconnect();
  }
  // setting the default for the menustate just in case we need it
  knob.setRange(5);
  lastSelected = modePos;
  
  
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
      tft.drawLine(0,3,60,3, fgColour);
      tft.setCursor(10, 0);
      tft.print(' ');
      for (int i = 0; i < sizeof(statusInfo.title) - 1; i++)
      {
        tft.print(statusInfo.title[i]);
      }
      tft.drawLine(84,3,160,3, fgColour);
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
      tft.setCursor(140, 0);
      tft.print("  ");
    }

    // bottom buttons label
    if (statusInfo.displayBottom)
    {
      tft.drawLine(0,118, 160, 118, fgColour);
      tft.setCursor(0, 120);
      Serial.print("modestring length: ");
      Serial.println(sizeof(ModesStrings));
      tft.print("|");
      for (int i=0; i < 5; i++)
      {
        if (i == modePos)
        {
          tft.setTextColor(bgColour, fgColour);
          tft.print(ModesStrings[i]);
          tft.setTextColor(fgColour, bgColour);
        }
        else
        {
          tft.print(ModesStrings[i]);
        }
        tft.print("|");
      }
      /*
      for (int i = 0; i < sizeof(statusInfo.buttonLabels) - 1; i++)
      {
        tft.print(statusInfo.buttonLabels[i]);
      }
      */
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
  menuIcons[0].img = i_lightbulb;
  menuIcons[1].x = 10;
  menuIcons[1].y = 68;
  menuIcons[1].Mode = MESSENGER;
  menuIcons[1].title = S_MESSENGER;
  menuIcons[1].img = i_messages;
  menuIcons[2].x = 60;
  menuIcons[2].y = 68;
  menuIcons[2].Mode = STATUS;
  menuIcons[2].title = S_STATUS;
  menuIcons[2].img = i_messages;
  menuIcons[3].x = 110;
  menuIcons[3].y = 68;
  menuIcons[3].Mode = SETTINGS;
  menuIcons[3].title = S_SETTINGS;
  menuIcons[3].img = i_settings;

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
  settingIcons[2].Mode = ADJUSTBRIGHTNESS;
  settingIcons[3].x = 0;
  settingIcons[3].y = 45;
  settingIcons[3].w = 62;
  settingIcons[3].h = 10;
  settingIcons[3].offsetX=1;
  settingIcons[3].offsetY=1;
  settingIcons[3].sbw = 62;
  settingIcons[3].sbh = 10;
  settingIcons[3].title = S_SETTINGS3;
  settingIcons[3].Mode = SHOWADMINPW;

  byte c = 0;
  for (byte y=0; y < 3; y++)
  {
    for (byte x=0; x < 4; x++)
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
