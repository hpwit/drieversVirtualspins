#include "Arduino.h"
#include "FastLED.h"

#include <WiFi.h>

#define NUM_LEDS_PER_STRIP 369
#define NUM_STRIPS 16 //if set >1 - GOOD
#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP
#include "I2S.h"
#define LED_PIN1    2
#define LED_PIN2    0
#define LED_PIN3    4
#define LED_PIN4    5

#define LED_PIN5    18
#define LED_PIN6    19
#define LED_PIN7    23
#define LED_PIN8    15
//--------//--------
#define LED_PIN9     13
#define LED_PIN10    12
#define LED_PIN11    14
#define LED_PIN12    27

#define LED_PIN13    26
#define LED_PIN14    25 //25
#define LED_PIN15    33 //33
#define LED_PIN16    32 //32

int Pins[16] = {LED_PIN1, LED_PIN2, LED_PIN3, LED_PIN4,
                LED_PIN5, LED_PIN6, LED_PIN7, LED_PIN8,
                LED_PIN9, LED_PIN10, LED_PIN11, LED_PIN12,
                LED_PIN13, LED_PIN14, LED_PIN15, LED_PIN16
               };

CRGB leds[NUM_LEDS];
//I2S controller(0);

void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
FastLED.addLeds<WS2812BI2S,16,1,2,14,15>(leds,NUM_LEDS_PER_STRIP);

  //controller.showPixels();
}

uint8_t gHue = 0; // rotating "base color" used by many of the patterns

void loop() {

  //sinelon();
  bpm();
//  controller.showPixels();
  //delay(1);
  //FastLED.show();
  //Serial.println("Show");
  EVERY_N_MILLISECONDS( 20 ) {
    
    gHue++;  // slowly cycle the "base color" through the rainbow
  }
  //gHue++;  // slowly cycle the "base color" through the rainbow
  //Serial.print(gHue);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for ( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
}

const char *ssid = "Phidani";
const char *password = "322220663";

boolean ConnectWiFi() {
  boolean state = true;
  return true;
  int i = 0;
  //WiFi.begin("1", "12345678");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\tConnecting to WiFi: ");
  Serial.println(ssid);
  //Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false;
      break;
    }
    i++;
  }
  if (state) {
    Serial.println();
    Serial.println("Connected!");
        Serial.println("IP address: ");
Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Connected failed!");
     WiFi.mode(WIFI_OFF);
  }
  Serial.println();
  return state;
}
