



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
#define NUM_VIRT_PINS 5 
#ifndef NBIS2SERIALPINS
#define NBIS2SERIALPINS 1
#endif

class I2S
{
  public:
	int i2sIndex;
	intr_handle_t interruptHandle;
	int nblinesperpin;
	int nbpins;
	int dmaBufferActive;
	DMABuffer **dmaBuffers;
	volatile bool stopSignal;
 volatile bool runningPixel=false;
	
	  //CRGB  pixelg[8][5]; //volatile uint8_t pixelg[8][5];
	   //uint8_t pixelg[16][8][4] ;
	//volatile uint8_t pixelr[8][5];
	//volatile uint8_t pixelb[8][5];
    int ledToDisplay;
CRGB *leds;
  int dmaBufferCount=2; //we use two buffers
   /* typedef union {
        uint8_t bytes[16];
        uint16_t shorts[8]; 
        uint32_t raw[2];
    } Lines;*/
	
	typedef union {
        uint8_t bytes[20];
        uint32_t shorts[8]; 
        uint32_t raw[2];
    } Lines;
  volatile  int num_strips;
  volatile  int nun_led_per_strip;
   // int *Pins;
    int brigthness;
    int ledType;

    
	/// hardware index [0, 1]
	I2S(const int i2sIndex = 0);
void setBrightness(uint8_t b)
    {
        this->brigthness=255/b;
    }

 void initled(CRGB *leds,int *Pins,int nbpins,int latchpin,int clockpin,int num_strips,int nun_led_per_strip,int ledType=1)
    {
        //initialization of the pins
		
		
		 Serial.println("init ready");
		this->nblinesperpin=5;
        dmaBufferCount=4; //we need one more buffer for the pause ...
        this->leds=leds;
        this->nun_led_per_strip=nun_led_per_strip;
        this->num_strips=num_strips;
      //  this->Pins=Pins;
        this->brigthness=2;
        this->runningPixel=false;
        this->ledType=ledType;
		this->nbpins=nbpins;
		Serial.printf("%d %d %d %d\n",Pins[0],Pins[1],Pins[2],Pins[3]);
		Serial.printf("nb pins %d %d %d\n",this->nbpins,sizeof(Pins),sizeof(*Pins));
        int pinMap[24];
        /*for(int i=0;i<24;i++)
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
        }*/
        
        this->dmaBufferCount=dmaBufferCount;
        this->dmaBuffers = (DMABuffer **)malloc(sizeof(DMABuffer *) * dmaBufferCount);
        if(!this->dmaBuffers)
        {
            Serial.println("Not enough memory soory...");
            return;
        }
		 Serial.println("init ready");
		//return;
        this->initParallelOutputMode(Pins,latchpin,clockpin);
        Serial.println("init ready");
		//return;
        for (int i = 0; i < this->dmaBufferCount; i++)
        {
            this->dmaBuffers[i] = DMABuffer::allocate((NUM_VIRT_PINS+1)*3*8*3); //(5+1)*3 pulses*24 bits
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

  
    
    void pu(uint32_t* buff)
    {
	memset((uint8_t*)buff,0,(NUM_VIRT_PINS+1)*8*3*4*3);
	for (int i=0;i<24*3;i++)
		{
		 buff[NUM_VIRT_PINS+i*(NUM_VIRT_PINS+1)]=0x80000000;
		}
    }

    void pu2(uint32_t* buff)
    {
	
	  
     for (int j=0;j<24;j++)
      {
      for (int i=0;i<NUM_VIRT_PINS;i++)
		{
		 *buff=0xFFFFF00;
		 buff++;
		}
		buff+=3*(NUM_VIRT_PINS+1)-NUM_VIRT_PINS; //13
      }
    }


   

  void transpose16x1_noinline2(unsigned char *A, uint8_t *B,uint8_t offset) {
    uint32_t  x, y, x1,y1,t;
    
   
    
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
	B-=offset;
    *((uint16_t*)(B)) = (uint16_t)(((y & 0xff00) |((y1&0xff00) <<8))>>8);
	B-=offset;
    *((uint16_t*)(B)) = (uint16_t)(((y & 0xff0000) |((y1&0xff0000) <<8))>>16);
	B-=offset;
    *((uint16_t*)(B)) = (uint16_t)(((y & 0xff000000) >>8 |((y1&0xff000000) ))>>16);
    B-=offset;
    *((uint16_t*)B) =(uint16_t)( (x & 0xff) |((x1&0xff) <<8));
    B-=offset;
	*((uint16_t*)(B)) = (uint16_t)(((x & 0xff00) |((x1&0xff00) <<8))>>8);
    B-=offset;
	*((uint16_t*)(B)) = (uint16_t)(((x & 0xff0000) |((x1&0xff0000) <<8))>>16);
    B-=offset;
	*((uint16_t*)(B)) = (uint16_t)(((x & 0xff000000) >>8 |((x1&0xff000000) ))>>16);
    
}

void transpose24x1_noinline(unsigned char *A, uint8_t *B,uint8_t offset) {

    uint32_t  x, y, x1,y1,t,x2,y2;

    

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

    

    

    

    *((uint32_t*)B) = (uint32_t)(((y & 0xff) |  (  (y1 & 0xff) << 8 )  |  (  (y2 & 0xff) << 16 ))<<8 )&0xfffff00   ;
	B-=offset;

    *((uint32_t*)(B)) = (uint32_t)(((y & 0xff00) |((y1&0xff00) <<8)  |((y2&0xff00) <<16)  )<<8  )&0xfffff00;
	B-=offset;

    *((uint32_t*)(B)) =(uint32_t)((  (  (y & 0xff0000) >>16)|((y1&0xff0000) >>8)   |((y2&0xff0000))   )<<8  )&0xfffff00;
	B-=offset;

    *((uint32_t*)(B)) = (uint32_t)(((y & 0xff000000) >>16 |((y1&0xff000000)>>8 ) |((y2&0xff000000) )  ))&0xfffff00;
	B-=offset;

    

    *((uint32_t*)B) =(uint32_t)(( (x & 0xff) |((x1&0xff) <<8)  |((x2&0xff) <<16))<<8 )&0xfffff00;
	B-=offset;

    *((uint32_t*)(B)) = (uint32_t)(((x & 0xff00) |((x1&0xff00) <<8)    |((x2&0xff00) <<16)    ))&0xfffff00;
	B-=offset;

    *((uint32_t*)(B)) = (uint32_t)( ( (  (x & 0xff0000) >>16)|((x1&0xff0000) >>8)   |((x2&0xff0000))   )<<8  )&0xfffff00;
	B-=offset;

    *((uint32_t*)(B)) = (uint32_t)(((x & 0xff000000) >>16 |((x1&0xff000000)>>8 )    |((x2&0xff000000) )    ))&0xfffff00;

    

}

void fillbuffer6(uint32_t *buff)
{
	Lines firstPixel[3];
       // Lines secondPixel[3];
		int nblines=5;
 
  int nbpins=20;//	this->nbpins;
  
  
   uint32_t l2=ledToDisplay;
	 uint32_t offset=(7)*(NUM_VIRT_PINS+1)*3+2*NUM_VIRT_PINS;
   for (int line=0;line<NUM_VIRT_PINS;line++){
   //uint32_t l=ledToDisplay+nun_led_per_strip*line;
     uint32_t l=l2;
	    for(int pin=0;pin<NBIS2SERIALPINS;pin++) {

	//uint32_t l=ledToDisplay+nun_led_per_strip*line+pin*nun_led_per_strip*5;
 
 
			firstPixel[0].bytes[pin] = leds[l].g/brigthness;
            firstPixel[1].bytes[pin] = leds[l].r/brigthness;
            firstPixel[2].bytes[pin] = leds[l].b/brigthness;
			l+=nun_led_per_strip*NUM_VIRT_PINS;


			}
			 l2+=nun_led_per_strip;
			 //putPixelinBuffer2(firstPixel,buff,line);
			/* transpose16x1_noinline2(pixel[0].bytes,(uint8_t*)&buff[(7)*18+10-line]+1,18*4);
        		transpose16x1_noinline2(pixel[1].bytes,(uint8_t*)&buff[(7+1*8)*18+10-line]+1,18*4);
        		transpose16x1_noinline2(pixel[2].bytes,(uint8_t*)&buff[(7+2*8)*18+10-line]+1,18*4);*/
			/*transpose16x1_noinline2(firstPixel[0].bytes,(uint8_t*)&buff[offset]+1,18*4);
        		transpose16x1_noinline2(firstPixel[1].bytes,(uint8_t*)&buff[offset+8*18]+1,18*4);
        		transpose16x1_noinline2(firstPixel[2].bytes,(uint8_t*)&buff[offset+16*18]+1,18*4);
				*/
			transpose24x1_noinline(firstPixel[0].bytes,(uint8_t*)&buff[offset],(NUM_VIRT_PINS+1)*3*4);
        		transpose24x1_noinline(firstPixel[1].bytes,(uint8_t*)&buff[offset+8*(NUM_VIRT_PINS+1)*3],(NUM_VIRT_PINS+1)*3*4);
        		transpose24x1_noinline(firstPixel[2].bytes,(uint8_t*)&buff[offset+16*(NUM_VIRT_PINS+1)*3],(NUM_VIRT_PINS+1)*3*4);		
				offset--;
				
		}
}

 


	void   putPixelinBuffer2(Lines *pixel,uint32_t *buf,int line)
    {
        //Lines b2,b,b3,b4;
		//Lines b;
		
		transpose16x1_noinline2(pixel[0].bytes,(uint8_t*)&buf[(7)*18+10-line]+1,18*4);
        transpose16x1_noinline2(pixel[1].bytes,(uint8_t*)&buf[(7+1*8)*18+10-line]+1,18*4);
        transpose16x1_noinline2(pixel[2].bytes,(uint8_t*)&buf[(7+2*8)*18+10-line]+1,18*4);
           
       /*
        for (int color=0;color<3;color++)
        {
			 b=pixel[color];         
			transpose16x1_noinline2(b.bytes,(uint8_t*)&buf[(7+color*8)*18+10-line]+1,18*4);
           
        }
		*/
    }

     void showPixels() {
        
        ledToDisplay=0;
        stopSignal=false;
 	
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
	bool initParallelOutputMode(const int *pinMap, int latchpin,int clockpin,long APLLFreq = 100000, int baseClock = -1, int wordSelect = -1);
	bool initParallelInputMode(const int *pinMap, long sampleRate = 100000, int baseClock = -1, int wordSelect = -1);

	void allocateDMABuffers(int count, int bytes);
	void deleteDMABuffers();
  
    
  protected:
	virtual void interrupt();
	
  private:
	static void IRAM_ATTR interrupt(void *arg);
};
