#include "FastLED.h"

CRGB leds[50*256];
uint8_t pixelg[8][8][3] ;
uint8_t pixelr[8][5];
uint8_t pixelb[8][5];//={{ 1, 0, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129}};
uint32_t buff[64*3*3];
uint32_t bug[25];
/*__attribute__ ((always_inline)) inline static uint32_t __clock_cycles() {
    uint32_t cyc;
    __asm__ __volatile__ ("rsr %0,ccount":"=a" (cyc));
    return cyc;
}*/
  void transpose32(uint8_t * pixels, uint8_t * bits)
    {
        transpose8rS32(& pixels[0],  1, 4, & bits[0]);
        transpose8rS32(& pixels[8],  1, 4, & bits[1]);
        transpose8rS32(& pixels[16], 1, 4, & bits[2]);
        //transpose8rS32(& pixels[24], 1, 4, & bits[3]);
    }
    
     void transpose8rS32(uint8_t * A, int m, int n, uint8_t * B)
    {
        uint32_t x, y, t;
        
        // Load the array and pack it into x and y.
        
        x = (A[0]<<24)   | (A[m]<<16)   | (A[2*m]<<8) | A[3*m];
        y = (A[4*m]<<24) | (A[5*m]<<16) | (A[6*m]<<8) | A[7*m];
        
        t = (x ^ (x >> 7)) & 0x00AA00AA;  x = x ^ t ^ (t << 7);
        t = (y ^ (y >> 7)) & 0x00AA00AA;  y = y ^ t ^ (t << 7);
        
        t = (x ^ (x >>14)) & 0x0000CCCC;  x = x ^ t ^ (t <<14);
        t = (y ^ (y >>14)) & 0x0000CCCC;  y = y ^ t ^ (t <<14);
        
        t = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
        y = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);
        x = t;
        
        B[0]=x>>24;    B[n]=x>>16;    B[2*n]=x>>8;  B[3*n]=x;
        B[4*n]=y>>24;  B[5*n]=y>>16;  B[6*n]=y>>8;  B[7*n]=y;
}
void  transpose24x1_noinline(unsigned char *A, uint32_t *B) {
        uint32_t  x, y, x1,y1,t,x2,y2;
        
        y = *(unsigned int*)(A);
        x = *(unsigned int*)(A+4);
        y1 = *(unsigned int*)(A+8);
        x1 = *(unsigned int*)(A+12);
        
        y2 = *(unsigned int*)(A+16);
        x2 = *(unsigned int*)(A+20);
        
        
        // pre-transform x
        t = (x ^ (x >> 7)) & 0x00AA00AA;  x = x ^ t ^ (t << 7);
        t = (x ^ (x >>14)) & 0x0000CCCC;  x = x ^ t ^ (t <<14);
        
        t = (x1 ^ (x1 >> 7)) & 0x00AA00AA;  x1 = x1 ^ t ^ (t << 7);
        t = (x1 ^ (x1 >>14)) & 0x0000CCCC;  x1 = x1 ^ t ^ (t <<14);
        
        t = (x2 ^ (x2 >> 7)) & 0x00AA00AA;  x2 = x2 ^ t ^ (t << 7);
        t = (x2 ^ (x2 >>14)) & 0x0000CCCC;  x2 = x2 ^ t ^ (t <<14);
        
        // pre-transform y
        t = (y ^ (y >> 7)) & 0x00AA00AA;  y = y ^ t ^ (t << 7);
        t = (y ^ (y >>14)) & 0x0000CCCC;  y = y ^ t ^ (t <<14);
        
        t = (y1 ^ (y1 >> 7)) & 0x00AA00AA;  y1 = y1 ^ t ^ (t << 7);
        t = (y1 ^ (y1 >>14)) & 0x0000CCCC;  y1 = y1 ^ t ^ (t <<14);
        
        t = (y2 ^ (y2 >> 7)) & 0x00AA00AA;  y2 = y2 ^ t ^ (t << 7);
        t = (y2 ^ (y2 >>14)) & 0x0000CCCC;  y2 = y2 ^ t ^ (t <<14);
        
        // final transform
        t = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
        y = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);
        x = t;
        
        t = (x1 & 0xF0F0F0F0) | ((y1 >> 4) & 0x0F0F0F0F);
        y1 = ((x1 << 4) & 0xF0F0F0F0) | (y1 & 0x0F0F0F0F);
        x1 = t;
        
        t = (x2 & 0xF0F0F0F0) | ((y2 >> 4) & 0x0F0F0F0F);
        y2 = ((x2 << 4) & 0xF0F0F0F0) | (y2 & 0x0F0F0F0F);
        x2 = t;
        
        
        
        *((uint32_t*)B) = (uint32_t)((y & 0xff) |  (  (y1 & 0xff) << 8 )  |  (  (y2 & 0xff) << 16 ) )   ;
        *((uint32_t*)(B+1)) = (uint32_t)(((y & 0xff00) |((y1&0xff00) <<8)  |((y2&0xff00) <<16)  )>>8    );
        *((uint32_t*)(B+2)) =(uint32_t)(  (  (y & 0xff0000) >>16)|((y1&0xff0000) >>8)   |((y2&0xff0000))     );
        *((uint32_t*)(B+3)) = (uint32_t)(((y & 0xff000000) >>16 |((y1&0xff000000)>>8 ) |((y2&0xff000000) )  )>>8);
        
        *((uint32_t*)B+4) =(uint32_t)( (x & 0xff) |((x1&0xff) <<8)  |((x2&0xff) <<16) );
        *((uint32_t*)(B+5)) = (uint32_t)(((x & 0xff00) |((x1&0xff00) <<8)    |((x2&0xff00) <<16)    )>>8);
        *((uint32_t*)(B+6)) = (uint32_t)(  (  (x & 0xff0000) >>16)|((x1&0xff0000) >>8)   |((x2&0xff0000))     );
        *((uint32_t*)(B+7)) = (uint32_t)(((x & 0xff000000) >>16 |((x1&0xff000000)>>8 )    |((x2&0xff000000) )    )>>8);
        
    }


/*

void putpixels2(uint32_t* buff)
{
int nblines=5;
  int nbbits=8;
  uint32_t m;
  int offset;
  for (int i = 0; i < nbbits; i++)
  {
    for (int j = 0; j < nblines; j++)
    {
      offset=(nbbits-1-i)*18+j+6;
     m=(pixelg[0][j].g&1 )+ ((pixelg[1][j].g&1 )<<1) + ((pixelg[2][j].g&1 )<<2)+ ((pixelg[3][j].g&1 )<<3)+ ((pixelg[4][j].g&1 )<<4);//+ ((pixelg[5][j].g & 1 )<<5);//+ ((pixelg[6][j].g & 1 )<<6)+ ((pixelg[7][j].g & 1 )<<7);
   buff[offset]= m<<8;
    /*  pixelg[0][j] = pixelg[0][j] >> 1;
       pixelg[1][j] = pixelg[1][j] >> 1;
       pixelg[2][j] = pixelg[2][j] >> 1;
        pixelg[3][j] = pixelg[3][j] >> 1;
          pixelg[4][j] = pixelg[4][j] >> 1;
             pixelg[5][j] = pixelg[5][j] >> 1;
            pixelg[6][j] = pixelg[6][j] >> 1;
             pixelg[7][j] = pixelg[7][j] >> 1;


     m=(pixelg[0][j].r&1)+ ((pixelg[1][j].r&1)<<1) + ((pixelg[2][j].r&1 )<<2)+ ((pixelg[3][j].r&1 )<<3)+ ((pixelg[4][j].r&1 )<<4);//+ ((pixelg[5][j].r & 1 )<<5);//+ ((pixelg[6][j].g & 1 )<<6)+ ((pixelg[7][j].g & 1 )<<7);
      buff[offset+8*18]=m<<8;
    /*  pixelr[0][j] = pixelr[0][j] >> 1;
       pixelr[1][j] = pixelr[1][j] >> 1;
       pixelr[2][j] = pixelr[2][j] >> 1;
        pixelr[3][j] = pixelr[3][j] >> 1;
          pixelr[4][j] = pixelr[4][j] >> 1;
            pixelr[5][j] = pixelr[5][j] >> 1;
            pixelr[6][j] = pixelr[6][j] >> 1;
             pixelr[7][j] = pixelr[7][j] >> 1;

             
      //buff[((nbbits-1-i)+16)*18+j+6]=(pixelg[0][j]& 0x1000000 )+ ((pixelg[1][j]& 0x1000000 )<<1) + ((pixelg[2][j] & 0x1000000 )<<2)+ ((pixelg[3][j] & 0x1000000 )<<3)+ ((pixelg[4][j] & 0x1000000 )<<4);//+ ((pixelb[5][j] & 1 )<<5)+ ((pixelb[6][j] & 1 )<<6)+ ((pixelb[7][j] & 1 )<<7);
     m=(pixelg[0][j].b&1 )+ ((pixelg[1][j].b&1)<<1) + ((pixelg[2][j].b&1 )<<2)+ ((pixelg[3][j].b&1 )<<3)+ ((pixelg[4][j].b&1 )<<4);//+ ((pixelg[5][j].b & 1 )<<5);//+((pixelg[6][j].b & 1 )<<6)+ ((pixelg[7][j].b & 1 )<<7);;
     buff[offset+16*18]=m<<8;
     /*    pixelb[0][j] = pixelb[0][j] >> 1;
       pixelb[1][j] = pixelb[1][j] >> 1;
        pixelb[2][j] = pixelb[2][j] >> 1;
        pixelb[3][j] = pixelb[3][j] >> 1;
         pixelb[4][j] = pixelb[4][j] >> 1;
          pixelb[5][j] = pixelb[5][j] >> 1;
            pixelb[6][j] = pixelb[6][j] >> 1;
             pixelb[7][j] = pixelb[7][j] >> 1;
             
            pixelg[0][j] = pixelg[0][j] >> 1;
       pixelg[1][j] = pixelg[1][j] >> 1;
       pixelg[2][j] = pixelg[2][j] >> 1;
        pixelg[3][j] = pixelg[3][j] >> 1;
          pixelg[4][j] = pixelg[4][j] >> 1;
         /*    pixelg[5][j] = pixelg[5][j] >> 1;
           pixelg[6][j] = pixelg[6][j] >> 1;
             pixelg[7][j] = pixelg[7][j] >> 1;
             
         
    }
  //  *(uint32_t *)(pixel[0])=(*(uint32_t*)(pixel[0]))>>1;

  }
}*/
int ledToDisplay=4;
      int nun_led_per_strip=256;

   void  fillbuffer2(uint32_t *buff)
  {
//  CRGB pixelg[8][5];
uint8_t pixelg[8][8][4] ;
    for(int pin=0;pin<7;pin++)
    {
      for (int line=0;line<5;line++)
      {
        //pixelg[pin][line]=leds[ledToDisplay+nun_led_per_strip*(line+pin*5)];
     memcpy(pixelg[pin][line],leds+ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5,3);
    
       // pixelg[pin][line]=((uint32_t)leds[ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5])<<8;
     // CRGB color=leds[ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5];
       // pixelg[pin][line]=color.g;
       // pixelr[pin][line]=color.r;
        //pixelb[pin][line]=color.b;
      }
    }
  // Serial.printf("led :%d,%d:%d:%d \n",ledToDisplay,pixel[0][0].g,pixel[0][0].r,pixel[0][0].b);
  //  putpixels2(buff);
   //return;
   int nblines=5;
  int nbbits=8;
  uint32_t m;
  uint32_t *f;
  int off;
  for (int i = 0; i < nbbits; i++)
  {
   // off=(nbbits-1-i)*18+nblines-1+6;
    for (int j = 0; j < nblines; j++)
    {
      m=(pixelg[0][j][1]&1 )+ ((pixelg[1][j][1]&1 )<<1) + ((pixelg[2][j][1]&1 )<<2)+ ((pixelg[3][j][1]&1 )<<3)+ ((pixelg[4][j][1]&1 )<<4)  + ((pixelg[5][j][1]&1 )<<5)  + ((pixelg[6][j][1]&1 )<<6) ;// + ((pixelg[7][j][1]&1 )<<7);
     buff[(nbbits-1-i)*18+nblines-1-j+6]=m<<8;
   //buff[off-j]=m<<8;


      m=(pixelg[0][j][0]&1)+ ((pixelg[1][j][0]&1)<<1) + ((pixelg[2][j][0]&1 )<<2)+ ((pixelg[3][j][0]&1 )<<3) + ((pixelg[4][j][0]&1 )<<4) + ((pixelg[5][j][0]&1 )<<5)  + ((pixelg[6][j][0]&1 )<<6) ;// + ((pixelg[7][j][0]&1 )<<7) ;
    
      buff[((nbbits-1-i)+8)*18+nblines-1-j+6]=m<<8;
   // buff+=8*18;
    //*buff=m<<8;

             
     m=(pixelg[0][j][2]&1 )+ ((pixelg[1][j][2]&1)<<1) + ((pixelg[2][j][2]&1 )<<2)+ ((pixelg[3][j][2]&1 )<<3) + ((pixelg[4][j][2]&1 )<<4) + ((pixelg[5][j][2]&1 )<<5) + ((pixelg[6][j][2]&1 )<<6) ;// + ((pixelg[7][j][2]&1 )<<7);
         buff[((nbbits-1-i)+16)*18+nblines-1-j+6]=m<<8;
        //buff[off-j+16*18]=m<<8;
      //   buff+=8*18;
    //*buff=m<<8;
  f=(uint32_t*)pixelg[0][j]; 
 *f=*f>>1;
  f=(uint32_t*)pixelg[1][j]; 
 *f=*f>>1;
 f=(uint32_t*)pixelg[2][j]; 
 *f=*f>>1;
   f=(uint32_t*)pixelg[3][j]; 
 *f=*f>>1; 
 f=(uint32_t*)pixelg[4][j]; 
 *f=*f>>1;
 f=(uint32_t*)pixelg[5][j]; 
 *f=*f>>1;
  f=(uint32_t*)pixelg[6][j]; 
 *f=*f>>1;
  //f=(uint32_t*)pixelg[7][j]; 
 //*f=*f>>1;            
         
    }
  //  *(uint32_t *)(pixel[0])=(*(uint32_t*)(pixel[0]))>>1;

  }
  }


   void  fillbuffer4(uint32_t *buff)
  {
//  CRGB pixelg[8][5];
uint8_t pixelg[8][8][3] ;
    for(int pin=0;pin<5;pin++)
    {
      for (int line=0;line<5;line++)
      {
        //pixelg[pin][line]=leds[ledToDisplay+nun_led_per_strip*(line+pin*5)];
     memcpy(pixelg[pin][line],leds+ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5,3);
    
       // pixelg[pin][line]=((uint32_t)leds[ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5])<<8;
     // CRGB color=leds[ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5];
       // pixelg[pin][line]=color.g;
       // pixelr[pin][line]=color.r;
        //pixelb[pin][line]=color.b;
      }
    }
  // Serial.printf("led :%d,%d:%d:%d \n",ledToDisplay,pixel[0][0].g,pixel[0][0].r,pixel[0][0].b);
  //  putpixels2(buff);
   //return;
   int nblines=5;
  int nbbits=8;
  uint32_t m;
  for (int i = 0; i < nbbits; i++)
  {
    for (int j = 0; j < nblines; j++)
    {
      m=(pixelg[0][j][1]&1 )+ ((pixelg[1][j][1]&1 )<<1) + ((pixelg[2][j][1]&1 )<<2)+ ((pixelg[3][j][1]&1 )<<3)+ ((pixelg[4][j][1]&1 )<<4);//  + ((pixelg[5][j][1]&1 )<<5);//+ ((pixelg[4][j].g&1 )<<4);//+ ((pixelg[5][j].g & 1 )<<5);//+ ((pixelg[6][j] & 1 )<<6)+ ((pixelg[7][j] & 1 )<<7);
     buff[(nbbits-1-i)*18+j+6]=m<<8;
    /*  pixelg[0][j] = pixelg[0][j] >> 1;
       pixelg[1][j] = pixelg[1][j] >> 1;
       pixelg[2][j] = pixelg[2][j] >> 1;
        pixelg[3][j] = pixelg[3][j] >> 1;
          pixelg[4][j] = pixelg[4][j] >> 1;
             pixelg[5][j] = pixelg[5][j] >> 1;
            pixelg[6][j] = pixelg[6][j] >> 1;
             pixelg[7][j] = pixelg[7][j] >> 1;*/


      m=(pixelg[0][j][0]&1)+ ((pixelg[1][j][0]&1)<<1) + ((pixelg[2][j][0]&1 )<<2)+ ((pixelg[3][j][0]&1 )<<3) + ((pixelg[4][j][0]&1 )<<4);// + ((pixelg[5][j][0]&1 )<<5) ;//+ ((pixelg[4][j].r&1 )<<4);//+ ((pixelg[5][j].r & 1 )<<5);//+ ((pixelr[6][j] & 1 )<<6)+ ((pixelr[7][j] & 1 )<<7);
      buff[((nbbits-1-i)+8)*18+j+6]=m<<8;
    /*  pixelr[0][j] = pixelr[0][j] >> 1;
       pixelr[1][j] = pixelr[1][j] >> 1;
       pixelr[2][j] = pixelr[2][j] >> 1;
        pixelr[3][j] = pixelr[3][j] >> 1;
          pixelr[4][j] = pixelr[4][j] >> 1;
            pixelr[5][j] = pixelr[5][j] >> 1;
            pixelr[6][j] = pixelr[6][j] >> 1;
             pixelr[7][j] = pixelr[7][j] >> 1;*/

             
      //buff[((nbbits-1-i)+16)*18+j+6]=(pixelg[0][j]& 0x1000000 )+ ((pixelg[1][j]& 0x1000000 )<<1) + ((pixelg[2][j] & 0x1000000 )<<2)+ ((pixelg[3][j] & 0x1000000 )<<3)+ ((pixelg[4][j] & 0x1000000 )<<4);//+ ((pixelb[5][j] & 1 )<<5)+ ((pixelb[6][j] & 1 )<<6)+ ((pixelb[7][j] & 1 )<<7);
    m=(pixelg[0][j][2]&1 )+ ((pixelg[1][j][2]&1)<<1) + ((pixelg[2][j][2]&1 )<<2)+ ((pixelg[3][j][2]&1 )<<3) + ((pixelg[4][j][2]&1 )<<4) ;// + ((pixelg[5][j][2]&1 )<<5);//+ ((pixelg[4][j].b&1 )<<4);//+ ((pixelg[5][j].b & 1 )<<5);
      buff[((nbbits-1-i)+16)*18+j+6]=m<<8;
     /*    pixelb[0][j] = pixelb[0][j] >> 1;
       pixelb[1][j] = pixelb[1][j] >> 1;
        pixelb[2][j] = pixelb[2][j] >> 1;
        pixelb[3][j] = pixelb[3][j] >> 1;
         pixelb[4][j] = pixelb[4][j] >> 1;
          pixelb[5][j] = pixelb[5][j] >> 1;
            pixelb[6][j] = pixelb[6][j] >> 1;
             pixelb[7][j] = pixelb[7][j] >> 1;*/
            
            pixelg[0][j][2] = pixelg[0][j][2] >>1;
       pixelg[1][j][2] = pixelg[1][j][2] >>1;
       pixelg[2][j][2] = pixelg[2][j][2]>>1;
        pixelg[3][j][2] = pixelg[3][j][2] >>1;
     pixelg[4][j][2] = pixelg[4][j][2] >>1;
    // pixelg[5][j][2] = pixelg[5][j][2] >>1;
         // pixelg[4][j].r = pixelg[4][j].r >> 1;
                 pixelg[0][j][1] = pixelg[0][j][1] >>1;
       pixelg[1][j][1] = pixelg[1][j][1] >>1;
       pixelg[2][j][1] = pixelg[2][j][1] >>1;
        pixelg[3][j][1] = pixelg[3][j][1] >>1;
    pixelg[4][j][1] = pixelg[4][j][1] >>1;
    //pixelg[5][j][1] = pixelg[5][j][1] >>1;
        //  pixelg[4][j].g = pixelg[4][j].g >> 1;
       pixelg[0][j][0] = pixelg[0][j][0] >>1;
       pixelg[1][j][0] = pixelg[1][j][0] >>1;
       pixelg[2][j][0] = pixelg[2][j][0] >>1;
        pixelg[3][j][0] = pixelg[3][j][0] >>1;
     pixelg[4][j][0] = pixelg[4][j][0] >>1;
     // pixelg[5][j][0] = pixelg[5][j][0] >>1;
         /* pixelg[4][j][0] = pixelg[4][j][0] >> 1;
            pixelg[5][j] = (pixelg[5][j] >>= 1);
            pixelg[6][j] = pixelg[6][j] >> 1;
             pixelg[7][j] = pixelg[7][j] >> 1;*/
             
         
    }
  //  *(uint32_t *)(pixel[0])=(*(uint32_t*)(pixel[0]))>>1;

  }
  }

 void  fillbuffer3(uint32_t *buff)
  {
//  CRGB pixelg[8][5];
uint8_t pixelg[8][8][4] ;
uint32_t *f;
    for(int pin=0;pin<8;pin++)
    {
      for (int line=0;line<5;line++)
      {
        //pixelg[pin][line]=leds[ledToDisplay+nun_led_per_strip*(line+pin*5)];
     //memcpy(pixelg[pin][line],leds+ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5,3);
     f=(uint32_t*)pixelg[pin][line]; 
     *f=*f>>pin;
       // pixelg[pin][line]=((uint32_t)leds[ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5])<<8;
     // CRGB color=leds[ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5];
       // pixelg[pin][line]=color.g;
       // pixelr[pin][line]=color.r;
        //pixelb[pin][line]=color.b;
      }
    }
  // Serial.printf("led :%d,%d:%d:%d \n",ledToDisplay,pixel[0][0].g,pixel[0][0].r,pixel[0][0].b);
  //  putpixels2(buff);
   //return;
   int nblines=5;
  int nbbits=8;
  uint32_t m;
  int off;
  for (int i = 0; i < nbbits; i++)
  {
   // off=(nbbits-1-i)*18+nblines-1+6;
    for (int j = 0; j < nblines; j++)
    {
      m=(pixelg[0][j][1]&1 )+ ((pixelg[1][j][1]&2 )) + ((pixelg[2][j][1]&4 ))+ ((pixelg[3][j][1]&8 ))+ ((pixelg[4][j][1]&16 ))  + ((pixelg[5][j][1]&32 ))  + ((pixelg[6][j][1]&64 ))  + ((pixelg[7][j][1]&1 )<<7);
     buff[(nbbits-1-i)*18+nblines-1-j+6]=m<<8;
   //buff[off-j]=m<<8;


      m=(pixelg[0][j][0]&1)+ ((pixelg[1][j][0]&2)) + ((pixelg[2][j][0]&4 ))+ ((pixelg[3][j][0]&8 )) + ((pixelg[4][j][0]&16 )) + ((pixelg[5][j][0]&32 ))  + ((pixelg[6][j][0]&64 ))  + ((pixelg[7][j][0]&1 )<<7) ;
    
      buff[((nbbits-1-i)+8)*18+nblines-1-j+6]=m<<8;
   // buff+=8*18;
    //*buff=m<<8;

             
     m=(pixelg[0][j][2]&1 )+ ((pixelg[1][j][2]&2)) + ((pixelg[2][j][2]&4 ))+ ((pixelg[3][j][2]&8 )) + ((pixelg[4][j][2]&16 )) + ((pixelg[5][j][2]&32 )) + ((pixelg[6][j][2]&64 ))  + ((pixelg[7][j][2]&1 )<<7);
         buff[((nbbits-1-i)+16)*18+nblines-1-j+6]=m<<8;
        //buff[off-j+16*18]=m<<8;
      //   buff+=8*18;
    //*buff=m<<8;
  f=(uint32_t*)pixelg[0][j]; 
 *f=*f>>1;
  f=(uint32_t*)pixelg[1][j]; 
 *f=*f>>1;
 f=(uint32_t*)pixelg[2][j]; 
 *f=*f>>1;
   f=(uint32_t*)pixelg[3][j]; 
 *f=*f>>1; 
 f=(uint32_t*)pixelg[4][j]; 
 *f=*f>>1;
 f=(uint32_t*)pixelg[5][j]; 
 *f=*f>>1;
  f=(uint32_t*)pixelg[6][j]; 
 *f=*f>>1;
  f=(uint32_t*)pixelg[7][j]; 
 *f=*f>>1;            
         
    }
  //  *(uint32_t *)(pixel[0])=(*(uint32_t*)(pixel[0]))>>1;

  }
  }

  
void setup() {
  Serial.begin(115200);
  // put your setup code here, to run once:
  

 long lastHandle = __clock_cycles();
 // transpose8x1_noinline(pixel[0],buff);
  Serial.printf("GetTime: %f us\n", (float)(__clock_cycles() - lastHandle) / 240);
  
 lastHandle =__clock_cycles();
 //transpose24x1_noinline(buff,bug);
 long lasthandle2=__clock_cycles();
    Serial.printf("GetTime: %f us\n", (float)(lasthandle2 - lastHandle) / 240);


    
 lastHandle = __clock_cycles();
    //transpose32(buff,buff);
    lasthandle2=__clock_cycles();
       Serial.printf("GetTime: %f us\n", (float)(lasthandle2 - lastHandle) / 240);
       int nb=1;
    
        
     
        lastHandle = __clock_cycles();
        for (int i=0;i<nb;i++)
        {
          ledToDisplay=ledToDisplay%nun_led_per_strip;
          fillbuffer4((uint32_t *)buff);
        }
    lasthandle2=__clock_cycles();
       Serial.printf("GetTime fillbuffer4 5 pins : %f us\n", (float)(lasthandle2 - lastHandle) / 240/nb);    
        
        lastHandle = __clock_cycles();
        for (int i=0;i<nb;i++)
        {
          ledToDisplay=ledToDisplay%nun_led_per_strip;
          fillbuffer2((uint32_t *)buff);
        }
    lasthandle2=__clock_cycles();
       Serial.printf("GetTime fillbuffer2 7 pins: %f us\n", (float)(lasthandle2 - lastHandle) / 240/nb);

        lastHandle = __clock_cycles();
        for (int i=0;i<nb;i++)
        {
          ledToDisplay=ledToDisplay%nun_led_per_strip;
          fillbuffer3((uint32_t *)buff);
        }
    lasthandle2=__clock_cycles();
       Serial.printf("GetTime fillbuffer3 8 pins: %f us\n", (float)(lasthandle2 - lastHandle) / 240/nb);

      
     uint8_t d[4]={255,1,0,0};
     uint32_t *f;
     f=(uint32_t*)d; 
     //(uint32_t*)d;
     
      Serial.printf("%ld %ld %d %d\n",*f,*f>>1,d[0],d[1]);
      *f=*f>>1;
      Serial.printf("%ld %ld %d %d\n",*f,*f>>1,d[0],d[1]);
      *f=*f>>1;
      Serial.printf("%ld %ld %d %d\n",*f,*f>>1,d[0],d[1]);
      CRGB l[1];
      l[0]=0xff0000;
      f=(uint32_t*)l;
      Serial.printf("%ld %#08x\n",*f,*f);
      
     
}

void loop() {
  // put your main code here, to run repeatedly:

}
