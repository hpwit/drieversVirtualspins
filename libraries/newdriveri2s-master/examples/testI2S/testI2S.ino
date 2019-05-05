


#include "FastLED.h"


#define NUM_LEDS_PER_STRIP 256
#define NUM_STRIPS 16 //up to 22
#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP
#include "I2S.h"
    
int Pins[16]={12,2,4,5,0,13,14,15,16,17,18,19,21,22,23,25};

CRGB leds[NUM_LEDS];
I2S controller(0);

void setup() {
  // put your setup code here, to run once:
     Serial.begin(115200);
  

controller.initled(leds,Pins,NUM_STRIPS,NUM_LEDS_PER_STRIP);

//controller.initled(leds,Pins,NUM_STRIPS,NUM_LEDS_PER_STRIP,0); more suitable for ws2811

fill_solid(leds,100,CRGB::Yellow);
controller.showPixels();
controller.setBrightness(20); //to be used instead of fastled.setbritghness 
delay(200);

 controller.showPixels();
}


   

  

    



void loop() {



}
