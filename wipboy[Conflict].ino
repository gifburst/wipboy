//import

// webserver
#include <ESP8266WiFi.h>
#include "./DNSServer.h"                  // Patched lib
// graphics
#include <ESP8266WebServer.h>
#include <Adafruit_GFX.h>       // Core graphics library
#include "./Adafruit_ST7735.h"  // Hardware-specific library - patched for esp
#include <SPI.h>
// physical controls (because the regular arduino stuff sucks)
#include "./Slider.h"
#include "./Button.h"

/*
 * Available pins:
 * 0, 4, 5, 15, 16
 * 
 * 
 * ESP8266-12        HY-1.8 SPI
 * REST                           Pin 06 (RESET)
 * GPIO2                          Pin 07 (A0)
 * GPIO13 (HSPID)                 Pin 08 (SDA)
 * GPIO14 (HSPICLK)               Pin 09 (SCK)
 * GND (formerly GPIO15)(HSPICS)  Pin 10 (CS)
 * 
 * GPIO ADC          Control knob
 * GPIO0             Button1
 * GPIO04            Button2
 * GPIO16            Button3
 * 
 * 
  */
