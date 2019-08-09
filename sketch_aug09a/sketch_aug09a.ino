#define ESP32_VIRTUAL_DRIVER true
#define NBIS2SERIALPINS 4

#include "FastLED.h"

#define LED_WIDTH 16
#define NUM_STRIPS 20
#define LED_HEIGHT_PER_STRIP 16
#define NUM_LEDS_PER_STRIP LED_HEIGHT_PER_STRIP*LED_WIDTH
#define LED_HEIGHT NUM_STRIPS*LED_HEIGHT_PER_STRIP
#define LATCH_PIN 12
#define CLOCK_PIN 27
#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP
CRGB leds[NUM_LEDS];
    
int Pins[4];
//int *Pins;
void setup() {
 FastLED.addLeds<VIRTUAL_DRIVER,Pins,CLOCK_PIN, LATCH_PIN,NUM_LEDS_PER_STRIP>(leds,NUM_LEDS);
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
