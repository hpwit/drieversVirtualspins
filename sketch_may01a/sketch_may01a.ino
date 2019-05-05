#define FASTLED_ESP32_I2S 1
#include "FastLED.h"

CRGB leds1[256];
CRGB leds2[22];
void setup() {
  Serial.begin(115200);
  
    FastLED.addLeds<WS2812B, 13, GRB>(leds2,22).setCorrection(TypicalLEDStrip);
 
   FastLED.addLeds<WS2812B, 12, GRB>(leds1,256).setCorrection(TypicalLEDStrip); // put your setup code here, to run once:

}
int k=0;
void loop() {
fill_solid(leds1,256,CRGB::Black);
fill_solid(leds2,10,CRGB::Black);
leds1[k%256]=CRGB::Red;
leds2[k%10]=CRGB::Blue;
k++;
  FastLED.show();
  // put your main code here, to run repeatedly:
delay(100);
}
