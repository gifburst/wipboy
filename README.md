# wipboy

www.wipboy.com

My own Pip-Boy clone using the ESP8266, an ST7735, some buttons and a knob. Note: this is still very WIP (Hence the name).  Much of it is still unfinished.

Planned features:

  1) CHEAP!  I want this to be less than $10 a device.  Why? Because of its second primary purpose.  
  
  2) Wifi beacon and tracking capabilities. We're going to try and run another game of "hunter/killer" at comicon, amongst a crowd of tens of thousands.  Building these should make it both cooler and easier than getting everyone's cell phones to work and using WN702s
  
  3) WiFi communications.  Comicon kills cell service, but wifi seems to cut through the massive crowds across the entire convention centre.  As such, I'm trying to sync the cell phone over wifi to the device, which hosts an input webpage. The user then types a msg, chooses a target, and the wristband will relay it to the target's wristband via wifi.
  
  4) Same general feel as a Fallout Pipboy.  Cause if I'm building a retro-wristmounted computer technology with a monochrome colour, I might as well shoot for the best.
  
  5) clean code.  Nowhere near yet!
  
  
How am I accomplishing this?
  1) Cheapest parts you can get from china.  Here's the part breakdown so far:
  
    ESP8266 (ESP-12e to be specific): $2.33
    
    1.8" TFT (ST7735):                $3.84
    
    knobs/buttons:                    $ .60
    
    850mAh Lipo battery               $3.00
    
    charging circuit/5-3.3v converter $ .38
    
  So far just over $10.  Still working on locating cheaper battery sources with more mAh for less cost.  I'll be 3d-modeling the cases and making them out of my 3d printer, so that cost isn't included either (don't know how much it'll take yet).  In theory you could make a case out of anything, from clay to foam to an altoids tin.


Current status:
  1) (see above)
  
  2) 80% complete - I have the tracking working (it gives you a rough range based off signal strength). I need to do some real-world testing to convert it to a range guestimate
  
  3) 10% complete - I have a prototype DNS server working which autodirects to a single page on the device.  non-iphone cell phones work np (iphones disconnect because they can't see apple.com and thus freak out.  Those phones really want to subliminal-sell you more apple products!).  Now I have to add a modicum of security, take inputs from phones through their browsers, then write the client to connect/forward/disconnect from one ESP to another
  
  4) 50% complete - the current prototype (this code) has GUI nothing like. Because of a single analogue port I settled on having only one knob, so started with a design philosophy of a main menu.  As I've built-out the features I've realised I can have a selector button inside each function to activate its features, and probably revert the knob back to controlling the GUI in the main mode.
  
    The menu approach though did point the way to functionality.  The knob and buttons are fully context-sensitive, meaning they do different things based on what you're doing.  Videos (and a webpage) to follow.
  
    Graphics are all monochrome.  To simplify things I convert a 1-bit (2 colour) png into a byte-array format I can store as text in the program.  I then copy/paste it into the code and use adafruit's GFX drawBitmap() command to draw them. Simple, low memory, works great.  Once I clean it up I'll post the script for converting .pngs as well.
  
    System is GUI though.  You select things by changing the knob, and it draws selection boxes around different sized and placed graphics. 
  
  5) Code is a horrible mess atm.  I didn't realise just how ridiculously huge the codebase would grow, so started it mostly in one file.  Now it's MASSIVE I need to go back and break all this into different libraries.
  
  
"WHY AREN"T YOU CLONING THE PIP-BOY WITH STATS AND A MAP AND VATS AND .....?!?!"

Mostly because most of the features of the pip-boy are fictional, and not implementable in real life.  Not to mention I'm building this as a tool for the hunter/killer game we're playing, not to cosplay as a fallout vault dweller.  That's just a fun aside ;)  That said, I do want to look at integrating other devices (maybe some IR lazertag so we can truly hunt one another, and wifi beacons that give you points for discovering "places" that I can build and scatter around the convention centre for a few $ each, or ones that create RAD-zones that cost you points).  Once I get the GUI more libraried-out then anyone can go back and make a better clone.
  
  
Notes if you download this: 
  Firstly, you probably shouldn't.  I work in multiple locations so I'm using GIThub to keep my codebase synchronised.  It's certainly not production quality yet.
  
  Second, the codebase is a terrible one-file mess.  Until I clean and comment it, things may not make sense.  I'll have a webpage with a forum up within a few days (hopefully) where questions can be asked of how I do things instead
  
  Third, if you want to download this, the master will always be compileable.  I commented in what connects to what if you want to build the circuit yourself (screen and buttons/knob. not too hard).  Note I'm going to be changing the amount of buttons and adding other features, so build on breadboards because this is due to change before final production!
  

Plans for the future?
  
  -Build 5-6 of these, give to friends along with a "touch-football" style bandoleer of flags to pull. I think that's the american game equivalent.
  
  If I can hammer down a lot of supply and manufacture details, maybe a kickstarter.  all the circuitry could be mounted to a PCB "backboard" sandwiched between the screen and the circuits. Cases would cost a mint to make the mould, so I would probably sell the 3d models on shapeways instead, and just put up the circuits, code, and development work up for KS instead as a kit.  I'll be opening up a forum soon to seek council from others online on how to do all the production stuff. I'm an open-source nut by nature, so having a free DIY alternative is fine with me.
  
    
  
