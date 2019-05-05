
#include "esp_heap_caps.h"
#include "soc/soc.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2s_reg.h"
#include "soc/i2s_struct.h"
#include "soc/io_mux_reg.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "rom/lldesc.h"
#include <soc/rtc.h>
#include "DMABuffer.h"


#define I2SESP32 1

#define PORT_MASK (((1<<LANES)-1) & 0x0000FFFFL)


FASTLED_NAMESPACE_BEGIN



template <uint8_t LANES,EOrder RGB_ORDER = GRB,int PIN1=-1,int PIN2=-1,int PIN3=-1,int PIN4=-1,int PIN5=-1,int PIN6=-1,int PIN7=-1,int PIN8=-1,int PIN9=-1,int PIN10=-1,int PIN11=-1,int PIN12=-1,int PIN13=-1,int PIN14=-1,int PIN15=-1,int PIN16=-1,int PIN17=-1,int PIN18=-1,int PIN19=-1,int PIN20=-1,int PIN21=-1,int PIN22=-1>
class I2SBlockClocklessController : public CPixelLEDController<RGB_ORDER, LANES> {



	int Pins[22]={PIN1,PIN2,PIN3,PIN4,PIN5,PIN6,PIN7,PIN8,PIN9,PIN10,PIN11,PIN12,PIN13,PIN14,PIN15,PIN16,PIN17,PIN18,PIN19,PIN20,PIN21,PIN22};
      int dmaBufferCount=2; //we use two buffers
    typedef union {
        uint8_t bytes[24];
        uint32_t shorts[8];
        uint32_t raw[2];
    } Lines;
  volatile  int num_strips;
  volatile  int nun_led_per_strip;
    PixelController<RGB_ORDER, LANES> pixels;
    int brigthness;
    int ledType;
		int i2sIndex=0;
	intr_handle_t interruptHandle;
	CRGB *leds;
	int dmaBufferActive;
	DMABuffer **dmaBuffers;
	volatile bool stopSignal;
 volatile bool runningPixel=false;

    int ledToDisplay;
public:
   

    virtual void showPixels(PixelController<RGB_ORDER, LANES> & pix) {
	pixels=pix;
	
    }


    virtual void init() {
	// Only supportd on pins 12-15
        // SZG: This probably won't work (check pins definitions in fastpin_esp32)

			for (int i=0;i<LANES;i++)
			{
				Serial.println(Pins[i]);
			}
	
      
	
	    }
private:

   
};

FASTLED_NAMESPACE_END

