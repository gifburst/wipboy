/*
    Wip-Boy 2000. ESP8266-based wristband game system.
    Copyright (C) 2016 Richard Woodward-Roth

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <ESP8266WiFi.h>
#include "./DNSServer.h"                  // Patched lib
#include <ESP8266WebServer.h>

#include <Adafruit_GFX.h>       // Core graphics library
#include "./Adafruit_ST7735.h"  // Hardware-specific library - patched for esp
#include <SPI.h>
#include <MFRC522.h>
#include "./Slider.h"
#include "./Button.h"
#include "./Icon.h"

#include "./WipboyNode.h"
#include "./Wipboy_Gfx.h"
#include "./Constants.h"

#include "./Quest.h"

// =======================================
// Declare


/*
 * Pinout for the ESP-12:
 * GPIO 0 - btn to gnd
 * GPIO 1 - 
 * GPIO 2 - buzzer
 * GPIO 3 - 
 * GPIO 4 - RFID SS
 * GPIO 5 - RFID RST
 * GPIO12 - SPI MISO (RFID)
 * GPIO13 - SPI MOSI (RFID, TFT)
 * GPIO14 - SPI CLK (RFID, TFT)
 * GPIO15 - 10k resistor to gnd (open, still useful)
 * GPIO16 - DC on LCD 
 * ADC - control knob
 * 
 * 
 * Given I want to connect the TFT CS back up, it'll
 * probably go onto GPIO15.  Pinouts say to use it too so
 * I doubt it'll be much trouble.  Will test soon.
 * 
 * This leaves specifically 2 GPIO left: 1 and 3.  It's
 * not much, but enough to run I2C.  If the I2C is hard-
 * wired for 4/5 then we just swap the RFID and the I2C.
 * We can't start the I2C (radio/mp3 player) until we get
 * OTA updating working, which is on the back burner ATM.
 * 
 * *If* I want the LEDs on the front it'll have to be
 * through the I2C as well.  Hopes aren't high for those,
 * I'll probably fill those in on future models.
 */

// ---------------------------------------------------
// ---------------------------------------------------
// screen
#define TFT_CS     2 // The CS is tied to gnd and
#define TFT_RST    150 // the RST is tied to the ESP's RST.  It's easier to just give the library fake pins than to edit the library.
#define TFT_DC     16   // A0 on the screen

uint16_t fgColour = 0x6D45;
uint16_t bgColour = ST7735_BLACK;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);


Quest Quests[] = 
{
  Quest(), Quest(), Quest(),
  Quest(), Quest(), Quest(),
  Quest(), Quest(), Quest()
};
// ---------------------------------------------------
// ---------------------------------------------------
//

const byte        DNS_PORT = 53;
IPAddress         apIP(10, 10, 10, 1);
DNSServer         dnsServer;
ESP8266WebServer  webServer(80);

String homepageHTML = "<html><head></head><body>Nothing here at the moment.  Coming soon: phone sync'ing</body></html>";
String HTMLHead = "<html><head><title>Wip-boy Remote!</title><style type=text/css>body{background-color:#000;font-family:Monospace,Lucida Console}.mainBody{border-radius:15px;border:2px solid #59955c;float:left;background-color:#030;color:#fff;background:-webkit-gradient(radial,center center,10,center center,900,from(#39652c),to(#3e4f63))}.borderz{border-radius:15px;border:5px ridge #c9decb}.borderz2{border-radius:11px;border:4px groove #93bf96}.title{font-weight:700;float:left;padding-right:5px}.hline{padding-right:5px;padding-left:5px}.hline:first-of-type{width:5%;float:left}p{padding-left:15px}.entryForm{width:200px;border:3px ridge #c9decb;padding:5px;border-radius:10px}</style><body><div class=main><div class=mainBody><div class=borderz><div class=borderz2><div class=hline><hr></div><div class=title>";
String HTMLMsgBody1 = "</div><div class=hline><hr></div><br><p>Welcome, vault dweller. Because of the apocalypse, internet and cell phone service is no longer available.<br><br>But fear not! With this interface and your standard-issue Wip-Boy you can again reach out to talk to someone! Just choose your recipient, type your message, and they will receive it on their Wip-boy (assuming they are in range).</p><div class=entryForm><form action=wipboy.com/post method=POST><select name=Target>";
String HTMLMsgBody2 = "</select><br><input name=message maxlength=140> <input type=hidden name=passkey value=";
String HTMLMsgBody3 = "><input type=submit value=Send></form></div></div></div></div></div></body></html>";
String HTMLLoginBody1 = "</div><div class=hline><hr></div><p>Type in the password in your Wip-boy's CFG or STATS window to log in.</p><p><form action=login method=POST><input name=passkey maxlength=8></form></p></div></div></div></div></body></html>";

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



// ---------------------------------------------------
// ---------------------------------------------------
#define RFID_SS   4
#define RFID_RST  5
MFRC522 rfid = MFRC522(RFID_SS, RFID_RST);






// ---------------------------------------------------
// ---------------------------------------------------
// buttons/controls
// TODO: update the Slider object to take min/max values
// and to report current raw value.  That way we can calibrate
// per knob.
Slider knob = Slider(A0, 4, 24);
Button b1 = Button(0, true);








// ---------------------------------------------------
// ---------------------------------------------------
// statusinfo
// This is a struct we use to just organise all
// the status info on the top and bottom of the
// display.  In theory we could just use global
// variables instead, but I wanted to experiment
// with organising in structs. Though I should 
// really move this to another library

// TODO: implement the buzzer/motor status/timer stuffs
typedef struct
{
  bool Update = true;
  bool displayTop = true;
  bool displayBottom = true;
  bool msgs = false;
  char title[TITLE_BUFFER_SIZE];
  char buttonLabels[LABELS_BUFFER_SIZE];
  bool buzzer = true;
  int buzzerDelay = 0;
  byte level = 1;
  long xp = 0;
} StatusInfo;

StatusInfo statusInfo = StatusInfo{};


// Ok, instantiating arrays of objects shouldn't be this hard.  This is just dumb.
// TODO: fix this
Icon menuIcons[4] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};
Icon settingIcons[4] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};
Icon colourIcons[12] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10),Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10),Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};
Icon mapIcons[4] = {Icon(10,10), Icon(10,10), Icon(10,10), Icon(10,10)};
// This is so when we change the selection box we know what the
// previous selected icon was. That way we can clear out the old
// one.
byte mapSelected = 0;
byte lastSelected = 0;
Icon selectionIcon = Icon(10,10); // This is so we have a simple way of rendering a selection box


// ---------------------------------------------------
// ---------------------------------------------------
// Modes and States
byte State = STOPPED;
int stateDelay = 0;   // stateDelay is a counter in case we need to pause for a bit
byte Mode = STOPPED;
byte modePos = 0;




// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------

void setup() 
{
  Mode = STARTING;
  State = STARTING;
  Serial.begin(115200);

  SPI.begin();

  // ------------------------
  // screen

  //pinMode (TFT_BACKLIGHT, OUTPUT);
  //digitalWrite(TFT_BACKLIGHT, HIGH);

  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.setRotation(3);
  tft.fillScreen(bgColour);
  tft.setTextColor(fgColour, bgColour);
  tft.setTextWrap(false);

  tft.setCursor(0,0);
  tft.println("System initialised");

  // ------------------------
  // webserver
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("Target1425");
  tft.println("Beacon activated");

  dnsServer.start(DNS_PORT, "*", apIP);

  webServer.onNotFound([]() {
    //webServer.send(200, "text/html", homepageHTML);
    serveHomepage();
  });
  webServer.begin();


  // /msg is when a connected phone asks for the messaging screen or tries to send
  // /admin is to allow a phone to log in.  We then need to record the phone's IP
  //   and autoserve them with the password
  
  webServer.on("/login", []()
  {
    serveHomepage();
  });
  webServer.on("/msg", []()
  {
    handleWebMsg();
  
  });
  tft.println("Wifi interface activated");

  // ------------------------
  // rfid
  rfid.PCD_Init();
  tft.println("Scanner activated");
  // ------------------------
  // build the different icons.  doing this to move
  // spammy code to bottom
  buildIcons();
  tft.println("GUI constructed");

  buildQuests();
  compileQuestList();
  checkLevel();
  tft.println("Quests added");

  
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  delay(1);
  digitalWrite(15, LOW);
  
  // Setup complete.  Load menu
  setMenuState();
  modePos = knob.getPos();
  changeMode(ModesRef[knob.getPos()]);

  Serial.println("Setup complete");

}

void loop() 
{
  backgroundUpdate();
  statusBarUpdate();
  
  if (State == MENUSTATE)
  {
    if (knob.hasChanged())
    {
      statusInfo.buzzerDelay = 1;
      changeMode(ModesRef[knob.getPos()]);
      modePos = knob.getPos();
      statusInfo.Update = true;
    }
  }

  if (Mode==TRACKER)
    runTracker();
  else if (Mode==QUESTS)
    runQuests();
  else if (Mode==STATUS)
    runStats();
  else if (Mode==SETTINGS)
    runSettings();
  else if (Mode==RADIO)
    runRadio();
  else if (Mode==MAPS)
    runMaps();
  else if (Mode==ADJUSTFGCOLOUR)
    runFGColour();
  else if (Mode==ADJUSTBGCOLOUR)
    runBGColour();
  else if (Mode==SHOWADMINPW)
    runAdminPW();
  else if (Mode==NODECHOOSER)
    runNodeChooser();
  else if (Mode==RUNPOPUP)
    runPopup();


}


// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------
int scanForNodes()
{
  ////Serial.println("starting scan");
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
  //Serial.println("finished");
  //Serial.println(millis() - delayTest);
  return numSsids;
}







void runNodeChooser()
{
  if (State == STARTING)
  {
    State = UPDATE;
    changeTitle(S_CHOOSENODE);

    selectionIcon.sbw = 60;
   
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

  else if (State == UPDATE)
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

    
    State = RUNNING; 
  }

  else if (State == RUNNING)
  {
    if (knob.hasChanged())
    {      
      statusInfo.buzzerDelay = 1;
      clearSelectionBox(selectionIcon);
      //if changed
      //  clear selectionbox
      //  change = knobpos - cursor
      //  if change < 0:
      //    cursor = 0
      //    offset = knobpos
      //    State = update
      //  else if change > 9
      //    cursor = 9
      //    offset = knobpos - 9
      //    State = update
      //  else
      //    draw selection box (offset Y + (cursor*10)

      int change = knob.getPos() - targetOffset;
      if (change < 0)
      {
        targetCursor = 0;
        targetOffset = knob.getPos();
        State = UPDATE;
      }
      else if (change > 9)
      {
        targetCursor = 9;
        targetOffset = knob.getPos() - 9;
        State = UPDATE;
      }
      else
      {
        targetCursor = change;
        selectionIcon.y = 10 + (targetCursor * 10);
        //Serial.println("runNodeChooser running");
        drawSelectionBox(selectionIcon);
        
      }
    }

    if (b1.isPressed() == 1)
    {
      setTargetInfo(knob.getPos());
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
    tft.print('.');
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
      targetting = false;
    } 
  }
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
  if (State == STARTING)
  {
    State = UPDATE;
    changeTitle(S_TRACKER);

    statusInfo.Update = true;
    tft.drawBitmap(7, 25, g_targetting, 64, 74, fgColour);
    tft.drawLine(80, 25, 80, 103, fgColour);
    tft.setCursor(85,20);
    tft.print(localisation[S_TARGET]);
    tft.setCursor(85,40);
    tft.print(localisation[S_TARGETSTRENGTH]);


  }



  else if (State == UPDATE)
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




  else if (State == MENUSTATE)
  {
    if (targetting)
    {
      stateDelay--;
      if (stateDelay <=0)
        State = UPDATE;
    }
    
    if (b1.isPressed() == 1)
    {
      changeMode(NODECHOOSER);
    }
  }
}

// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------




void printPopupMsg(String* msg)
{
  byte startX = 20;
  byte startY = 15;
  byte endX = 140;
  byte endY = 100;
  tft.fillRoundRect(startX, startY, endX, endY, 3, bgColour);
  tft.drawRoundRect(startX, startY, endX, endY, 3, fgColour);
  tft.setCursor(startX+5, startY+5);
  // we only have 18 characters per line.  We also only have 10 lines
  byte y = 0;
  byte counter = 0;
  for (byte i=0; i < msg->length(); i++)
  {
    counter++;
    if ((counter >= 20) || (msg->charAt(i) == '~'))
    {
      counter = 0;
      y++;
      tft.setCursor(startX+5, startY+5+(y*8));
    }
    if (msg->charAt(i) != '~')
      tft.print(msg->charAt(i));
  }
  Mode = RUNPOPUP;
  State = STARTING;
}

void runPopup()
{
  if (b1.isPressed() == 1)
  {
    setMenuState();
    changeMode(ModesRef[knob.getPos()]); 
    
  }
}


int qBase = 0;
int qCursor = 0;
int qCount = 0;


bool compileQuestList()
{
  qCount = 0;
  Serial.println("-----------");
  Serial.println("Compiling quests");
  for (int i=0; i<QUESTCOUNT; i++)
  {
    Serial.print("Quest: ");
    Serial.println(QuestTitles[Quests[i].title]);
    if (Quests[i].active && Quests[i].visible)
    {
      // we can see it. add to the counter
      Quests[i].id = qCount;
      qCount++;
      Serial.println("active&visible");
    }
    else
    {
      Quests[i].id = 255;
      Serial.println(Quests[i].active);
      Serial.println(Quests[i].visible);
    }
  }
  Serial.print("qCount: ");
  Serial.println(qCount);
  if (qCount == 0) // we want to tell it not to do anything if there are no active visible quests.
    return false;  // that way we're not locked into the submenu without any way of getting out.

  /*
   * Now we need to set the cursor and hte base to where the knob is
   */
  //*if (qCount < 10)
  if (qCount < 5)
  {
    qBase = 0;
    qCursor = qCount-1;
  }
  else
  {
    //*qCursor = 9;
    //*qBase = qCount - 10;
    qCursor = 4;
    qBase = qCount - 5;
  }
  return true;
}



void printQuestList()
{
  byte counter = 0;
  /*
  Serial.println("-----------");
  Serial.println("Printing quests");
  Serial.print("qBase: ");
  Serial.println(qBase);
  Serial.print("qCount: ");
  Serial.println(qCount);
  */
  //*for (int i = qBase; i < qBase + 10; i++)
  for (int i = qBase; i < qBase + 5; i++)
  {
    //Serial.print("i: ");
    //Serial.println(i);
    if (i >= qCount)
    {
      //Serial.println("Breaking");
      break;
    }

    for (int x=0; x < QUESTCOUNT; x++)
    {
      /*Serial.println("---");
      Serial.print(QuestTitles[Quests[x].title]);
      Serial.print(": ");
      Serial.println(Quests[x].id);
      */
      if (Quests[x].id == i)
      {
        //Serial.println("printing title");
        tft.setCursor(1, 15+(counter*10));
        tft.print('-');
        tft.print(QuestTitles[Quests[x].title]);
        counter++;        
        break;
      }
    }
  }
  // now we have to draw the selectionBox
}



bool updateQuestList()
{
  /*
   * so the knob has changed. we need to check whether we just move the selectionbox or redraw the list.
   */
  Serial.println(knob.getPos());
  Serial.println(qBase);
  if (knob.getPos() > qBase)
  {
    qCursor = knob.getPos() - qBase;
    //*if (qCursor >= 10)
    if (qCursor >= 10)
    {
      // it's gone off the bottom of the screen
      //*qCursor = 9;
      //*qBase = knob.getPos() - 10;
      qCursor = 4;
      qBase = knob.getPos() - 5;
    }
    // otherwise we're happy to just leave it as is
  }
  else if (knob.getPos() < qBase)
  {
    // this is easy. set the base as the knob and spin from there.
    qCursor = 0;
    qBase = knob.getPos();
  }
  else
  {
    qCursor = knob.getPos();
    return false; // it moved but not enough to justify redrawing the list
  } 
  return true;  // since it did move enough to do something, redraw
}


void runQuests()
{

  if (State == STARTING)
  {
    setMenuState();
    changeTitle(S_QUESTS);
    statusInfo.Update = true;

    compileQuestList();
    printQuestList();

    selectionIcon.sbw = 84;
  }

  else if (State == MENUSTATE)
  {
    if (b1.isPressed() == 1)
    {
      /*
       * if button pressed
       *  reprint list
       *  put selection box on quests
       *  if pressed
       *    print message
       *    
        */
      printQuestList();
      knob.setRange(qCount);
      qCursor = knob.getPos();
      State = INMODE;
      selectionIcon.y = 14 + (qCursor * 10);
      drawSelectionBox(selectionIcon);
    }
  }

  else if (State == INMODE)
  {
    if (knob.hasChanged())
    {
      // clear the selection box.
      clearSelectionBox(selectionIcon);
      // check if we have to reprint the list.
      if (updateQuestList())
        printQuestList();
      selectionIcon.y = 14 + (qCursor * 10);
      drawSelectionBox(selectionIcon);
    }
  if (b1.isPressed() == 1)
    {
      Serial.println("--questSelected--");
      Serial.println(knob.getPos());
      printPopupMsg(&QuestDescriptions[Quests[knob.getPos()].descriptions[Quests[knob.getPos()].stage]]);
    }      
  }    
}


// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------
void runStats()
{
  if (State == STARTING)
  {

    
    //tft.setCursor(10, 20);
    //tft.print("Feature not enabled");
    setMenuState();
    changeTitle(S_STATUS);
    statusInfo.Update = true;

    compileQuestList();
    
    tft.setCursor(10, 40);
    tft.print("Quests");
    tft.setCursor(10, 50);
    tft.print(qCount);
    
    tft.setCursor(120, 40);
    tft.print("Level");
    tft.setCursor(120,50);
    tft.print(statusInfo.level);
    
    tft.setCursor(120, 60);
    tft.print("XP");
    tft.setCursor(120,70);
    tft.print(statusInfo.xp);
    
    tft.drawBitmap(48, 32, g_mainmenu, 64, 72, fgColour);
  }
}


// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------

void runSettings()
{
  if (State == STARTING)
  {
    setMenuState();
    changeTitle(S_SETTINGS);

    // clear edit area. 
    tft.fillRect(76,10,152,109,bgColour);
    // TODO: draw the settings icon
    tft.drawBitmap(87, 25, g_settings, 56, 66, fgColour);
    tft.drawLine(75,12, 75, 113, fgColour);
    
    // display selection options
    for (byte i=0; i < 3; i++)
    {
      tft.setCursor(settingIcons[i].x+settingIcons[i].offsetX, settingIcons[i].y+settingIcons[i].offsetY);
      tft.print(localisation[settingIcons[i].title]);
    }
  }
  
  else if (State == MENUSTATE)
  {
    if (b1.isPressed() == 1)
    {
      // we've entered the selection window.  change the state to stop menu choosing and set the knob.
      knob.setRange(3);
      lastSelected = knob.getPos();
      drawSelectionBox(settingIcons[lastSelected]);
      State = INMODE;
    }
  }
  
  else if (State == INMODE)
  {
    // check the knob
    // if hasChanged()
    //  clear selection box
    //  lastSelected
    //  draw selection box
    if (knob.hasChanged())
    {
      statusInfo.buzzerDelay = 1;
      clearSelectionBox(settingIcons[lastSelected]);
      lastSelected = knob.getPos();
      drawSelectionBox(settingIcons[lastSelected]);
    }
    
    else if (b1.isPressed()==1)
    {
      //Serial.println("b1 pressed");
      Mode = settingIcons[lastSelected].Mode;
      // I'm not using changeMode() because I don't want to undraw the settings window
      State = STARTING;
    }
  }


}


// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------
void runFGColour()
{
  // clear the draw box area
  // draw the selections
  // draw the selection box
  if (State == STARTING)
  {
    State = RUNNING;
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

    //Serial.println("runFGColour starting");
    drawSelectionBox(colourIcons[lastSelected]);
  }
  
  // check the knob move
  // if moved
  //  clear selection box
  //  draw the selection box
  else if (State == RUNNING)
  {
    if (knob.hasChanged())
    {
      statusInfo.buzzerDelay = 1;
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
  if (State == STARTING)
  {
    State = RUNNING;
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
  else if (State == RUNNING)
  {
    if (knob.hasChanged())
    {
      statusInfo.buzzerDelay = 1;
      clearSelectionBox(colourIcons[lastSelected]);
      lastSelected = knob.getPos();
      //Serial.println("runBGColour running");
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

void runAdminPW()
{
  tft.setCursor(80, 100);
  tft.print(ADMIN_PW);
  setMenuState();
  Mode = SETTINGS;
}
// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------
void runMaps()
{
  if (State == STARTING)
  {
    setMenuState();
    changeTitle(S_MAPS);
    statusInfo.Update = true;

    // display the current map
    // print the list of maps
    drawMap();
    for (byte i=0; i < 4; i++)
    {
      tft.setCursor(mapIcons[i].x+mapIcons[i].offsetX, mapIcons[i].y+mapIcons[i].offsetY);
      tft.print(localisation[mapIcons[i].title]);      
    }
  }

  else if (State == MENUSTATE)
  {
    if (b1.isPressed() == 1)
    {
      // we've entered the selection window.  change the state to stop menu choosing and set the knob.
      knob.setRange(4);
      lastSelected = knob.getPos();
      drawSelectionBox(mapIcons[lastSelected]);
      State = INMODE;
    }
  }
  
  else if (State == INMODE)
  {
    if (knob.hasChanged())
    {
      clearSelectionBox(mapIcons[lastSelected]);
      lastSelected = knob.getPos();
      drawSelectionBox(mapIcons[lastSelected]);
    }
    if (b1.isPressed() == 1)
    {
      mapSelected = mapIcons[knob.getPos()].Mode;
      tft.fillRect(35,12,120,100,bgColour);
      drawMap();
      setMenuState();
    }
  }
}




void drawMap()
{
  if (mapSelected == 0)
  {
    tft.drawBitmap(35, 12, m_vhall, 120, 100, fgColour);
  }
  if (mapSelected == 1)
  {
    tft.drawBitmap(35, 12, m_100n, 120, 100, fgColour);
  }
  if (mapSelected == 2)
  {
    tft.drawBitmap(35, 12, m_100w, 120, 100, fgColour);
  }
  if (mapSelected == 3)
  {
    tft.drawBitmap(35, 12, m_100s, 120, 100, fgColour);
  }
  
}


// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------
void runRadio()
{
  if (State == STARTING)
  {
    //tft.setCursor(10, 20);
    //tft.print("Feature not enabled");
    setMenuState();
    changeTitle(S_RADIO);

    statusInfo.Update = true;
    tft.setCursor(10, 40);
    tft.print("Radio Coming Soon");
    tft.setCursor(10, 50);
    tft.print("(After everything else)");
    
  }
}




// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------



void handleWebMsg()
{
  
}


void serveHomepage()
{
  if (webServer.hasArg("passkey"))
  {
    if (webServer.arg("passkey") == ADMIN_PW)
    {
      // Authenticated.  Check if they're posting a msg or not
      if (webServer.hasArg("message"))
      {  // they've POSTed a msg to send. handle it
        handleMsg();
      }
      webServer.send(200, "text/html", HTMLHead + "messenger" + HTMLMsgBody1 + "<option value='woodnet'>Woodnet</option>" + HTMLMsgBody2 + "\""+ADMIN_PW+"\""+HTMLMsgBody3);    }
    else
    {
      webServer.send(200, "text/html", HTMLHead + "login" + HTMLLoginBody1);
    }
  }  
  else
  {
    webServer.send(200, "text/html", HTMLHead + "login" + HTMLLoginBody1);
  }
  
}



void handleMsg()
{
}  



// ---------------------------------------------------
// ---------------------------------------------------
// ---------------------------------------------------


void backgroundUpdate()
{
  dnsServer.processNextRequest();
  webServer.handleClient();
  delay(10); // a delay of 1ms will allow the wifi AP half to execute itself.
  checkRFID();
}


void checkRFID()
{
  byte key[4];
  bool scanned = false;
  if (rfid.PICC_IsNewCardPresent())
  {
    if (rfid.PICC_ReadCardSerial())
    {
      scanned = true;
      for (int i=0; i < 4; i++)
      {
        key[i] = rfid.uid.uidByte[i];
        Serial.print(key[i]);
        Serial.print(' ');
      }
      Serial.println("");
      rfid.PICC_HaltA();
    }
  }
  if (!scanned)
  {
    //Serial.println("returning");    
    return;
  }
    
  // now check against active quests
  Quest *tempQuest;
  //Serial.println("=====================================");
  //Serial.println("checking if key matches any quests");
  for (int i=0; i<QUESTCOUNT; i++)
  {
    tempQuest = &Quests[i];
    //Serial.print("Active?:");
    //Serial.println(tempQuest->active);
    if (tempQuest->active)
    {
      /*
      Serial.println("------");
      Serial.println("Active quest:");
      Serial.println(QuestTitles[tempQuest->title]);
      Serial.println(tempQuest->getStageKeyIndex());
      
      Serial.print(StageKeys[tempQuest.getStageKeyIndex()][0]);
      Serial.print(" ");
      Serial.print(StageKeys[tempQuest.getStageKeyIndex()][1]);
      Serial.print(" ");
      Serial.print(StageKeys[tempQuest.getStageKeyIndex()][2]);
      Serial.print(" ");
      Serial.print(StageKeys[tempQuest.getStageKeyIndex()][3]);
      Serial.println(" ");
      Serial.println("-----------");
      Serial.println(key[0]);
      Serial.println(StageKeys[tempQuest.getStageKeyIndex()][0]);
      */
      if (checkKey(key, StageKeys[tempQuest->getStageKeyIndex()]))
      {
        // it's a match.  advance the quest, do the popup window
        if (tempQuest->nextStage())  // It'll return true if it completes
        {
          statusInfo.xp += QuestXPAmounts[tempQuest->maxStage];
          checkLevel();
        }
          
        printPopupMsg(&QuestDescriptions[tempQuest->getDescIndex()]);
        break;
      }
      //else
      //  Serial.println("does not match this quest");
    }
  }
}

void checkLevel()
{
  // we cycle through 100 iterations. if it is still more than that we've maxed out
  for (int i=1; i<101; i++)
  {
    if (i*1500 > statusInfo.xp)
    {
      // it's more. found the level.
      statusInfo.level = i;
      return;
    }
  }
  // if we get here we've maxed out
  statusInfo.level = 100;
}

bool checkKey(byte a[], byte b[])
{
  bool match;
  if (a[0] != NULL)
  {
    match = true;
  }
  for (byte k=0; k<4; k++)
  {
    if (a[k] != b[k])
    {
      match = false;
      break;
    }
  }
  return match;
}


void setMenuState()
{
  State = MENUSTATE;
  knob.setRange(6);
  lastSelected = knob.getPos();
}








void changeTitle(byte string)
{
  localisation[string].toCharArray(statusInfo.title, TITLE_BUFFER_SIZE);
  for (int i = localisation[string].length(); i < TITLE_BUFFER_SIZE - 1; i++)
  {
    statusInfo.title[i] = ' ';
  }
  statusInfo.Update = true;
}






void changeMode(byte newMode)
{
  
  if (Mode == TRACKER) // this is a bad place to have this. we need to objectize the modes! TODO!
  {
    targetting = false;
    WiFi.disconnect();
  }
  // setting the default for the menustate just in case we need it
  knob.setRange(6);
  lastSelected = modePos;
  
  
  tft.fillScreen(bgColour);
  Mode = newMode;
  State = STARTING;
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
    Serial.println("updating status bar");
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

      //tft.print(" ");
      for (int i=0; i < 6; i++)
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
        tft.print(" ");
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















/*
Quest Quests[] = 
{
  Quest(),Quest(),Quest()
};
*/




void buildQuests()
{
  /*
   * build out all the quests manually.  We'll need to write a program to let 
   * others help us program them later.  Maybe a google spreadsheet
   */
  Quests[0].stageKeys[0]=0;
  Quests[0].descriptions[0] = 7;
  Quests[0].descriptions[1] = 2;
  Quests[0].title = 4;
  Quests[0].visible = true;
  
  Quests[1].stageKeys[0] = 1;
  Quests[1].descriptions[0] = 1;
  Quests[1].descriptions[1] = 3;
  Quests[1].visible = true;
  Quests[1].title = 2;

  Quests[2].stageKeys[0] = 2;
  Quests[2].stageKeys[1] = 3;
  Quests[2].stageKeys[2] = 4;
  Quests[2].descriptions[0] = 0;
  Quests[2].descriptions[1] = 4;
  Quests[2].descriptions[2] = 5;
  Quests[2].descriptions[3] = 2;
  Quests[2].maxStage = 3;
  Quests[2].visible = false;
  Quests[2].title = 3;

  Quests[3].stageKeys[0]=6;
  Quests[3].descriptions[0] = 0;
  Quests[3].descriptions[1] = 6;
  Quests[3].title = 5;
  //Quests[3].visible = false;

  Quests[4].stageKeys[0]=7;
  Quests[4].descriptions[0] = 0;
  Quests[4].descriptions[1] = 6;
  Quests[4].title = 5;
  //Quests[4].visible = false;

  Quests[5].stageKeys[0]=8;
  Quests[5].descriptions[0] = 0;
  Quests[5].descriptions[1] = 6;
  Quests[5].title = 5;
  //Quests[5].visible = false;

  Quests[6].stageKeys[0]=9;
  Quests[6].descriptions[0] = 0;
  Quests[6].descriptions[1] = 6;
  Quests[6].title = 5;
  //Quests[6].visible = false;

  Quests[7].stageKeys[0]=10;
  Quests[7].descriptions[0] = 0;
  Quests[7].descriptions[1] = 6;
  Quests[7].title = 5;
  //Quests[7].visible = false;

  Quests[8].stageKeys[0]=11;
  Quests[8].descriptions[0] = 0;
  Quests[8].descriptions[1] = 6;
  Quests[8].title = 5;
  //Quests[8].visible = false;

  


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
  menuIcons[1].Mode = QUESTS;
  menuIcons[1].title = S_QUESTS;
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
  settingIcons[2].title = S_SETTINGS3;
  settingIcons[2].Mode = SHOWADMINPW;


  mapIcons[0].x = 0;
  mapIcons[0].y = 15;
  mapIcons[0].w = 32;
  mapIcons[0].h = 10;
  mapIcons[0].offsetX=1;
  mapIcons[0].offsetY=1;
  mapIcons[0].sbw = 32;
  mapIcons[0].sbh = 10;
  mapIcons[0].title = S_VHALL;
  mapIcons[0].Mode = 0;
  mapIcons[1].x = 0;
  mapIcons[1].y = 25;
  mapIcons[1].w = 32;
  mapIcons[1].h = 10;
  mapIcons[1].offsetX=1;
  mapIcons[1].offsetY=1;
  mapIcons[1].sbw = 32;
  mapIcons[1].sbh = 10;
  mapIcons[1].title = S_100N;
  mapIcons[1].Mode = 1;
  mapIcons[2].x = 0;
  mapIcons[2].y = 35;
  mapIcons[2].w = 32;
  mapIcons[2].h = 10;
  mapIcons[2].offsetX=1;
  mapIcons[2].offsetY=1;
  mapIcons[2].sbw = 32;
  mapIcons[2].sbh = 10;
  mapIcons[2].title = S_100W;
  mapIcons[2].Mode = 2;
  mapIcons[3].x = 0;
  mapIcons[3].y = 35;
  mapIcons[3].w = 32;
  mapIcons[3].h = 10;
  mapIcons[3].offsetX=1;
  mapIcons[3].offsetY=1;
  mapIcons[3].sbw = 32;
  mapIcons[3].sbh = 10;
  mapIcons[3].title = S_100S;
  mapIcons[3].Mode = 3;


  
  
  /*
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
  settingIcons[4].x = 0;
  settingIcons[4].y = 55;
  settingIcons[4].w = 62;
  settingIcons[4].h = 10;
  settingIcons[4].offsetX=1;
  settingIcons[4].offsetY=1;
  settingIcons[4].sbw = 62;
  settingIcons[4].sbh = 10;
  settingIcons[4].title = S_SETTINGS4;
  settingIcons[4].Mode = SETBUZZER;
*/

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


