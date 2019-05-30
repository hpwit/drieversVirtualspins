/*
 * I2S Driver
 *
 * Copyright (c) 2019 Yves Bazin
 * Copyright (c) 2019 Samuel Z. Guyer
 * Derived from lots of code examples from other people.
 *
 * The I2S implementation can drive up to 24 strips in parallel, but
 * with the following limitation: all the strips must have the same
 * timing (i.e., they must all use the same chip).
 *
 * To enable the I2S driver, add the following line *before* including
 * FastLED.h (no other changes are necessary):
 *
 * #define FASTLED_ESP32_I2S true
 *
 * The overall strategy is to use the parallel mode of the I2S "audio"
 * peripheral to send up to 24 bits in parallel to 24 different pins.
 * Unlike the RMT peripheral the I2S system cannot send bits of
 * different lengths. Instead, we set the I2S data clock fairly high
 * and then encode a signal as a series of bits. 
 *
 * For example, with a clock divider of 10 the data clock will be
 * 8MHz, so each bit is 125ns. The WS2812 expects a "1" bit to be
 * encoded as a HIGH signal for around 875ns, followed by LOW for
 * 375ns. Sending the following pattern results in the right shape
 * signal:
 *
 *    1111111000        WS2812 "1" bit encoded as 10 125ns pulses
 *
 * The I2S peripheral expects the bits for all 24 outputs to be packed
 * into a single 32-bit word. The complete signal is a series of these
 * 32-bit values -- one for each bit for each strip. The pixel data,
 * however, is stored "serially" as a series of RGB values separately
 * for each strip. To prepare the data we need to do three things: (1)
 * take 1 pixel from each strip, and (2) tranpose the bits so that
 * they are in the parallel form, (3) translate each data bit into the
 * bit pattern that encodes the signal for that bit. This code is in
 * the fillBuffer() method:
 *
 *   1. Read 1 pixel from each strip into an array; store this data by
 *      color channel (e.g., all the red bytes, then all the green
 *      bytes, then all the blue bytes). For three color channels, the
 *      array is 3 X 24 X 8 bits.
 *
 *   2. Tranpose the array so that it is 3 X 8 X 24 bits. The hardware
 *      wants the data in 32-bit chunks, so the actual form is 3 X 8 X
 *      32, with the low 8 bits unused.
 *
 *   3. Take each group of 24 parallel bits and "expand" them into a
 *      pattern according to the encoding. For example, with a 8MHz
 *      data clock, each data bit turns into 10 I2s pulses, so 24
 *      parallel data bits turn into 10 X 24 pulses.
 *
 * We send data to the I2S peripheral using the DMA interface. We use
 * two DMA buffers, so that we can fill one buffer while the other
 * buffer is being sent. Each DMA buffer holds the fully-expanded
 * pulse pattern for one pixel on up to 24 strips. The exact amount of
 * memory required depends on the number of color channels and the
 * number of pulses used to encode each bit.
 *
 * We get an interrupt each time a buffer is sent; we then fill that
 * buffer while the next one is being sent. The DMA interface allows
 * us to configure the buffers as a circularly linked list, so that it
 * can automatically start on the next buffer.
 */
/*
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    
#ifdef __cplusplus
}
#endif

__attribute__ ((always_inline)) inline static uint32_t __clock_cycles() {
    uint32_t cyc;
    __asm__ __volatile__ ("rsr %0,ccount":"=a" (cyc));
    return cyc;
}

#define FASTLED_HAS_CLOCKLESS 1
#define NUM_COLOR_CHANNELS 3

// -- Choose which I2S device to use
#ifndef I2S_DEVICE
#define I2S_DEVICE 0
#endif

// -- Max number of controllers we can support
#ifndef FASTLED_I2S_MAX_CONTROLLERS
#define FASTLED_I2S_MAX_CONTROLLERS 24
#endif

#ifndef MAX_SERIAL_CONTROLLERS_PER_PIN
#define MAX_SERIAL_CONTROLLERS_PER_PIN
#endif

#ifndef MAX_VIRTUALS_PINS
#define MAX_VIRTUALS_PINS
#endif

// -- Define the clock and latch pins default
#ifndef I2S_CLOCK_PIN
#define I2S_CLOCK_PIN 27
#endif

#ifndef I2S_LATCH_PIN 12
#define I2S_LATCH_PIN 12
#endif

// -- I2S clock
#define I2S_BASE_CLK (80000000L)
#define I2S_MAX_CLK (20000000L) //more tha a certain speed and the I2s looses some bits
#define I2S_MAX_PULSE_PER_BIT 20 //put it higher to get more accuracy but it could decrease the refresh rate without real improvement
// -- Convert ESP32 cycles back into nanoseconds
#define ESPCLKS_TO_NS(_CLKS) (((long)(_CLKS) * 1000L) / F_CPU_MHZ)


typedef struct{
	int pinNumber;
	int nb_lines;
	CLEDController * gControllers[MAX_SERIAL_CONTROLLERS_PER_PIN]; ///controllers
} virtualPins;


// -- Array of all controllers
//static CLEDController * gControllers[FASTLED_I2S_MAX_CONTROLLERS];
static virtualPins VirtualPins[MAX_VIRTUALS_PINS];
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
static int i2s_clock;

// --- I2S DMA buffers
struct DMABuffer {
    lldesc_t descriptor;
    uint8_t * buffer;
};

#define NUM_DMA_BUFFERS 3
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

template <int DATA_PIN, int T1, int T2, int T3, EOrder RGB_ORDER = RGB, int XTRA0 = 0, bool FLIP = false, int WAIT_TIME = 5>
class ClocklessController : public CPixelLEDController<RGB_ORDER>
{
    // -- Store the GPIO pin
    gpio_num_t     mPin;
    
    // -- This instantiation forces a check on the pin choice
    FastPin<DATA_PIN> mFastPin;
    
    // -- Save the pixel controller
    PixelController<RGB_ORDER> * mPixels;
    
public:


	void add(virtualPins *vp,int pin,CLEDController controller)
	{
		bool found=false;
		int index=0;
		for(int i=0;i<16;i++)
		{
			//printf("on a %d nb_lines\n",vp[i].nb_lines);  
			if(vp[i].nb_lines>0)
			{
				index=i+1;
				
			//printf("on a %d pins\n",vp[i].pinNumber);  
				if(vp[i].pinNumber==pin)
				{
					vp[i].nb_lines++;
					vp[i].gControllers[vp[i].nb_lines-1]=controller;
					found=true;
					//printf("on ajoute %d au pin %d nb values %d\n",value,pin,vp[i].nb_lines);
					break;
				}
			}
		}
		if(found==false)
		{
			virtualPins k;
			k.nb_lines=1;
			k.pinNumber=pin;
			k.gControllers[0]=controller;
			//printf("on cree %d au pin %d nb value %d index %d\n",value,pin,k.nb_lines,index);
			vp[index].nb_lines=1;
			vp[index].pinNumber=pin;
			//vp[index].l[0]=value;
			
		}
	}
	
    void init()
    {
        i2sInit();
        
        // -- Allocate space to save the pixel controller
        //    during parallel output
        mPixels = (PixelController<RGB_ORDER> *) malloc(sizeof(PixelController<RGB_ORDER>));
        
       add(VirtualPins,DATA_PIN,this);
	   // gControllers[gNumControllers] = this;
        int my_index = gNumControllers;
        gNumControllers++;
        
        // -- Set up the pin We have to do two things: configure the
        //    actual GPIO pin, and route the output from the default
        //    pin (determined by the I2S device) to the pin we
        //    want. We compute the default pin using the index of this
        //    controller in the array. This order is crucial because
        //    the bits must go into the DMA buffer in the same order.
        mPin = gpio_num_t(DATA_PIN);
        
        PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[DATA_PIN], PIN_FUNC_GPIO);
        gpio_set_direction(mPin, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
        pinMode(mPin,OUTPUT);
        gpio_matrix_out(mPin, i2s_base_pin_index + my_index, false, false);
    }
    
    virtual uint16_t getMaxRefreshRate() const { return 400; }
    
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
        if (gInitialized) return;
        
        // -- Construct the bit patterns for ones and zeros
       // initBitPatterns();
        
        // -- Choose whether to use I2S device 0 or device 1
        //    Set up the various device-specific parameters
        int interruptSource;
		
        if (I2S_DEVICE == 0) {
            i2s = &I2S0;
            periph_module_enable(PERIPH_I2S0_MODULE);
            interruptSource = ETS_I2S0_INTR_SOURCE;
            i2s_base_pin_index = I2S0O_DATA_OUT0_IDX;
			i2s_clock=I2S0O_BCK_OUT_IDX;
        } else {
            i2s = &I2S1;
            periph_module_enable(PERIPH_I2S1_MODULE);
            interruptSource = ETS_I2S1_INTR_SOURCE;
            i2s_base_pin_index = I2S1O_DATA_OUT0_IDX;
			i2s_clock=I2S1O_BCK_OUT_IDX;
        }
        
		//set the clock pin
		gpio_matrix_out(I2S_CLOCK_PIN, i2s_clock, false, false);
		
		//set the latch pin
		
		PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[I2S_LATCH_PIN], PIN_FUNC_GPIO);
		gpio_set_direction((gpio_num_t)I2S_LATCH_PIN, (gpio_mode_t)GPIO_MODE_DEF_OUTPUT);
    	pinMode(12,OUTPUT);
		gpio_matrix_out(I2S_LATCH_PIN, i2s_base_pin_index + 23, false, false);
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
		
		rtc_clk_apll_enable(true, 82, 184,2, 0); //set the speed of the clock at 14.4 Mhz=0.8*3*6
       
        
        i2s->clkm_conf.val = 0;
	    i2s->clkm_conf.clka_en = 1;
	    i2s->clkm_conf.clkm_div_num =1;//33;//1; //clockN;
	    i2s->clkm_conf.clkm_div_a =1;   //clockA;
	    i2s->clkm_conf.clkm_div_b =0;   //clockB;
    	i2s->sample_rate_conf.tx_bck_div_num = 1;
      
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
        dmaBuffers[0] = allocateDMABuffer(7*3*8*3*4);
        dmaBuffers[1] = allocateDMABuffer(7*3*8*3*4);
		dmaBuffers[2] = allocateDMABuffer(7*3*8*3*4);
        
        // -- Arrange them as a circularly linked list
        dmaBuffers[0]->descriptor.qe.stqe_next = &(dmaBuffers[1]->descriptor);
        dmaBuffers[1]->descriptor.qe.stqe_next = &(dmaBuffers[0]->descriptor);
       
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
	memset((uint8_t*)buff,0,7*3*8*3*4);
	for (int i=0;i<24*3;i++)
		{
		 buff[6+i*7]=0xFFFFFFFF;
		}
    }

    static void pu2(uint32_t* buff)
    {
     for (int j=0;j<24;j++)
      {
      for (int i=0;i<5;i++)
		{
		 *buff=0x000000;
		 buff++;
		}
		buff+=13;
      }
    }


   

  static void transpose16x1_noinline2(unsigned char *A, uint8_t *B,uint8_t offset) {
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
    
    // -- Show pixels
    //    This is the main entry point for the controller.
    virtual void showPixels(PixelController<RGB_ORDER> & pixels)
    {
        if (gNumStarted == 0) {
            // -- First controller: make sure everything is set up
            xSemaphoreTake(gTX_sem, portMAX_DELAY);
        }
        
        // -- Initialize the local state, save a pointer to the pixel
        //    data. We need to make a copy because pixels is a local
        //    variable in the calling function, and this data structure
        //    needs to outlive this call to showPixels.
        (*mPixels) = pixels;
        
        // -- Keep track of the number of strips we've seen
        gNumStarted++;

        // Serial.print("Show pixels ");
        // Serial.println(gNumStarted);
        
        // -- The last call to showPixels is the one responsible for doing
        //    all of the actual work
        if (gNumStarted == gNumControllers) {
            pu((uint32_t*)this->dmaBuffers[0]->buffer); //latch
			pu((uint32_t*)this->dmaBuffers[1]->buffer);
			pu((uint32_t*)this->dmaBuffers[2]->buffer);
			pu2((uint32_t*)this->dmaBuffers[0]->buffer); //first pulse
			pu2((uint32_t*)this->dmaBuffers[1]->buffer);


        dmaBufferActive=1;
		// this->dmaBuffers[3]->next(this->dmaBuffers[0]);
			this->dmaBuffers[1]->next(this->dmaBuffers[0]);
			this->dmaBuffers[2]->next(this->dmaBuffers[0]);
			this->dmaBuffers[0]->next(this->dmaBuffers[1]); 
            gCurBuffer = 0;
            gDoneFilling = false;
            
            // -- Prefill both buffers
            fillbuffer6((uint32_t*)dmaBuffers[0]->buffer);
            
            i2sStart();
            
            // -- Wait here while the rest of the data is sent. The interrupt handler
            //    will keep refilling the DMA buffers until it is all sent; then it
            //    gives the semaphore back.
            xSemaphoreTake(gTX_sem, portMAX_DELAY);
            xSemaphoreGive(gTX_sem);
            
            i2sStop();
            
            // -- Reset the counters
            gNumStarted = 0;
        }
    }
    
    // -- Custom interrupt handler
    static IRAM_ATTR void interruptHandler(void *arg)
    {
        if (i2s->int_st.out_eof) {
            i2s->int_clr.val = i2s->int_raw.val;
            
            if ( ! gDoneFilling) {
                fillBuffer();
            } else {
                portBASE_TYPE HPTaskAwoken = 0;
                xSemaphoreGiveFromISR(gTX_sem, &HPTaskAwoken);
                if(HPTaskAwoken == pdTRUE) portYIELD_FROM_ISR();
            }
        }
    }
    
	
	
static void fillbuffer6(uint32_t *buff)
{
	Lines firstPixel[3];
        Lines secondPixel[3];
		int nblines=5;
  int nbbits=8;
  int nbpins=16;
  int lowerpins;
  

	
   for (int line=0;line<nblines;line++){
   uint32_t l=ledToDisplay+nun_led_per_strip*line;
        for(int pin=0;pin<nbpins;pin++) {
 
			firstPixel[0].bytes[pin] = leds[l].g/brigthness;
            firstPixel[1].bytes[pin] = leds[l].r/brigthness;
            firstPixel[2].bytes[pin] = leds[l].b/brigthness;
			l+=nun_led_per_strip*5;


			}
			 putPixelinBuffer2(firstPixel,buff,line);
		}
}

 


	static void   putPixelinBuffer2(Lines *pixel,uint32_t *buf,int line)
    {
        Lines b2,b,b3,b4;
        
        for (int color=0;color<3;color++)
        {
			 b=pixel[color];         
			transpose16x1_noinline2(b.bytes,(uint8_t*)&buf[(7+color*8)*18+10-line]+1,18*4);
           
        }
    }
    /** Fill DMA buffer
     *
     *  This is where the real work happens: take a row of pixels (one
     *  from each strip), transpose and encode the bits, and store
     *  them in the DMA buffer for the I2S peripheral to read.
     */
    static void fillBuffer()
    {
        // -- Alternate between buffers
        volatile uint32_t * buf = (uint32_t *) dmaBuffers[gCurBuffer]->buffer;
        gCurBuffer = (gCurBuffer + 1) % NUM_DMA_BUFFERS;
        
        // -- Get the requested pixel from each controller. Store the
        //    data for each color channel in a separate array.
        uint32_t has_data_mask = 0;
        for (int i = 0; i < gNumControllers; i++) {
            // -- Store the pixels in reverse controller order starting at index 23
            //    This causes the bits to come out in the right position after we
            //    transpose them.
            int bit_index = 23-i;
            ClocklessController * pController = static_cast<ClocklessController*>(gControllers[i]);
            if (pController->mPixels->has(1)) {
                gPixelRow[0][bit_index] = pController->mPixels->loadAndScale0();
                gPixelRow[1][bit_index] = pController->mPixels->loadAndScale1();
                gPixelRow[2][bit_index] = pController->mPixels->loadAndScale2();
                pController->mPixels->advanceData();
                pController->mPixels->stepDithering();
                
                // -- Record that this controller still has data to send
                has_data_mask |= (1 << (i+8));
            }
        }
        
        // -- None of the strips has data? We are done.
        if (has_data_mask == 0) {
            gDoneFilling = true;
            return;
        }
        
        // -- Transpose and encode the pixel data for the DMA buffer
        int buf_index = 0;
        for (int channel = 0; channel < NUM_COLOR_CHANNELS; channel++) {
            
            // -- Tranpose each array: all the bit 7's, then all the bit 6's, ...
            transpose32(gPixelRow[channel], gPixelBits[channel][0] );
            
            //Serial.print("Channel: "); Serial.print(channel); Serial.print(" ");
            for (int bitnum = 0; bitnum < 8; bitnum++) {
                uint8_t * row = (uint8_t *) (gPixelBits[channel][bitnum]);
                uint32_t bit = (row[0] << 24) | (row[1] << 16) | (row[2] << 8) | row[3];
                
               /* SZG: More general, but too slow:
                    for (int pulse_num = 0; pulse_num < gPulsesPerBit; pulse_num++) {
                        buf[buf_index++] = has_data_mask & ( (bit & gOneBit[pulse_num]) | (~bit & gZeroBit[pulse_num]) );
                     }
               */

                // -- Only fill in the pulses that are different between the "0" and "1" encodings
                for(int pulse_num = ones_for_zero; pulse_num < ones_for_one; pulse_num++) {
                    buf[bitnum*gPulsesPerBit+channel*8*gPulsesPerBit+pulse_num] = has_data_mask & bit;
                }
            }
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
