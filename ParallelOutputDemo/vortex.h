//
//  vortex.h
//  
//
//  Created by Yves BAZIN on 27/11/2018.
//

#ifndef vortex_h
#define vortex_h
const bool    kMatrixSerpentineLayout = true;

uint16_t XY( uint8_t x, uint8_t y)
{
    uint16_t i;
    
    if ( kMatrixSerpentineLayout == false) {
        i = (y * kMatrixWidth) + x;
    }
    
    if ( kMatrixSerpentineLayout == true) {
        if ( y & 0x01) {
            // Odd rows run backwards
            uint8_t reverseX = (kMatrixWidth - 1) - x;
            i = (y * kMatrixWidth) + reverseX;
        } else {
            // Even rows run forwards
            i = (y * kMatrixWidth) + x;
        }
    }
    
    return i;
}

struct vtx {
    int x = 1;
    int y = 0;
    int right = 1;
    int up = 0;
    int w = kMatrixWidth - 2;
    int h = kMatrixHeight - 1;
    int margin = 0;
    int hue;
};

class Vortex {
    int x = 1;
    int y = 0;
    int right = 1;
    int up = 0;
    int w = kMatrixWidth - 2;
    int h = kMatrixHeight - 1;
    int margin = 0;
    int hue;
    
public:
    Vortex() {
        
    }
    
    void vortex() {
        if(y&001)
        leds[ (kMatrixWidth <<1)* ( (y>>1) + 1) - (1 + x)]  = CHSV(hue, 255, 255); //XY(x, y)
        else
             leds[ kMatrixWidth*y+ x]  = CHSV(hue, 255, 255);
        // End of right
        if ((x >= (w - margin)) && (up == 0)) {
            up = 1;
            right = 0;
            //    vortexHue += 15;
        }
        // End of up
        else if ((y >= (h - margin)) && (up == 1)) {
            up = 0;
            right = -1;
            //    vortexHue += 15;
        }
        // End of left
        else if ((x == (0 + margin)) && (up == 0 && right == -1)) {
            up = -1;
            right = 0;
            //    vortexHue += 15;
        }
        // End of down
        else if ((x == (0 + margin) && y == (1 + margin)) && (up == -1 && right == 0)) {
            y += up;
            up = 0;
            right = 1;
            hue += 15;
            margin++;
            y++;
        }
        
        // Final LED
        if (right == 0 && up == -1 && margin == ( (kMatrixHeight / 2) - 1)) { // >=
            //delay(5000);
            hue = random(0, 255);
            margin = 0;
            x = -1;
            y = 0;
            right = 1;
            up = 0;
        }
        
        // Option toggle?
        //  fadeLightBy(leds, (kMatrixWidth * kMatrixHeight), 20);
        x += right;
        y += up;
    }
};
#define SNAKE_GAP 30
#define NUM_SNAKES 100
vtx *vortexs;
void vortex2(int vnumber) {
    if(vortexs[vnumber].y&001)
        leds[ (kMatrixWidth <<1)* ( (vortexs[vnumber].y>>1) + 1) - (1 + vortexs[vnumber].x)]  = CHSV(vortexs[vnumber].hue, 255, 255); //XY(x, y)
    else
        leds[ kMatrixWidth*vortexs[vnumber].y+ vortexs[vnumber].x]  = CHSV(vortexs[vnumber].hue, 255, 255);
    // End of right
    if ((vortexs[vnumber].x >= (vortexs[vnumber].w - vortexs[vnumber].margin)) && (vortexs[vnumber].up == 0)) {
        vortexs[vnumber].up = 1;
        vortexs[vnumber].right = 0;
        //    vortexHue += 15;
    }
    // End of up
    else if ((vortexs[vnumber].y >= (vortexs[vnumber].h - vortexs[vnumber].margin)) && (vortexs[vnumber].up == 1)) {
        vortexs[vnumber].up = 0;
        vortexs[vnumber].right = -1;
        //    vortexHue += 15;
    }
    // End of left
    else if ((vortexs[vnumber].x == (0 + vortexs[vnumber].margin)) && (vortexs[vnumber].up == 0 && vortexs[vnumber].right == -1)) {
        vortexs[vnumber].up = -1;
        vortexs[vnumber].right = 0;
        //    vortexHue += 15;
    }
    // End of down
    else if ((vortexs[vnumber].x == (0 + vortexs[vnumber].margin) && vortexs[vnumber].y == (1 + vortexs[vnumber].margin)) && (vortexs[vnumber].up == -1 && vortexs[vnumber].right == 0)) {
        vortexs[vnumber].y +=vortexs[vnumber].up;
        vortexs[vnumber].up = 0;
        vortexs[vnumber].right = 1;
        vortexs[vnumber].hue += 15;
        vortexs[vnumber].margin++;
        vortexs[vnumber].y++;
    }
    
    // Final LED
    if (vortexs[vnumber].right == 0 && vortexs[vnumber].up == -1 && vortexs[vnumber].margin == ( (kMatrixHeight / 2) - 1)) { // >=
        //delay(5000);
        vortexs[vnumber].hue = random(0, 255);
        vortexs[vnumber].margin = 0;
        vortexs[vnumber].x = -1;
        vortexs[vnumber].y = 0;
        vortexs[vnumber].right = 1;
        vortexs[vnumber].up = 0;
    }
    
    // Option toggle?
    //  fadeLightBy(leds, (kMatrixWidth * kMatrixHeight), 20);
    vortexs[vnumber].x += vortexs[vnumber].right;
    vortexs[vnumber].y += vortexs[vnumber].up;
}
//((NUM_LEDS / SNAKE_GAP) / 2)
// [NUM_SNAKES];

int vortexsStarted = 1;
//int const vortexDelay = ((kMatrixWidth * 2) + (kMatrixHeight * 2) - 1);
int const vortexDelay = SNAKE_GAP * 2;
int vortexGap = 0;

void vortex() {
    
    if (vortexsStarted < NUM_SNAKES) {
        if (vortexGap == vortexDelay)  {
            vortexsStarted++;
            vortexGap = 0;
        }
        else {
            vortexGap++;
        }
    }
    
    for (int  i = 0; i < vortexsStarted; i++) {
        //vortexs[i].vortex();
        vortex2(i);
        
    }
    fadeToBlackBy(leds, (kMatrixWidth * kMatrixHeight), 30); // TWEAK ME
    //FastLED.delay(20);
    //FastLEDshowESP322()
    EVERY_N_SECONDS(10) {
        Serial.println(FastLED.getFPS());
    }
}





#endif /* vortex_h */
