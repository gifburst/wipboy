#ifndef constants_h
#define constants_h

#include <Arduino.h>

#define ADMIN_PW "12345"
char WIFI_PW[] = "signpost";

#define QUESTCOUNT 9
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
  "Quests",
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
  "Target:",
  "Signal Str",
  "Select Node",
  "Sound on/off",
  "Sound Disabled",
  "Sound Enabled",
  "Radio",
  "Maps",
  "vhall",
  "100n",
  "100s",
  "100w"
};

// Yes, I'm aware defines are not constants.  I'll convert them over later.  We still have 26k of RAM atm, so we're not suffering yet

#define S_MENU 0
#define S_TRACKER 1
#define S_QUESTS 2
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
#define S_SETTINGS4 16
#define S_SOUNDDISABLED 17
#define S_SOUNDENABLED 18
#define S_RADIO 19
#define S_MAPS 20
#define S_VHALL 21
#define S_100N 22
#define S_100S 23
#define S_100W 24

// ------------------------
// icons, menu items, modes

// We use these constants to define how big the
// buffers are for the title bar and the button
// labels.  This is useful later when we have to
// copy strings into them.  It's an imperfect
// system though, due for revision
#define TITLE_BUFFER_SIZE 13
#define LABELS_BUFFER_SIZE 30



// ------------------------
// Modes and States
// The "mode" is the program that's running.  Like the
// tracker or messenger.  The "state" is what state the
// mode is in, from starting to running to needing update
// to shutting down.

//States
#define STOPPED 0
#define STARTING 1
#define RUNNING 2
#define UPDATE 3
#define SHUTDOWN 4
#define MENUSTATE 5
#define INMODE 6
#define POPUPSTATE 7

//Modes
#define MAINMENU 102
#define TRACKER 103
#define QUESTS 104
#define SETTINGS 105
#define STATUS 106
#define RADIO 107
#define ADJUSTFGCOLOUR 108
#define ADJUSTBGCOLOUR 109
#define SHOWADMINPW  110
#define NODECHOOSER 111
#define SETBUZZER 112
#define QUESTNODECHOOSER 113
#define RUNPOPUP 114
#define MAPS 115

byte ModesRef[] =
{
 STATUS,
 TRACKER,
 QUESTS,
 SETTINGS,
 MAPS,
 RADIO,
};

String ModesStrings[] = 
{
  "STATS",
  "TGT",
  "QST",
  "CFG",  
  "MAP",
  "RDO", 
};
// |-------------------------|
// STATS - Trkr - Quests - CFG   
// |STATS|Trkr|Qsts|CFG|Radio|
// STATS Trkr Qsts CFG Rdo Map                                                                  
// -----------------------------
// list of colours

// this is so the user can choose their foreground and background colours
static uint16_t colours[] = {
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



String QuestTitles[] = 
{
  "",
  "Default Title",
  "Introduction",
  "Our 1st Steps",
  "Card Thief",
  "Easter Egg"  
};
String QuestDescriptions[] = 
{
  "",
  "Welcome! This~introductory quest~is completed by~swiping the right card",
  "Quest completed",
  "Swipecard 1 discovered!",
  "Quest unlocked.~Swipe card 3 to~continue",
  "Swipe card 4 to~continue",
  "Secret Tag~Discovered",
  "Steal card 5 and~swipe to complete"
};

byte StageKeys[][4]
{
  {35,21,96,0},
  {170,39,62,213},
  {215,140,58,252},
  {147,35,108,0},
  {51,105,104,0},
  {217,117,58,69},
  {246,92,251,17},
  {117,42,172,79},
  {69,61,173,79},
  {70,179,113,35},
  {181,135,169,79},
  {150,209,250,17}
};


int QuestXPAmounts[] = 
{
  0,
  1000,
  2500,
  4500,
  7000,
};



#endif
