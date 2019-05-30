/*
This example will receive multiple universes via Artnet and control a strip of sk6812RGBW leds via
Adafruit's NeoPixel library: https://github.com/adafruit/Adafruit_NeoPixel
This example may be copied under the terms of the MIT license, see the LICENSE file for details
*/

#include <Artnet.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>

// Neopixel settings
const int numLeds = 120; // change for your setup
const int numberOfChannels = numLeds * 4; // Total number of channels you want to receive (1 led = 4 channels)
const byte dataPin = 2;
Adafruit_NeoPixel leds = Adafruit_NeoPixel(numLeds, dataPin, NEO_GRBW + NEO_KHZ800);

// Artnet settings
Artnet artnet;
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.

// Check if we got all universes
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;

// Change ip and mac address for your setup
byte ip[] = {192, 168, 2, 2};
byte mac[] = {0x04, 0xE9, 0xE5, 0x00, 0x69, 0xEC};

void setup()
{
  //Serial.begin(115200);
  artnet.begin(mac, ip);
  leds.begin();
  initTest();

  // this will be called for each packet received
  artnet.setArtDmxCallback(onDmxFrame);
}

void loop()
{
  // we call the read function inside the loop
  artnet.read();
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP)
{
  sendFrame = 1;
  // set brightness of the whole strip
  if (universe == 15)
  {
    leds.setBrightness(data[0]);
    leds.show();
  }

  // Store which universe has got in
  if ((universe - startUniverse) < maxUniverses)
    universesReceived[universe - startUniverse] = 1;

  for (int i = 0 ; i < maxUniverses ; i++)
  {
    if (universesReceived[i] == 0)
    {
      //Serial.println("Broke");
      sendFrame = 0;
      break;
    }
  }

  // read universe and put into the right part of the display buffer
  for (int i = 0; i < length / 4; i++)
  {
    int led = i + (universe - startUniverse) * (previousDataLength / 4);
    if (led < numLeds)
      leds.setPixelColor(led, data[i * 4], data[i * 4 + 1], data[i * 4 + 2], data[i * 4 + 3]);
  }
  previousDataLength = length;

  if (sendFrame)
  {
    leds.show();
    // Reset universeReceived to 0
    memset(universesReceived, 0, maxUniverses);
  }
}

void initTest()
{
  for (int i = 0 ; i < numLeds ; i++)
    leds.setPixelColor(i, 127, 0, 0, 0);
  leds.show();
  delay(500);
  for (int i = 0 ; i < numLeds ; i++)
    leds.setPixelColor(i, 0, 127, 0, 0);
  leds.show();
  delay(500);
  for (int i = 0 ; i < numLeds ; i++)
    leds.setPixelColor(i, 0, 0, 127, 0);
  leds.show();
  delay(500);
    for (int i = 0 ; i < numLeds ; i++)
    leds.setPixelColor(i, 0, 0, 0, 127);
  leds.show();
  delay(500);
  for (int i = 0 ; i < numLeds ; i++)
    leds.setPixelColor(i, 0, 0, 0, 0);
  leds.show();
}
