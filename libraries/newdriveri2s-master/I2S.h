/*
 Author Yves BAZIN
 change the Speed to adapt to 3.2 Mhz and 32 bits and all the functions to push the leds
	Author: bitluni 2019
	License: 
	Creative Commons Attribution ShareAlike 2.0
	https://creativecommons.org/licenses/by-sa/2.0/
	
	For further details check out: 
		https://youtube.com/bitlunislab
		https://github.com/bitluni
		http://bitluni.net
*/
#pragma once

#include "esp_heap_caps.h"
#include "soc/soc.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/io_mux_reg.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "rom/lldesc.h"
#include "DMABuffer.h"
#include "FastLed.h"


class I2S
{
  public:
	int i2sIndex;
	intr_handle_t interruptHandle;
	
	int dmaBufferActive;
	DMABuffer **dmaBuffers;
	volatile bool stopSignal;
 volatile bool runningPixel=false;
	
	  //CRGB  pixelg[8][5]; //volatile uint8_t pixelg[8][5];
	   //uint8_t pixelg[16][8][4] ;
	volatile uint8_t pixelr[8][5];
	volatile uint8_t pixelb[8][5];
    int ledToDisplay;
CRGB *leds;
  int dmaBufferCount=2; //we use two buffers
    typedef union {
        uint8_t bytes[16];
        uint16_t shorts[8];
        uint32_t raw[2];
    } Lines;
  volatile  int num_strips;
  volatile  int nun_led_per_strip;
    int *Pins;
    int brigthness;
    int ledType;

    
	/// hardware index [0, 1]
	I2S(const int i2sIndex = 0);
void setBrightness(uint8_t b)
    {
        this->brigthness=255/b;
    }

 void initled(CRGB *leds,int * Pins,int num_strips,int nun_led_per_strip,int ledType=1)
    {
        //initialization of the pins
		 Serial.println("init ready");
		
        dmaBufferCount=4; //we need one more buffer for the pause ...
        this->leds=leds;
        this->nun_led_per_strip=nun_led_per_strip;
        this->num_strips=num_strips;
        this->Pins=Pins;
        this->brigthness=2;
        this->runningPixel=false;
        this->ledType=ledType;
        int pinMap[24];
        for(int i=0;i<24;i++)
        {
            if(i>=24)
            {
                pinMap[i]=-1;
            }
            else
            {
                if(this->Pins[i]>=0 && this->Pins[i]<=33)
                    pinMap[i]=this->Pins[i]; //we could add remove 6,7,8,9,10,11,12
                else
                    pinMap[i]=-1;
            }
        }
        
        this->dmaBufferCount=dmaBufferCount;
        this->dmaBuffers = (DMABuffer **)malloc(sizeof(DMABuffer *) * dmaBufferCount);
        if(!this->dmaBuffers)
        {
            Serial.println("Not enough memory soory...");
            return;
        }
		 Serial.println("init ready");
		//return;
        this->initParallelOutputMode(pinMap);
        Serial.println("init ready");
		//return;
        for (int i = 0; i < this->dmaBufferCount; i++)
        {
            this->dmaBuffers[i] = DMABuffer::allocate(6*3*8*3); //(5+1)*3 pulses*24 bits
           // if (i)
             //   this->dmaBuffers[i - 1]->next(this->dmaBuffers[i]);
           // pu((uint32_t*)this->dmaBuffers[i]->buffer); //we do get the buffer prefilled with the 0 at the end and the 1
        }
		Serial.println("init ready");
		//return;
       // this->dmaBuffers[dmaBufferCount - 1]->next(this->dmaBuffers[0]);
        pu((uint32_t*)this->dmaBuffers[0]->buffer); //latch
		pu((uint32_t*)this->dmaBuffers[1]->buffer);
		//pu((uint32_t*)this->dmaBuffers[2]->buffer);
		pu2((uint32_t*)this->dmaBuffers[0]->buffer); //first pulse
		pu2((uint32_t*)this->dmaBuffers[1]->buffer);
        Serial.println("Controller ready");
    }

   void empty ( uint32_t *buf)
    {
        for(int i=0;i<24;i++)
        {
            buf[4*i]=0xffffffff;
            buf[4*i+3]=0;
        }
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
    
    void pu(uint32_t* buff)
    {
	memset((uint8_t*)buff,0,6*3*8*3*4);
	for (int i=0;i<24*3;i++)
		{
		 buff[5+i*6]=0x80000000;
		}
    }

    void pu2(uint32_t* buff)
    {
      for (int j=0;j<24;j++)
      {
      for (int i=0;i<5;i++)
		{
		 buff[i+j*6*3]=0xFFFF00;
		 
		}
//buff[j*6*3+6]=0x200;
      }
     
    }


   

  

void transpose16x1_noinline(unsigned char *A, uint16_t *B) {
    uint32_t  x, y, x1,y1,t;
    
    // Load the array and pack it into x and y.
    /*  y = (*(unsigned char*)(A) & 0xffff ) |  ((*(unsigned char*)(A+4) & 0xffffL )<<16) ;
     x = (*(unsigned char*)(A+8)& 0xffff ) |  ((*(unsigned char*)(A+12) & 0xffffL )<<16) ;
     y1 = (*(unsigned char*)(A+2)& 0xffff ) |  ((*(unsigned char*)(A+6) & 0xffffL )<<16);
     x1 = (*(unsigned char*)(A+10)& 0xffff )| ((*(unsigned char*)(A+14) & 0xffffL )<<16);*/
    //printf("%d\n",*(unsigned int*)(A+4));
    
    
    y = *(unsigned int*)(A);
    x = *(unsigned int*)(A+4);
    y1 = *(unsigned int*)(A+8);
    x1 = *(unsigned int*)(A+12);
    
    
    
    
    // pre-transform x
    t = (x ^ (x >> 7)) & 0x00AA00AA;  x = x ^ t ^ (t << 7);
    t = (x ^ (x >>14)) & 0x0000CCCC;  x = x ^ t ^ (t <<14);
    t = (x1 ^ (x1 >> 7)) & 0x00AA00AA;  x1 = x1 ^ t ^ (t << 7);
    t = (x1 ^ (x1 >>14)) & 0x0000CCCC;  x1 = x1 ^ t ^ (t <<14);
    // pre-transform y
    t = (y ^ (y >> 7)) & 0x00AA00AA;  y = y ^ t ^ (t << 7);
    t = (y ^ (y >>14)) & 0x0000CCCC;  y = y ^ t ^ (t <<14);
    t = (y1 ^ (y1 >> 7)) & 0x00AA00AA;  y1 = y1 ^ t ^ (t << 7);
    t = (y1 ^ (y1 >>14)) & 0x0000CCCC;  y1 = y1 ^ t ^ (t <<14);
    
    
    // final transform
    t = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
    y = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);
    x = t;
    
    t = (x1 & 0xF0F0F0F0) | ((y1 >> 4) & 0x0F0F0F0F);
    y1 = ((x1 << 4) & 0xF0F0F0F0) | (y1 & 0x0F0F0F0F);
    x1 = t;
    
    
    
    *((uint16_t*)B) = (uint16_t)((y & 0xff) |  (  (y1 & 0xff) << 8 ) )   ;
    *((uint16_t*)(B+1)) = (uint16_t)(((y & 0xff00) |((y1&0xff00) <<8))>>8);
    *((uint16_t*)(B+2)) = (uint16_t)(((y & 0xff0000) |((y1&0xff0000) <<8))>>16);
    *((uint16_t*)(B+3)) = (uint16_t)(((y & 0xff000000) >>8 |((y1&0xff000000) ))>>16);
    
    *((uint16_t*)B+4) =(uint16_t)( (x & 0xff) |((x1&0xff) <<8));
    *((uint16_t*)(B+5)) = (uint16_t)(((x & 0xff00) |((x1&0xff00) <<8))>>8);
    *((uint16_t*)(B+6)) = (uint16_t)(((x & 0xff0000) |((x1&0xff0000) <<8))>>16);
    *((uint16_t*)(B+7)) = (uint16_t)(((x & 0xff000000) >>8 |((x1&0xff000000) ))>>16);
    
}


void fillbuffer6(uint32_t *buff)
{
	Lines firstPixel[3];
        Lines secondPixel[3];
		int nblines=5;
  int nbbits=8;
  int nbpins=16	;
  int lowerpins;
  
  if(nbpins>8)
  
  	lowerpins=8;
	else
	lowerpins=nbpins;
	
   for (int line=0;line<nblines;line++){
   uint32_t l=ledToDisplay+nun_led_per_strip*line;
        for(int pin=0;pin<nbpins;pin++) {

	//uint32_t l=ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5;
 
 
			firstPixel[0].bytes[pin] = leds[l].g/brigthness;
            firstPixel[1].bytes[pin] = leds[l].r/brigthness;
            firstPixel[2].bytes[pin] = leds[l].b/brigthness;
			l+=nun_led_per_strip*5;

/*
if(nbpins>(pin+8))
{
		uint32_t l=ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5;
 
 
			secondPixel[0].bytes[pin] = leds[l].g/brigthness;
            secondPixel[1].bytes[pin] = leds[l].r/brigthness;
            secondPixel[2].bytes[pin] = leds[l].b/brigthness;
			}
			*/
			}
			 putPixelinBuffer(firstPixel,secondPixel,buff,line);
		}
}

 


	void transpose8x1(unsigned char *A, unsigned char *B) {
  uint32_t x, y, t;

  // Load the array and pack it into x and y.
  y = *(unsigned int*)(A);
  x = *(unsigned int*)(A+4);

  // pre-transform x
  t = (x ^ (x >> 7)) & 0x00AA00AA;  x = x ^ t ^ (t << 7);
  t = (x ^ (x >>14)) & 0x0000CCCC;  x = x ^ t ^ (t <<14);

  // pre-transform y
  t = (y ^ (y >> 7)) & 0x00AA00AA;  y = y ^ t ^ (t << 7);
  t = (y ^ (y >>14)) & 0x0000CCCC;  y = y ^ t ^ (t <<14);

  // final transform
  t = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
  y = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);
  x = t;

  *((uint32_t*)B) = y;
  *((uint32_t*)(B+4)) = x;
}
	
    void   putPixelinBuffer(Lines *pixel,Lines *pixel2,uint32_t *buf,int line)
    {
        Lines b2,b,b3,b4;
        
        for (int color=0;color<3;color++)
        {
		 //b=pixel[color];
           // b2=b;
            //transpose8x1(b.bytes,b2.shorts);
           
			// b4=pixel2[color];
            //b3=b4;
           //transpose8x1(b4.bytes,b3.shorts);
			 b=pixel[color];
            b2=b;
			transpose16x1_noinline(b.bytes,b2.shorts);
            //transpose8x1(b.bytes,b2.shorts);
            for(int bits=0;bits<8;bits++)
            {
			//uint8_t m=b2.shorts[bits];
			//*((uint8_t*)buf+((8-1-bits+color*8)*18+5-1-line+6)*4+2)=b3.shorts[bits];//b2.shorts[bits];
			//*((uint8_t*)buf+((8-1-bits+color*8)*18+5-1-line+6)*4+1)=b2.shorts[bits];
			//Serial.printf("%#08x\n",((uint32_t)b2.shorts[bits] << 16) & 0xFF0000);
			//uint32_t m=(b2.shorts[bits] ) + (b2.shorts[bits] << 8);
			//if(bits%4==0)
			 buf[(8-1-bits+color*8)*18+5-1-line+6]= b2.shorts[bits] <<8;
			 //else
			 // buf[(8-1-bits+color*8)*18+5-1-line+6]= b2.shorts[bits];
			  
			 // *((uint8_t*)buf+((8-1-bits+color*8)*18+5-1-line+6)*4+1)=*(uint8_t*)&b2.shorts[bits];
			  //*((uint8_t*)buf+((8-1-bits+color*8)*18+5-1-line+6)*4+2)=*(uint8_t*)(&b2.shorts[bits]+1);
			// memcpy( (uint8_t*)buf+((8-1-bits+color*8)*18+5-1-line+6)*4+1,&b2.shorts[bits],2);
			 //((b2.shorts[bits] <<8) & 0xFF00)
               // buf[color*32+4*i+1]=(b2.shorts[7-i] << 8); //the <<8 is to free up the first byte
                //buf[color*32+4*i+2]=ledType*(b2.shorts[7-i] << 8);
            }
        }
    }

     void showPixels() {
        
        ledToDisplay=0;
        stopSignal=false;
        //pixelsToDisplay(allpixels);
       /* Lines firstPixel[3];
        Lines secondPixel[3];
        for(int i = 0; i < num_strips; i++) {
            firstPixel[0].bytes[i] = leds[ledToDisplay+nun_led_per_strip*i].g/brigthness;
            firstPixel[1].bytes[i] = leds[ledToDisplay+nun_led_per_strip*i].r/brigthness;
            firstPixel[2].bytes[i] = leds[ledToDisplay+nun_led_per_strip*i].b/brigthness;
        }
        
        
        ledToDisplay++;
        putPixelinBuffer(firstPixel,(uint32_t*)dmaBuffers[0]->buffer);
        
        for(int i = 0; i < num_strips; i++) {
            
            secondPixel[0].bytes[i] = leds[ledToDisplay+nun_led_per_strip*i].g/brigthness;
            secondPixel[1].bytes[i] = leds[ledToDisplay+nun_led_per_strip*i].r/brigthness;
            secondPixel[2].bytes[i] = leds[ledToDisplay+nun_led_per_strip*i].b/brigthness;
            
        }
        
        
        ledToDisplay++;
        putPixelinBuffer(secondPixel,(uint32_t*)dmaBuffers[1]->buffer);*/
		
		pu((uint32_t*)this->dmaBuffers[0]->buffer); //latch
		pu((uint32_t*)this->dmaBuffers[1]->buffer);
		pu((uint32_t*)this->dmaBuffers[2]->buffer);
		//pu((uint32_t*)this->dmaBuffers[3]->buffer);
		pu2((uint32_t*)this->dmaBuffers[0]->buffer); //first pulse
		pu2((uint32_t*)this->dmaBuffers[1]->buffer);
		//pu2((uint32_t*)this->dmaBuffers[2]->buffer);
		fillbuffer6((uint32_t*)dmaBuffers[0]->buffer);
		 ledToDisplay++;
		//fillbuffer2((uint32_t*)dmaBuffers[1]->buffer);
		 //ledToDisplay++;

        dmaBufferActive=1;
		// this->dmaBuffers[3]->next(this->dmaBuffers[0]);
		 this->dmaBuffers[1]->next(this->dmaBuffers[0]);
		 this->dmaBuffers[2]->next(this->dmaBuffers[0]);
		  this->dmaBuffers[0]->next(this->dmaBuffers[1]); //on utilise le dernier buffer
        runningPixel=true;
        startTX();
        while(runningPixel==true);
         //delay(0);
        //Serial.println("pixel done");
        
        
    }

    void IRAM_ATTR  callback()
    {
	/*
	i2sStop();
return;*/
	/*if(stopSignal)
	{
		i2sStop();
		return;
		}
		 dmaBufferActive = (dmaBufferActive + 1)% dmaBufferCount;
		 if(dmaBufferActive==3)
		 stopSignal=true;
	return;*/
	//Serial.printf("display %d\n",(dmaBufferActive+1)%2);
	//Serial.printf("charge %d\n",ledToDisplay);
	
        Lines pixel[3];
        
        if(stopSignal)
        {
            //delay(0);
            i2sStop();
            runningPixel=false;
            return;
        }
        if(ledToDisplay<=nun_led_per_strip)
        {
            
            
            
            
            
			
			if(ledToDisplay==nun_led_per_strip)
			{
				pu( (uint32_t*)this->dmaBuffers[dmaBufferActive]->buffer);

				            stopSignal=true;
			}
			fillbuffer6((uint32_t*)dmaBuffers[dmaBufferActive]->buffer);
			ledToDisplay++;
            dmaBufferActive = (dmaBufferActive + 1)% 2;
			//if(ledToDisplay)
        }
        else
        {
            //if no more pixels then we will read the other buffer and stop
           // if(ledToDisplay==nun_led_per_strip)
             //   ledToDisplay++;
            //if(ledToDisplay==nun_led_per_strip+1)
                stopSignal=true;
        }
    }
    
    
    
    

	void reset();

	void stop();

	void i2sStop();
	void startTX();
	void startRX();
  
	void resetDMA();
	void resetFIFO();
	bool initParallelOutputMode(const int *pinMap, long APLLFreq = 100000, int baseClock = -1, int wordSelect = -1);
	bool initParallelInputMode(const int *pinMap, long sampleRate = 100000, int baseClock = -1, int wordSelect = -1);

	void allocateDMABuffers(int count, int bytes);
	void deleteDMABuffers();
  
    
  protected:
	virtual void interrupt();
	
  private:
	static void IRAM_ATTR interrupt(void *arg);
};
