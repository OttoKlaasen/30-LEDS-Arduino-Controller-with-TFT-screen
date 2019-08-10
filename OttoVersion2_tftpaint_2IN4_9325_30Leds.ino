
// LED strip controller made by Otto Klaasen to be used as an interface for RGB LED strip.
// Version 2 where we use portrait mode to obtain a maximum of buttons in vertical direction.

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <FastLED.h>         // library for the RGB strip
#include <Wire.h>
//#include <LM75.h>
// sensor;  // initialize an LM75 object (temp sensor)

// -TFT screen uses the A0 ~ A3 pins, D4 ~ D13 pins and I2C interface, D0/D1/D2/D3 can be used for other products, we use pin2 to control the RGB strip.

#define LED_PIN     2
const int FAN = 3; 
#define NUM_LEDS    30
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

const int MIC = A5; // Microphone input, not possible , already used by he LM75
int MICValue = 0;
int MICValueRead = 0;
int NoiseLevel = 512;
int SoundSample = 1;
int AvgMICValueRead = 0;
int PeakMICValueRead = 0;

#define UPDATES_PER_SECOND 100

// #############################################################################################################################


#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

#ifndef USE_ADAFRUIT_SHIELD_PINOUT 
 #error "This sketch is intended for use with the TFT LCD Shield. Make sure that USE_ADAFRUIT_SHIELD_PINOUT is #defined in the Adafruit_TFTLCD.h library file."
#endif

// These are the pins for the shield!
#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 6   // can be a digital pin


#define TS_MINX  166
#define TS_MINY  167
#define TS_MAXX  893
#define TS_MAXY  927

#define box 38  // This parameter define the number of boxes in the color strip
// Array of colors for color strip bar
unsigned int ColorArray[box] = {0xF820,0xF8AA,0xF9A0,0xFAE0,0xFC40,0xFD60,0xFE20,0xFEE0,0xFFE0,0xE7E0,0xC7E0,0xA7E0,0x87E0,0x5FE0,0x3FE0,0x27E0,0x07E0,0x07E6,0x07E9,0x07F0,0x07F9,0x07FC,0x079F,0x053F,0x045F,0x029F,0x00DF,0x181F,0x601F,0x901F,0xD01F,0xF01F,0xF81C,0xF817,0xF812,0xF80A,0xF805,0xF800};

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 330 ohms across the X plate

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);


// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define BLACKBUTTON 0x8C12  // For the script menu we use a dark color button and WHITE text

#define WHITEtest 0x00ff00 // 24 bit code for WHITE

// Text for buttons
#define ARRAYSIZE 8  // 8 textbuttons used
String ButtonText[ARRAYSIZE] = { "OFF", "Scanner", "Cylon","PACMAN", "Police", "Theatre","Fire", "VU meter" };

Adafruit_TFTLCD tft;

#define SMALLBOX   40  // Size of small box
#define LARGEBOX  120  // Size of large box
#define radius 23      // Radius of buttons
#define BLACKOFFSET 5 // Add to BLACK to make button visisble

// Pressure parameter for the TFT screen
#define MINPRESSURE 1
#define MAXPRESSURE 1000

// Define offset in the button for text
#define XTxtOffset   19
#define YTxtOffset   14


unsigned int oldcolor, currentcolor;
int OldRGBscript ,currentRGBscript ;  // currentRGBscript will hold a value indicating which RGB script is running for the LED strip.
int MAX_WIDTH;     // MAX width of the LCD screen
int MAX_HEIGHT;    // MAX height of the LCD screen
bool CallLed;      // receive return value from LedController function
long BitColorTest;
int BitColor_16bit;
long BitColor_24bit;
int MyLed;         // The led selected
int Forward;       // Move forward
// Push the LED forward over the bar for Cylon mode
int xs;  // Scan start
int ys;  // Scan offset
int  TempSensor;  // Temperature of the on board TFT temp sensor.


// Detection for color strip
int ArrayIndex; // Used to find the selected color in the array via the screen
unsigned int StripColor; // The color selected from the strip.

// Indicators which button area has been selected.
bool SelectColorButton, SelectColorBar,SelectScriptBar;

void setup(void) {
  pinMode(FAN, OUTPUT);
  Serial.begin(57600);
  //Wire.begin(); // Setup for temp sensor.

  tft.reset();

  uint16_t identifier = tft.readID();

  if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
     // OK: we have this drive:  Found ILI9325 LCD driver
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else if(identifier == 0x4747) {
    Serial.println(F("Found HX8347A LCD driver"));
  } else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Adafruit 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_ADAFRUIT_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Adafruit_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    return;
  }
  tft.begin(identifier);
  
  // The rotation parameter can be 0, 1, 2 or 3. For displays that are part of an Arduino shield, rotation value 0 sets the display to a portrait (tall) mode, 
  // with the USB jack at the top right. Rotation value 2 is also a portrait mode, with the USB jack at the bottom left. 
  //Rotation 1 is landscape (wide) mode, with the USB jack at the bottom right, while rotation 3 is also landscape, but with the USB jack at the top left.
  tft.setRotation(2);
  tft.fillScreen(BLACK);

  Serial.print("Screen Width: "); Serial.println (tft.width());
  Serial.print("Screen Height: "); Serial.println (tft.height());

// Please note that these values depend on the SetRotation, in this code we use portrait mode with the USB connection at bottom.
  MAX_WIDTH = tft.width();  // 240 pixels in my display.
  MAX_HEIGHT = tft.height(); // 340 pixels in my display.

  
// Syntax tft.fillRect = screen.rect(xStart, yStart, width, height);
// Draws a rectangle to the TFT screen. rect() takes 4 arguments, the first two are the top left corner of the shape, the last two are the width and height of the shape.
// Setup RGB script buttons with text in each button.

// Splash screen
// #############################################################################################################################
    tft.setCursor(0,0);
    tft.setTextColor(MAGENTA);  tft.setTextSize(2);
    tft.println("Version 1.0 RGB-V");
    tft.println("30 LED Version");
    tft.println("By Otto M. Klaasen");
    delay (3000);  // Wait 2 seconds
 // Do RGB colors
    tft.fillScreen(RED);
    delay(20);
    tft.fillScreen(GREEN);
    delay(20);
    tft.fillScreen(BLUE);
    delay(20);
    tft.fillScreen(BLACK);
    delay(5);
// Rounded button command syntax
// void drawRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color); 
// void fillRoundRect(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h, uint16_t radius, uint16_t color);

// #############################################################################################################################
// Setup of the color button at the left side of the screen
  tft.fillRoundRect(0,0, SMALLBOX, SMALLBOX, radius, WHITE);
  tft.fillRoundRect(0,SMALLBOX, SMALLBOX, SMALLBOX, radius, RED);
  tft.fillRoundRect(0,SMALLBOX*2, SMALLBOX, SMALLBOX, radius, YELLOW);
  tft.fillRoundRect(0,SMALLBOX*3, SMALLBOX, SMALLBOX, radius, GREEN);
  tft.fillRoundRect(0,SMALLBOX*4, SMALLBOX, SMALLBOX, radius, CYAN);   
  tft.fillRoundRect(0,SMALLBOX*5, SMALLBOX, SMALLBOX, radius, BLUE);
  tft.fillRoundRect(0,SMALLBOX*6, SMALLBOX, SMALLBOX, radius, MAGENTA);
  tft.fillRoundRect(0,SMALLBOX*7, SMALLBOX, SMALLBOX, radius, BLACK+BLACKOFFSET);
// Here we draw the selector around the button
  currentcolor = WHITE;
  if (currentcolor == WHITE)  { tft.drawRoundRect(0, 0, SMALLBOX, SMALLBOX, radius, BLUE);tft.drawRoundRect(1, 1, SMALLBOX-2, SMALLBOX-2, radius, BLUE);
  }
  else  tft.drawRoundRect(0, 0, SMALLBOX, SMALLBOX, radius, WHITE);

// #############################################################################################################################

// Setup of the program 8 scripts  buttons
for (int i=0; i <= 7; i++){ tft.fillRoundRect (MAX_WIDTH - LARGEBOX,SMALLBOX*i ,LARGEBOX, SMALLBOX, radius,BLACKBUTTON);
tft.setCursor( MAX_WIDTH - LARGEBOX +XTxtOffset,SMALLBOX*i + YTxtOffset );
tft.setTextColor(WHITE);  tft.setTextSize(2);
tft.print(ButtonText[i]);  // Print the array of text on the buttons
}
// Here we draw the selector around the script buttons
// Create selection rectangle
tft.drawRoundRect(MAX_WIDTH - LARGEBOX, 0, LARGEBOX, SMALLBOX, radius, WHITE);

// #############################################################################################################################

// Make color bar.
for (int i=0; i <= box; i++){
// Use the line draw function
// Syntax void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
tft.fillRect(SMALLBOX + 5, i* MAX_HEIGHT/box , SMALLBOX,(MAX_HEIGHT/box)+1, ColorArray[i]);
}

// #############################################################################################################################

currentRGBscript = 0;
OldRGBscript = 0;
pinMode(13, OUTPUT);
MyLed = 0;
Forward = 1;
xs = (NUM_LEDS/2)-5; // Start Value
ys = (NUM_LEDS/2)+5; // Max Value

// #############################################################################################################################
// Setup of the LED strip

delay( 1000 ); // power-up safety delay
FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
FastLED.setBrightness(  BRIGHTNESS );

} // End of setup 


void loop()
{
//TempSensor = sensor.temp();
//Serial.print(sensor.temp());
//Serial.println(" C");
TempSensor = 35;
if (currentRGBscript != 7){  // When the speaker is used switch off the fan to avoid PCM noise.
  if (TempSensor > 45) analogWrite(FAN, 255); // Give a value to the PCM output, at 0 FAN will stop , max is 255.
  else if(TempSensor > 40) analogWrite(FAN, 225);
  else if(TempSensor > 35) analogWrite(FAN, 200);
  else if(TempSensor > 30) analogWrite(FAN, 175);
  else analogWrite(FAN,0);
}
else analogWrite(FAN,0);


  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint(); // Reading the screen
  digitalWrite(13, LOW);
  // Set the menu area status parameters
  SelectColorButton = 0;
  SelectColorBar = 0;
  SelectScriptBar = 0;
   

  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  //pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  //pinMode(YM, OUTPUT);

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    
            //Serial.print("X = "); Serial.print(p.x);
            //Serial.print("\tY = "); Serial.print(p.y);
            //Serial.print("\tPressure = "); Serial.println(p.z);
            
            // syntax for mapmap(value, fromLow, fromHigh, toLow, toHigh)
            // scale from 0->1023 to tft.width
            p.x = map(p.x, TS_MINX, TS_MAXX,0, tft.width());
            p.y = map(p.y, TS_MINY, TS_MAXY,0, tft.height());
            // Serial.print("("); Serial.print(p.x);
            // Serial.print(", "); Serial.print(p.y);
            // Serial.println(")");
        
            // #############################################################################################################################

            // Detection for stripcolor
          
            if (p.x > SMALLBOX and p.x < 2 * SMALLBOX) {
                  // Selection for color script
                  // Serial.println("Color strip touched");
                  SelectColorBar = 1; // Color bar selected
                     
                  // We have 38 boxes which indicated an array color
                  // So p.y /box should give the correct array number
                  ArrayIndex = ((p.y)/(MAX_HEIGHT/(box-3)));
                  // Serial.print("Array Index: ");Serial.println(ArrayIndex);
                  if (ArrayIndex < box) { 
                      // Serial.print("Array Value: ");Serial.println(ColorArray[ArrayIndex],HEX);
                      StripColor = (ColorArray[ArrayIndex]);  
                      // Serial.print("Color code: ");Serial.println(StripColor,HEX);
                      }
            }

            // #############################################################################################################################
     
            // Detection for script buttons
            
            if (p.x > MAX_WIDTH - LARGEBOX) {
                  // Selection for different RGB script detected.
                  OldRGBscript = currentRGBscript;
                  SelectScriptBar = 1;  // Scriptbar selected
            
                  if (p.y < SMALLBOX) {
                    currentRGBscript = 0;
                  } else if (p.y < SMALLBOX * 2) {
                    currentRGBscript = 1;
                  } else if (p.y < SMALLBOX * 3) {
                    currentRGBscript = 2; 
                  } else if (p.y < SMALLBOX * 4) {
                    currentRGBscript = 3;
                  } else if (p.y < SMALLBOX * 5) {
                    currentRGBscript = 4;
                  } else if (p.y < SMALLBOX * 6) {
                    currentRGBscript = 5;
                  } else if (p.y < SMALLBOX * 7) {
                    currentRGBscript = 6;
                  } else if (p.y < SMALLBOX * 8) {
                    currentRGBscript = 7;
                  }
            }
     
            tft.drawRoundRect(MAX_WIDTH - LARGEBOX, SMALLBOX*currentRGBscript, LARGEBOX, SMALLBOX, radius, WHITE);
            
            // Rewrite old button
              if (OldRGBscript != currentRGBscript) { // Rewrite the old button is what happens here.
                  // OldRGBscript is used to rewrite the old button
                  tft.fillRoundRect (MAX_WIDTH - LARGEBOX,SMALLBOX* OldRGBscript,LARGEBOX, SMALLBOX, radius,BLACKBUTTON);
                  tft.setCursor( MAX_WIDTH - LARGEBOX +XTxtOffset,SMALLBOX*OldRGBscript + YTxtOffset );
                  tft.setTextColor(WHITE);  tft.setTextSize(2);
                  tft.print(ButtonText[OldRGBscript]);
              }
        
            // #############################################################################################################################
   
            // Detection for color buttons
                if (p.x < SMALLBOX) {
                    oldcolor = currentcolor;
                    // Serial.println("Color change");
                    SelectColorButton = 1;  //Color button selected
                    if (p.y < SMALLBOX) { 
                     currentcolor = WHITE; 
                     tft.drawRoundRect(0, 0, SMALLBOX, SMALLBOX, radius, BLUE);
                     tft.drawRoundRect(1, 1, SMALLBOX-2, SMALLBOX-2, radius, BLUE);
                   } else if (p.y < SMALLBOX*2) {
                     currentcolor = RED;
                     tft.drawRoundRect(0, SMALLBOX, SMALLBOX, SMALLBOX, radius, WHITE);
                   } else if (p.y < SMALLBOX*3) {
                     currentcolor = YELLOW;
                     tft.drawRoundRect(0, SMALLBOX*2, SMALLBOX, SMALLBOX, radius, WHITE);
                   } else if (p.y < SMALLBOX*4) {
                     currentcolor = GREEN;
                     tft.drawRoundRect(0, SMALLBOX*3, SMALLBOX, SMALLBOX, radius, WHITE);
                   } else if (p.y < SMALLBOX*5) {
                     currentcolor = CYAN;
                     tft.drawRoundRect(0, SMALLBOX*4, SMALLBOX, SMALLBOX, radius, WHITE);
                   } else if (p.y < SMALLBOX*6) {
                     currentcolor = BLUE;
                     tft.drawRoundRect(0, SMALLBOX*5, SMALLBOX, SMALLBOX, radius, WHITE);
                   } else if (p.y < SMALLBOX*7) {
                     currentcolor = MAGENTA;
                     tft.drawRoundRect(0, SMALLBOX*6, SMALLBOX, SMALLBOX, radius, WHITE);
                   } else if (p.y < SMALLBOX*8) {
                     currentcolor = BLACK;
                     tft.drawRoundRect(0, SMALLBOX*7, SMALLBOX, SMALLBOX, radius, WHITE);
                   }
   
                  if (oldcolor != currentcolor) {
                        if (oldcolor == WHITE)tft.fillRoundRect(0,0, SMALLBOX, SMALLBOX, radius, WHITE);
                        if (oldcolor == RED) tft.fillRoundRect(0,SMALLBOX, SMALLBOX, SMALLBOX, radius, RED);
                        if (oldcolor == YELLOW) tft.fillRoundRect(0,SMALLBOX*2, SMALLBOX, SMALLBOX, radius, YELLOW);
                        if (oldcolor == GREEN)tft.fillRoundRect(0,SMALLBOX*3, SMALLBOX, SMALLBOX, radius, GREEN);
                        if (oldcolor == CYAN)tft.fillRoundRect(0,SMALLBOX*4, SMALLBOX, SMALLBOX, radius, CYAN);
                        if (oldcolor == BLUE) tft.fillRoundRect(0,SMALLBOX*5, SMALLBOX, SMALLBOX, radius, BLUE);
                        if (oldcolor == MAGENTA) tft.fillRoundRect(0,SMALLBOX*6, SMALLBOX, SMALLBOX, radius, MAGENTA);
                        if (oldcolor == BLACK) tft.fillRoundRect(0,SMALLBOX*7, SMALLBOX, SMALLBOX, radius, BLACK+BLACKOFFSET);
                     }
                  }
// #############################################################################################################################
  }
// Call Led control procedure
// Serial.print("Old Color: ");Serial.println(oldcolor,HEX);
// Serial.print("Current Color: ");Serial.println(currentcolor,HEX);delay(10);

CallLed = LedController(SelectColorButton,SelectColorBar,SelectScriptBar,currentcolor,StripColor,currentRGBscript);
  
}  // End of loop

// #############################################################################################################################
// Function Area

bool Touch() {
  // This function return true or false if the touchscreen is pressed
  bool Pressed;

  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint(); // Reading the screen
  digitalWrite(13, LOW);
  
  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  //pinMode(XP, OUTPUT);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  //pinMode(YM, OUTPUT);

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!

  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    Pressed = 1;
    //Serial.println("Touch detected pressed");
 }
  else{
    Pressed = 0;
    }
return Pressed;
}

// *******************************************************************************************

int LedController(bool SelectColorButton, bool SelectColorBar,bool SelectScriptBar,unsigned int WriteColor ,unsigned int Strip_Color, int currentRGBscript ){

if (SelectColorButton == 1) {
            // Change color depending on current color
         
            for (int j=0; j < NUM_LEDS; j++) {
                if ( WriteColor == WHITE) leds[j] = rgb16convertor(WHITE);
                if ( WriteColor == RED) leds[j] = rgb16convertor(RED);
                if ( WriteColor == YELLOW) leds[j] = rgb16convertor(YELLOW);
                if ( WriteColor == GREEN)leds[j] = rgb16convertor(GREEN);
                if ( WriteColor == CYAN)leds[j] = rgb16convertor(CYAN);
                if ( WriteColor == BLUE) leds[j] = rgb16convertor(BLUE);
                if ( WriteColor == MAGENTA) leds[j] = rgb16convertor(MAGENTA);
                if ( WriteColor == BLACK) leds[j] = rgb16convertor(BLACK);
                             }
            // Activated
            FastLED.show(); 
            delay(10); 
}
if (SelectColorBar == 1) {
          // Serial.print("Color code in procedure: ");Serial.println(Strip_Color,HEX);
          for (int j=0; j < NUM_LEDS; j++) {            
            leds[j] = rgb16convertor(Strip_Color);
            }
          // Activated
          FastLED.show(); 
          delay(10);
}

          
// This section will start and stop scripts, the parameter currentRGBscript will indicate the selected or running script ( value 0 -7)
// Script 0
        if (currentRGBscript == 0 & SelectScriptBar == 1 ) {
            CleanLedStrip();   // Clean all LEDS to OFF
            MyLed = 0;
            Forward = 1;
         }
    
// Script 1 Scanner
        
        if ((currentRGBscript == 1 & SelectColorButton == 0 & SelectColorBar == 0) && WriteColor != BLACK ) {
                  
                     if (Forward == 1) {      
                                leds[MyLed] = rgb16convertor(WriteColor);
                                if (MyLed != 0) leds[MyLed-1] = rgb16convertor(BLACK);
                                FastLED.show(); 
                                if (MyLed < NUM_LEDS-1) {MyLed++;}
                                else {
                                    Forward = 0;
                                    leds[NUM_LEDS-1] = rgb16convertor(BLACK);
                                    FastLED.show(); 
                                }
                                delay(20);
                    }
                           
                    // Reverse the LED forward over the bar
                   if (Forward != 1) { 
                               leds[MyLed] = rgb16convertor(WriteColor);
                               if (MyLed < NUM_LEDS-1) leds[MyLed+1] = rgb16convertor(BLACK);
                               FastLED.show(); 
                               if (MyLed > 0) {MyLed = MyLed - 1;}
                               else {
                                    Forward = 1;
                                    leds[MyLed] = rgb16convertor(BLACK);
                                    FastLED.show(); 
                               }
                    }
                    delay(20);
       }
 
// Script2 Cylon
        
        if ((currentRGBscript == 2  & SelectColorButton == 0 & SelectColorBar == 0) && WriteColor != BLACK ) {
                      // Clean strip
                      CleanLedStrip();   // Clean all LEDS to OFF
                      if (MyLed <= xs) MyLed = xs; // Make the start point
                      if (Forward == 1) { 
                          leds[MyLed] = rgb16convertor(RED);
                          leds[MyLed+1] = rgb16convertor(RED);
                          leds[MyLed+2] = rgb16convertor(RED);
                          if (MyLed != xs) leds[MyLed-1] = rgb16convertor(BLACK);
                          FastLED.show(); 
                          if (MyLed < ys) MyLed++;
                                else {
                                    Forward = 0;
                                    FastLED.show(); 
                                }
                                delay(10);
                         }
       
                    // Reverse the LED forward over the bar

                    if (Forward == 0) { 
                           leds[MyLed] = rgb16convertor(RED);
                           leds[MyLed-1] = rgb16convertor(RED);
                           leds[MyLed-2] = rgb16convertor(RED);
                           leds[MyLed+1] = rgb16convertor(BLACK);
                           FastLED.show(); 
                           if (MyLed >xs) MyLed--;
                           else {
                                 Forward = 1;
                                 FastLED.show(); 
                                 }
                           delay(70);
                    }
                   
}

// Script 3 PACMAN
        
        if ((currentRGBscript == 3 & SelectColorButton == 0 & SelectColorBar == 0) && WriteColor != BLACK ) {
                   if (Forward == 1 or Forward ==0) {      
                          CleanLedStrip();   // Clean all LEDS to OFF
                          leds[MyLed] = rgb16convertor(GREEN);
                          if (MyLed > 2) leds[MyLed-3] = rgb16convertor(RED);                                
                          FastLED.show(); 
                          if (MyLed < NUM_LEDS-1) {MyLed++;}
                                else {
                                    Forward = 1;
                                    leds[NUM_LEDS-1] = rgb16convertor(BLACK);
                                    FastLED.show(); 
                                    leds[NUM_LEDS-4] = rgb16convertor(BLACK);
                                    leds[NUM_LEDS-3] = rgb16convertor(RED);
                                    FastLED.show(); 
                                    delay(20);
                                    leds[NUM_LEDS-3] = rgb16convertor(BLACK);
                                    leds[NUM_LEDS-2] = rgb16convertor(RED);
                                    FastLED.show(); 
                                    delay(20);
                                    leds[NUM_LEDS-2] = rgb16convertor(BLACK);
                                    leds[NUM_LEDS-1] = rgb16convertor(RED);
                                    FastLED.show(); 
                                    delay(20);
                                    leds[NUM_LEDS-1] = rgb16convertor(BLACK);
                                    FastLED.show(); 
                                    delay(20);
                                    FastLED.show(); 
                                    MyLed = 0;
                                }
                   }       
                   delay(20);                                           
       }

// Script 4 Police
        
        if ((currentRGBscript == 4  & SelectColorButton == 0 & SelectColorBar == 0) && WriteColor != BLACK ) {
                      // Clean strip
                      CleanLedStrip();   // Clean all LEDS to OFF
                      switch (Forward) {
                         case 0:
                         for (int j=0; j < NUM_LEDS; j++){
                             leds[j] = rgb16convertor(RED);
                             if (Touch()) {
                                    goto bailout;
                                }
                         }
                         break;
                         case 1:
                         for (int j=0; j < NUM_LEDS; j++) {
                           leds[j] = rgb16convertor(WHITE);
                           if (Touch()) {
                                    goto bailout;
                                }
                         }
                         break;
                         case 2:
                         for (int j=0; j < NUM_LEDS; j++) {
                          leds[j] = rgb16convertor(BLUE);
                           if (Touch()) {
                                    goto bailout;
                                }
                         }
                         break;
                         }
                        // Activated
                        FastLED.show(); 
                        delay(150); 
                        if (Forward < 2) Forward++;
                        else Forward = 0;
 
        }

// Script 5 Theatre
        
       if ((currentRGBscript == 5 & SelectColorButton == 0 & SelectColorBar == 0) && WriteColor != BLACK ) {
 
          for (int q=0; q < 3; q++) {
              if (Touch()) {
                 goto bailout;
              }
           
            for (int i=0; i < NUM_LEDS; i=i+3) {
                leds[i+q] = rgb16convertor(WriteColor); //turn every third pixel on
                  if (Touch()) {
                     goto bailout;
                  }
            }
            FastLED.show(); 

            delay(80);
           
            for (int i=0; i < NUM_LEDS; i=i+3) {
                if (Touch()) {
                   goto bailout;
              }
              leds[i+q] = rgb16convertor(BLACK); //turn every third pixel off
            }
          }
        }

// Script6 Fire
        
        if ((currentRGBscript == 6  & SelectColorButton == 0 & SelectColorBar == 0) && WriteColor != BLACK ) {
                      // Clean strip
                      CleanLedStrip();   // Clean all LEDS to OFF
                      int s = random(4); // Start random value for spark
                      int h = random(4 ,NUM_LEDS-random(10,20)); // Heat value
  
                      for (MyLed = s; MyLed < h; MyLed++){
                        if (Touch()) {
                           goto bailout;
                        }
                        if (s != 2) leds[MyLed] = rgb16convertor(RED); //Create sometimes full yellow flame
                          else leds[MyLed] = rgb16convertor(YELLOW);
                          FastLED.show();
                          if (MyLed > h/3) {
                                  int n = random(0,4);  //Sparks generator
                                  switch (n) {
                                    case 2: leds[MyLed-2] = rgb16convertor(BLACK);
                                    break;
                                    case 3: leds[MyLed] = rgb16convertor(YELLOW);
                                    break;
                                    }
                              }
                              else {
                                leds[MyLed] = rgb16convertor(RED + random(0,20));
                              }
                          FastLED.show();
                          delay(30);
                      }
       }
       // Script 7 VU Meter
        
       if ((currentRGBscript == 7 & SelectColorButton == 0 & SelectColorBar == 0) && WriteColor != BLACK ) {

            AvgMICValueRead = 0;
            PeakMICValueRead = 0;
            
            for (int j=0; j < SoundSample; j++) { 
                MICValueRead = analogRead(MIC);
                if (MICValueRead < 0 ) MICValueRead = 0;  // Filter negative spikes out
                AvgMICValueRead = AvgMICValueRead + MICValueRead;
                if (MICValueRead > PeakMICValueRead) PeakMICValueRead=MICValueRead;
               // Serial.print("MICValueRead ");
               // Serial.println(MICValueRead);
            }
            AvgMICValueRead =  AvgMICValueRead/SoundSample;

            if (PeakMICValueRead < NoiseLevel) PeakMICValueRead = NoiseLevel;
            //Serial.print("AvgMICValueRead ");
            //Serial.println(AvgMICValueRead); 
            //Serial.print("PeakMICValueRead ");
            //Serial.println(PeakMICValueRead);
            
        // Maybe also play with log(input) to have a log scale
        // Could be that the sensor is already log
        // To prevent the log of zero check value before
        // if (MICValueRead > 0 ) MICValueRead= 10 * log(MICValueRead);  // So at 1024 it will give you 30.1 and at 512 it will be 27 , and 256 is 24
        // So the loud signals are compressed
        //MICValueRead = constrain(MICValueRead, 0, 30);
        //MICValueRead = abs(MICValueRead);
        //MICValue =  map(MICValueRead,NoiseLevel,30,0,NUM_LEDS); // For log algorith VU Meter.
                
        // syntax for map(value, fromLow, fromHigh, toLow, toHigh)
           
            MICValue =  map(MICValueRead,NoiseLevel,1025,0,NUM_LEDS);
            
            if (MICValue < 0) MICValue = 0;
            //Serial.print("MIC value mapped: ");
            //Serial.println(MICValue);
            if (MICValue > 0){
                for (int j=0; j < NUM_LEDS; j++) {            
                        leds[j] = rgb16convertor(BLACK);
                        }
                // Activate
                FastLED.show(); 
          
                for (int j=0; j <= MICValue-5; j++){  // Diesplay the VU meter value
                       leds[j] = rgb16convertor(currentcolor);
                }
                for (int j=MICValue-4; j <= MICValue; j++){  // Diesplay the VU meter value
                       if (j > NUM_LEDS -5)  {leds[j] = rgb16convertor(RED);
                       }
                       else  {
                        leds[j] = rgb16convertor(currentcolor);
                       }
                }
            }
            FastLED.show();
        }
bailout:  
// Serial.println("Bailout");
return 1;
 
}

//*********************************************************************************************

int rgb24convertor( long BitColor24) {
// Convert from 24 bit to 16 bit RGB
// 24 bit color code to 16 bits convertor for rgb565 and 16 to 24 bit convertor.
// 24bit is R8 G8 R8 and 16 bit is R5 G6 B5

long r,g,b;

r = BitColor24 >> 16; //Shift 16 to right.
g = ((unsigned long)BitColor24 << 16) >> 24;
b = ((unsigned long)BitColor24 <<24) >>24;  // shift 24 to left and than 24 to right.

r = ((unsigned int)r>>3)<<11;
// Serial.print("r+: ");Serial.println(r,BIN);
g = ((unsigned int)g>>2)<<5;
// Serial.print("g+: ");Serial.println(g,BIN);
b = (unsigned int)b>>3;
// Serial.print("b+: ");Serial.println(b,BIN);

uint16_t rgb565 = (r + g + b);

//Serial.print("rgb565: ");Serial.println(rgb565,HEX);
return  rgb565;
}

//*********************************************************************************************


long rgb16convertor( int BitColor16) {
// Convert from 16 bit to 24 bit RGB

int r,g,b;

r = ((((BitColor16 >> 11) & 0x1F) * 527) + 23) >> 6;
g = ((((BitColor16 >> 5) & 0x3F) * 259) + 33) >> 6;
b = (((BitColor16 & 0x1F) * 527) + 23) >> 6;

uint32_t RGB888 = (unsigned long)r << 16 | (unsigned long)g << 8 | b;
return RGB888;  
}

//*********************************************************************************************

int CleanLedStrip() {
    for (int j=0; j < NUM_LEDS; j++) {            
                        leds[j] = rgb16convertor(BLACK);
                        }
            // Activate
            FastLED.show(); 
            delay(5);
            
         }
// *******************************************************************************************
