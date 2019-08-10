#define ESP32_VIRTUAL_DRIVER true
#define NBIS2SERIALPINS 12

#include "FastLED.h"

#define LED_WIDTH 16
#define NUM_STRIPS NBIS2SERIALPINS*5
#define LED_HEIGHT_PER_STRIP 16
#define NUM_LEDS_PER_STRIP LED_HEIGHT_PER_STRIP*LED_WIDTH
#define LED_HEIGHT NUM_STRIPS*LED_HEIGHT_PER_STRIP
#define LATCH_PIN 12
#define CLOCK_PIN 27
#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP
CRGB leds[NUM_LEDS];
    
int Pins[12]={13,14,26,25,33,32,15,4,5,18,19,21};
//int *Pins;
void setup() {
  Serial.begin(115200);
 FastLED.addLeds<VIRTUAL_DRIVER,Pins,CLOCK_PIN, LATCH_PIN,NUM_LEDS_PER_STRIP>(leds,NUM_LEDS);
  for(int j=0;j<NUM_STRIPS;j++)
  {
  for(int i=0;i<j+4;i++)
  {
    leds[i+j*NUM_LEDS_PER_STRIP]=CRGB::Red;  
  }
  }
  FastLED.show();
  // put your setup code here, to run once:

}
int k=0;
void loop() {
  // put your main code here, to run repeatedly:
  for(int j=0;j<NUM_STRIPS;j++)
  {
  for(int i=0;i<NUM_LEDS_PER_STRIP;i++)
  {
    leds[i+j*NUM_LEDS_PER_STRIP]=CHSV(i+k,255,255);  
  }
  }
  FastLED.show();
  k=k+10;

}
