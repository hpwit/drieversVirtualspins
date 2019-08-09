/*
 * I2S Driver
 *
 * 
 */

#pragma once

#pragma message "NOTE: ESP32 support using I2S parallel driver. All strips must use the same chipset"

FASTLED_NAMESPACE_BEGIN

#ifdef __cplusplus
extern "C" {
#endif
    
#include "esp_heap_caps.h"
#include "soc/soc.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/io_mux_reg.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "rom/lldesc.h"
#include "esp_intr.h"
#include "esp_log.h"
#include <soc/rtc.h>

    
#ifdef __cplusplus
}
#endif

__attribute__ ((always_inline)) inline static uint32_t __clock_cycles() {
    uint32_t cyc;
    __asm__ __volatile__ ("rsr %0,ccount":"=a" (cyc));
    return cyc;
}


#define NUM_COLOR_CHANNELS 3

// -- Choose which I2S device to use
#ifndef I2S_DEVICE
#define I2S_DEVICE 0
#endif

// -- Max number of controllers we can support
#ifndef FASTLED_I2S_MAX_CONTROLLERS
#define FASTLED_I2S_MAX_CONTROLLERS 24
#endif

// -- I2S clock
#define I2S_BASE_CLK (80000000L)
#define I2S_MAX_CLK (20000000L) //more tha a certain speed and the I2s looses some bits
#define I2S_MAX_PULSE_PER_BIT 20 //put it higher to get more accuracy but it could decrease the refresh rate without real improvement
// -- Convert ESP32 cycles back into nanoseconds
#define ESPCLKS_TO_NS(_CLKS) (((long)(_CLKS) * 1000L) / F_CPU_MHZ)
#define NUM_VIRT_PINS 5 
#ifndef NBIS2SERIALPINS
#define NBIS2SERIALPINS 1
#endif
// -- Array of all controllers
//static CLEDController * gControllers[FASTLED_I2S_MAX_CONTROLLERS];
static int gNumControllers = 0;
static int gNumStarted = 0;

// -- Global semaphore for the whole show process
//    Semaphore is not given until all data has been sent
static xSemaphoreHandle gTX_sem = NULL;

// -- One-time I2S initialization
static bool gInitialized = false;

// -- Interrupt handler
static intr_handle_t gI2S_intr_handle = NULL;

// -- A pointer to the memory-mapped structure: I2S0 or I2S1
static i2s_dev_t * i2s;

// -- I2S goes to these pins until we remap them using the GPIO matrix
static int i2s_base_pin_index;

// --- I2S DMA buffers
struct DMABuffer {
    lldesc_t descriptor;
    uint8_t * buffer;
};

#define NUM_DMA_BUFFERS 4
static DMABuffer * dmaBuffers[NUM_DMA_BUFFERS];

// -- Bit patterns
//    For now, we require all strips to be the same chipset, so these
//    are global variables.

static int      gPulsesPerBit = 0;
static uint32_t gOneBit[40] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static uint32_t gZeroBit[40]  = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

// -- Counters to track progress
static int gCurBuffer = 0;
static bool gDoneFilling = false;
static int ones_for_one;
static int ones_for_zero;

// -- Temp buffers for pixels and bits being formatted for DMA
static uint8_t gPixelRow[NUM_COLOR_CHANNELS][32];
static uint8_t gPixelBits[NUM_COLOR_CHANNELS][8][4];
static int CLOCK_DIVIDER_N;
static int CLOCK_DIVIDER_A;
static int CLOCK_DIVIDER_B;
static int dmaBufferActive;
	
	static  bool stopSignal;
 static  bool runningPixel=false;
	
	  //CRGB  pixelg[8][5]; //volatile uint8_t pixelg[8][5];
	   //uint8_t pixelg[16][8][4] ;
	//volatile uint8_t pixelr[8][5];
	//volatile uint8_t pixelb[8][5];
  static  int ledToDisplay;
//CRGB *int_leds;
 static  int dmaBufferCount=2; //we use two buffers
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
  static volatile  int num_strips;
 static volatile  int nun_led_per_strip;
   // int *Pins;
  static  int brigthness;
    static int ledType;
	 static CRGB *int_leds;
//template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER = RGB, int XTRA0 = 0, bool FLIP = false, int WAIT_TIME = 5>
template<int *Pins,int CLOCK_PIN,int LATCH_PIN,int NUM_LED_PER_STRIP, EOrder RGB_ORDER = GRB>
class ClocklessController : public CPixelLEDController<RGB_ORDER>
{
   
  //int *Pins;
   	const int deviceBaseIndex[2] = {I2S0O_DATA_OUT0_IDX, I2S1O_DATA_OUT0_IDX};
	const int deviceClockIndex[2] = {I2S0O_BCK_OUT_IDX, I2S1O_BCK_OUT_IDX};
	const int deviceWordSelectIndex[2] = {I2S0O_WS_OUT_IDX, I2S1O_WS_OUT_IDX};
	const periph_module_t deviceModule[2] = {PERIPH_I2S0_MODULE, PERIPH_I2S1_MODULE};
public:

    void init()
    {
        i2sInit();
      
	  nun_led_per_strip=  NUM_LED_PER_STRIP;
	  for (int i = 0; i < NBIS2SERIALPINS; i++)
		if (Pins[i] > -1)
		{
			PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[Pins[i]], PIN_FUNC_GPIO);
			gpio_set_direction((gpio_num_t)Pins[i], (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
     pinMode(Pins[i],OUTPUT);
			gpio_matrix_out(Pins[i], deviceBaseIndex[I2S_DEVICE] + i, false, false);
		}
		
		//latch pin
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[LATCH_PIN], PIN_FUNC_GPIO);
			gpio_set_direction((gpio_num_t)LATCH_PIN, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
     pinMode(LATCH_PIN,OUTPUT);
			gpio_matrix_out(LATCH_PIN, deviceBaseIndex[I2S_DEVICE] + 23, false, false);
	//if (baseClock > -1)
	//clock pin
		gpio_matrix_out(CLOCK_PIN, deviceClockIndex[I2S_DEVICE], false, false);
       
      
    }
    
    virtual uint16_t getMaxRefreshRate() const { return 800; }
    
protected:
   
  
    
    static DMABuffer * allocateDMABuffer(int bytes)
    {
        DMABuffer * b = (DMABuffer *)heap_caps_malloc(sizeof(DMABuffer), MALLOC_CAP_DMA);
        
        b->buffer = (uint8_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA);
        memset(b->buffer, 0, bytes);
        
        b->descriptor.length = bytes;
        b->descriptor.size = bytes;
        b->descriptor.owner = 1;
        b->descriptor.sosf = 1;
        b->descriptor.buf = b->buffer;
        b->descriptor.offset = 0;
        b->descriptor.empty = 0;
        b->descriptor.eof = 1;
        b->descriptor.qe.stqe_next = 0;
        
        return b;
    }
    
    static void i2sInit()
    {
        // -- Only need to do this once
       // if (gInitialized) return;
        
        // -- Construct the bit patterns for ones and zeros
     //   initBitPatterns();
        
        // -- Choose whether to use I2S device 0 or device 1
        //    Set up the various device-specific parameters
        int interruptSource;
        if (I2S_DEVICE == 0) {
            i2s = &I2S0;
            periph_module_enable(PERIPH_I2S0_MODULE);
            interruptSource = ETS_I2S0_INTR_SOURCE;
            i2s_base_pin_index = I2S0O_DATA_OUT0_IDX;
        } else {
            i2s = &I2S1;
            periph_module_enable(PERIPH_I2S1_MODULE);
            interruptSource = ETS_I2S1_INTR_SOURCE;
            i2s_base_pin_index = I2S1O_DATA_OUT0_IDX;
        }
        
        // -- Reset everything
        i2sReset();
        i2sReset_DMA();
        i2sReset_FIFO();
        
        // -- Main configuration
        i2s->conf.tx_msb_right = 1;
        i2s->conf.tx_mono = 0;
        i2s->conf.tx_short_sync = 0;
        i2s->conf.tx_msb_shift = 0;
        i2s->conf.tx_right_first = 1; // 0;//1;
        i2s->conf.tx_slave_mod = 0;
        
        // -- Set parallel mode
        i2s->conf2.val = 0;
        i2s->conf2.lcd_en = 1;
        i2s->conf2.lcd_tx_wrx2_en = 0; // 0 for 16 or 32 parallel output
        i2s->conf2.lcd_tx_sdx2_en = 0; // HN
        
        // -- Set up the clock rate and sampling
        i2s->sample_rate_conf.val = 0;
        i2s->sample_rate_conf.tx_bits_mod = 32; // Number of parallel bits/pins
        i2s->sample_rate_conf.tx_bck_div_num = 1;
        i2s->clkm_conf.val = 0;
        i2s->clkm_conf.clka_en = 1;
        
		rtc_clk_apll_enable(true, 215, 163,4, 1);
        // -- Data clock is computed as Base/(div_num + (div_b/div_a))
        //    Base is 80Mhz, so 80/(10 + 0/1) = 8Mhz
        //    One cycle is 125ns
        i2s->clkm_conf.clkm_div_a =1;// CLOCK_DIVIDER_A;
        i2s->clkm_conf.clkm_div_b = 0;//CLOCK_DIVIDER_B;
        i2s->clkm_conf.clkm_div_num = 1;//CLOCK_DIVIDER_N;
        
        i2s->fifo_conf.val = 0;
        i2s->fifo_conf.tx_fifo_mod_force_en = 1;
        i2s->fifo_conf.tx_fifo_mod = 3;  // 32-bit single channel data
        i2s->fifo_conf.tx_data_num = 32; // fifo length
        i2s->fifo_conf.dscr_en = 1;      // fifo will use dma
        
        i2s->conf1.val = 0;
        i2s->conf1.tx_stop_en = 0;
        i2s->conf1.tx_pcm_bypass = 1;
        
        i2s->conf_chan.val = 0;
        i2s->conf_chan.tx_chan_mod = 1; // Mono mode, with tx_msb_right = 1, everything goes to right-channel
        
        i2s->timing.val = 0;
        
        // -- Allocate two DMA buffers
        dmaBuffers[0] = allocateDMABuffer((NUM_VIRT_PINS+1)*3*8*3);
        dmaBuffers[1] = allocateDMABuffer((NUM_VIRT_PINS+1)*3*8*3);
        dmaBuffers[2] = allocateDMABuffer((NUM_VIRT_PINS+1)*3*8*3);
		
        // -- Arrange them as a circularly linked list
        dmaBuffers[0]->descriptor.qe.stqe_next = &(dmaBuffers[1]->descriptor);
        dmaBuffers[1]->descriptor.qe.stqe_next = &(dmaBuffers[0]->descriptor);
        pu((uint32_t*)dmaBuffers[0]->buffer); //latch
		pu((uint32_t*)dmaBuffers[1]->buffer);
		//pu((uint32_t*)this->dmaBuffers[2]->buffer);
		pu2((uint32_t*)dmaBuffers[0]->buffer); //first pulse
		pu2((uint32_t*)dmaBuffers[1]->buffer);
	   
        // -- Allocate i2s interrupt
        SET_PERI_REG_BITS(I2S_INT_ENA_REG(I2S_DEVICE), I2S_OUT_EOF_INT_ENA_V, 1, I2S_OUT_EOF_INT_ENA_S);
        esp_err_t e = esp_intr_alloc(interruptSource, 0, // ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_LEVEL3,
                                     &interruptHandler, 0, &gI2S_intr_handle);
        
        // -- Create a semaphore to block execution until all the controllers are done
        if (gTX_sem == NULL) {
            gTX_sem = xSemaphoreCreateBinary();
            xSemaphoreGive(gTX_sem);
        }
        
        // Serial.println("Init I2S");
        gInitialized = true;
    }
    
	static void pu(uint32_t* buff)
    {
	memset((uint8_t*)buff,0,(NUM_VIRT_PINS+1)*8*3*4*3);
	for (int i=0;i<24*3;i++)
		{
		 buff[NUM_VIRT_PINS+i*(NUM_VIRT_PINS+1)]=0x80000000;
		}
    }

   static void pu2(uint32_t* buff)
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
	
    /** Clear DMA buffer
     *
     *  Yves' clever trick: initialize the bits that we know must be 0
     *  or 1 regardless of what bit they encode.
     */
    static void empty( uint32_t *buf)
    {
        for(int i=0;i<8*NUM_COLOR_CHANNELS;i++)
        {
            int offset=gPulsesPerBit*i;
            for(int j=0;j<ones_for_zero;j++)
                buf[offset+j]=0xffffffff;
            
            for(int j=ones_for_one;j<gPulsesPerBit;j++)
                buf[offset+j]=0;
        }
    }
    
    // -- Show pixels
    //    This is the main entry point for the controller.
    virtual void showPixels(PixelController<RGB_ORDER> & pixels)
    {
        
			int_leds=(CRGB*)pixels.mData;
                ledToDisplay=0;
        stopSignal=false;
 	
		pu((uint32_t*)dmaBuffers[0]->buffer); //latch
		pu((uint32_t*)dmaBuffers[1]->buffer);
		pu((uint32_t*)dmaBuffers[2]->buffer);
		//pu((uint32_t*)this->dmaBuffers[3]->buffer);
		pu2((uint32_t*)dmaBuffers[0]->buffer); //first pulse
		pu2((uint32_t*)dmaBuffers[1]->buffer);
		//pu2((uint32_t*)this->dmaBuffers[2]->buffer);
		fillbuffer6((uint32_t*)dmaBuffers[0]->buffer);
		 ledToDisplay++;
		//fillbuffer2((uint32_t*)dmaBuffers[1]->buffer);
		 //ledToDisplay++;

        dmaBufferActive=1;
		// this->dmaBuffers[3]->next(this->dmaBuffers[0]);
		/*
		 dmaBuffers[1]->next(this->dmaBuffers[0]);
		 dmaBuffers[2]->next(this->dmaBuffers[0]);
		 dmaBuffers[0]->next(this->dmaBuffers[1]); //on utilise le dernier buffer*/
		 dmaBuffers[1]->descriptor.qe.stqe_next = &(dmaBuffers[0]->descriptor);
        dmaBuffers[2]->descriptor.qe.stqe_next = &(dmaBuffers[0]->descriptor);
		dmaBuffers[0]->descriptor.qe.stqe_next = &(dmaBuffers[1]->descriptor);
        runningPixel=true;
        //startTX();
		i2sStart();
        while(runningPixel==true);
    }
    
    // -- Custom interrupt handler
    static IRAM_ATTR void interruptHandler(void *arg)
    {
	
	/*
        if (i2s->int_st.out_eof) {
            i2s->int_clr.val = i2s->int_raw.val;
            
            if ( ! gDoneFilling) {
                fillBuffer6();
            } else {
                portBASE_TYPE HPTaskAwoken = 0;
                xSemaphoreGiveFromISR(gTX_sem, &HPTaskAwoken);
                if(HPTaskAwoken == pdTRUE) portYIELD_FROM_ISR();
            }
        }*/
		
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
				pu( (uint32_t*)dmaBuffers[dmaBufferActive]->buffer);

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
    
static void transpose24x1_noinline(unsigned char *A, uint8_t *B,uint8_t offset) {

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

static void fillbuffer6(uint32_t *buff)
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
 
 
			firstPixel[0].bytes[pin] = int_leds[l].g/brigthness;
            firstPixel[1].bytes[pin] = int_leds[l].r/brigthness;
            firstPixel[2].bytes[pin] = int_leds[l].b/brigthness;
			l+=nun_led_per_strip*NUM_VIRT_PINS;


			}
			 l2+=nun_led_per_strip;
			 
			transpose24x1_noinline(firstPixel[0].bytes,(uint8_t*)&buff[offset],(NUM_VIRT_PINS+1)*3*4);
        		transpose24x1_noinline(firstPixel[1].bytes,(uint8_t*)&buff[offset+8*(NUM_VIRT_PINS+1)*3],(NUM_VIRT_PINS+1)*3*4);
        		transpose24x1_noinline(firstPixel[2].bytes,(uint8_t*)&buff[offset+16*(NUM_VIRT_PINS+1)*3],(NUM_VIRT_PINS+1)*3*4);		
				offset--;
				
		}
}

    
    /** Start I2S transmission
     */
    static void i2sStart()
    {
        // esp_intr_disable(gI2S_intr_handle);
        // Serial.println("I2S start");
        i2sReset();
        //Serial.println(dmaBuffers[0]->sampleCount());
        i2s->lc_conf.val=I2S_OUT_DATA_BURST_EN | I2S_OUTDSCR_BURST_EN | I2S_OUT_DATA_BURST_EN;
        i2s->out_link.addr = (uint32_t) & (dmaBuffers[2]->descriptor);
        i2s->out_link.start = 1;
        ////vTaskDelay(5);
        i2s->int_clr.val = i2s->int_raw.val;
        // //vTaskDelay(5);
        i2s->int_ena.out_dscr_err = 1;
        //enable interrupt
        ////vTaskDelay(5);
        esp_intr_enable(gI2S_intr_handle);
        // //vTaskDelay(5);
        i2s->int_ena.val = 0;
        i2s->int_ena.out_eof = 1;
        
        //start transmission
        i2s->conf.tx_start = 1;
    }
    
    static void i2sReset()
    {
        // Serial.println("I2S reset");
        const unsigned long lc_conf_reset_flags = I2S_IN_RST_M | I2S_OUT_RST_M | I2S_AHBM_RST_M | I2S_AHBM_FIFO_RST_M;
        i2s->lc_conf.val |= lc_conf_reset_flags;
        i2s->lc_conf.val &= ~lc_conf_reset_flags;
        
        const uint32_t conf_reset_flags = I2S_RX_RESET_M | I2S_RX_FIFO_RESET_M | I2S_TX_RESET_M | I2S_TX_FIFO_RESET_M;
        i2s->conf.val |= conf_reset_flags;
        i2s->conf.val &= ~conf_reset_flags;
    }
    
    static void i2sReset_DMA()
    {
        i2s->lc_conf.in_rst=1; i2s->lc_conf.in_rst=0;
        i2s->lc_conf.out_rst=1; i2s->lc_conf.out_rst=0;
    }
    
    static void i2sReset_FIFO()
    {
        i2s->conf.rx_fifo_reset=1; i2s->conf.rx_fifo_reset=0;
        i2s->conf.tx_fifo_reset=1; i2s->conf.tx_fifo_reset=0;
    }
    
    static void i2sStop()
    {
        // Serial.println("I2S stop");
        esp_intr_disable(gI2S_intr_handle);
        i2sReset();
        i2s->conf.rx_start = 0;
        i2s->conf.tx_start = 0;
    }
};

FASTLED_NAMESPACE_END
