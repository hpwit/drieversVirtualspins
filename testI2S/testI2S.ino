


#include "FastLED.h"
//uint8_t pixelg[8][8] ={{ 64, 0, 0, 255, 10, 25, 24, 129},{0, 255,  255, 0, 10, 25, 24, 129},{1, 0, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129}};
//uint8_t pixelr[8][8] ={{ 0, 255, 0, 0, 10, 25, 24, 129},{255, 255, 0,  255, 10, 25, 24, 129},{1, 0, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129}};
//uint8_t pixelb[8][8] ={{ 0, 0,255, 255, 10, 25, 24, 129},{0,    0,   0,  255, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129},{1, 255, 8, 9, 10, 25, 24, 129}};

#define DOWN_RIGHT   0 //natural mode no transpostion to be made
#define DOWN_LEFT    1 //We turn 90° clock wise
#define UP_LEFT      2
#define UP_RIGHT     3
#define DOWN_RIGHT_INV   4 //natural mode no transpostion to be made
#define DOWN_LEFT_INV    5 //We turn 90° clock wise
#define UP_LEFT_INV      6
#define UP_RIGHT_INV 7
#define LED_WIDTH 16
#define NUM_STRIPS 80
#define LED_HEIGHT_PER_STRIP 16
#define NUM_LEDS_PER_STRIP LED_HEIGHT_PER_STRIP*LED_WIDTH
#define LED_HEIGHT NUM_STRIPS*LED_HEIGHT_PER_STRIP
 //up to 22
#define NUM_LEDS NUM_STRIPS * NUM_LEDS_PER_STRIP
#include "I2S.h"
#include "fontamstrad.h"
int tableOrientation=DOWN_RIGHT_INV; //DOWN_RIGHT_INV;
    
int Pins[24]={13,21,4,5,26,2,32,17,18,19,21,23,25,22,0,33,-1,-1,-1,-1,-1,-1,-1,12};

CRGB leds[NUM_LEDS];
CRGB Tpic[12];
I2S controller(0);




 void afficheMessage2(char *mess,int x,int y)
{
    if (!mess)
        return;
    int taille=strlen(mess);
   // Serial.println(mess);
    //Serial.println(taille);
    int d=0;
    int dx=0;
    
    for (long int i=0;i<taille;i++)
    {
//Serial.printf("%d ",mess[i]);
  
       if(mess[i]=='\n')
      {
        d++;
        Serial.printf("\n d:%d \n",d);
        dx=i+1;
        if (y-d*8<=-8)
         return;
      }
      else
      {
       if(x+(i-dx)*8 > -8 and x+(i-dx)*8 <LED_WIDTH)
        afficheLettre2(mess[i]-32, x+(i-dx)*8,y-d*8);
        
        //if (x+i*8>= LED_WIDTH)
          //  return;
    }
    }
  
  //Serial.println("fin message");  
}

void displayPicNew(CRGB *pica, int x0, int y0, int h, int w)
{

    int max_wide=0;
    int wide=w+x0;
    int offsety;
    int offsetx;
    int newx=0;
    int newy=0;
    if(wide<=0)
        return; // l'image ne peut etre affichée
    if(tableOrientation%2==0)
        max_wide=LED_WIDTH;
    else
        max_wide=LED_HEIGHT;
    
    if (x0>=max_wide) //on est en dehors du cadre
        return;
    if(x0>=0)
    {
        newx=x0;
        offsetx=0;
        if(w+x0<max_wide)
            wide=w;
        else
            wide=max_wide-x0;
    }
    else
    {
        newx=0;
        offsetx=-x0;
        if(w+x0<max_wide)
            wide=w+x0;
        else
            wide=max_wide;
    }
    
    
    
    
    int max_height=0;
    int height=h+y0;
    if(height<=0)
        return; // l'image ne peut etre affichée
    if(tableOrientation%2==0)
        max_height=LED_HEIGHT;
    else
        max_height=LED_WIDTH;
    
    if (y0>=max_height) //on est en dehors du cadre
        return;
    if(y0>=0)
    {
        newy=y0;
        offsety=0;
        if(h+y0<max_height)
            height=h;
        else
            height=max_height-y0;
    }
    else
    {
        newy=0;
        offsety=-y0;
        if(h+y0<max_height)
            height=h+y0;
        else
            height=max_height;
    }
    
    
    int r;
    int g;
    int b;

   CRGB *bitmapRGB =(CRGB*)malloc(wide*height*sizeof(CRGB)) ;
    if(bitmapRGB==NULL)
    {
        Serial.printf("immossibnle de créer l'image im %d %d\n",wide,height);
        return;
    }
//Serial.println("debut dessin");
    for(int y=0;y<height;y++){
       /* for(int x=0;x<wide;x++){
          
            
        //    r= gammar[(int)pgm_read_byte(pic+ 3*((y+offsety)*w+x+offsetx))];
         //   g= gamma8[(int)pgm_read_byte(pic+3*((y+offsety)*w+x+offsetx)+1)];
          //  b= gamma8[(int)pgm_read_byte(pic+3*((y+offsety)*w+x+offsetx)+2)];
            bitmapRGB[x+y*wide]=pica[(y+offsety)*w+x+offsetx];
            //Serial.printf("%03X ",pica[(y+offsety)*w+x+offsetx]);//I have a program that switches directly from
        }*/
        // memcpy(&leds[LED_WIDTH * y], &Tpic[(y - y0) *wmin - x0], nh * sizeof(CRGB));
        //Serial.println("ici create bitmaap");
        memcpy(bitmapRGB+wide*y,&pica[(y+offsety)*w+offsetx],wide*sizeof(CRGB));
     // Serial.println();
    }
    //putOrientation(bitmapRGB,newx,newy,height,wide);
    // Serial.println("fin dessin");
    putOrientation2(bitmapRGB,height,wide,newx,newy);
   // displaypic(bitmapRGB,newx,newy,height,wide);
    free(bitmapRGB);
    
}
CRGB bgColor=CRGB(10,10,10);
CRGB lettrefont2[8*9];
void afficheLettre2(int let,int x0,int y0)
{
    
    int taille=8;
    int hauteur=8;
    
    for(int x=x0;x<taille+x0;x++)
    {
        for (int y=y0;y<hauteur+y0;y++)
        {
            if ( ( (1 << (x0-x+taille-1)) &   fontamstrad[let][hauteur-1 - y + y0] ) > 0 )
            {
                lettrefont2[(x-x0)+(y-y0)*taille]=CRGB(64,0,64);//CRGB::Blue;
            }
            else
            {
                lettrefont2[(x-x0)+(y-y0)*taille]=bgColor;
            }
        }
         lettrefont2[(x-x0)+(hauteur)*taille]=bgColor;
    }
    //Serial.printf("pic :%d %d\n",x0,y0);
    displayPicNew(lettrefont2,x0,y0,hauteur+1,taille);
    
}

void displayPicNewInv(CRGB *pica, int x0, int y0, int h, int w)
{

    int max_wide=0;
    int wide=w+x0;
    int offsety;
    int offsetx;
    int newx=0;
    int newy=0;
    if(wide<=0)
        return; // l'image ne peut etre affichée
    if(tableOrientation%2==0)
        max_wide=LED_WIDTH;
    else
        max_wide=LED_HEIGHT;
    
    if (x0>=max_wide) //on est en dehors du cadre
        return;
    if(x0>=0)
    {
        newx=x0;
        offsetx=0;
        if(w+x0<max_wide)
            wide=w;
        else
            wide=max_wide-x0;
    }
    else
    {
        newx=0;
        offsetx=-x0;
        if(w+x0<max_wide)
            wide=w+x0;
        else
            wide=max_wide;
    }
    
    
    
    
    int max_height=0;
    int height=h+y0;
    if(height<=0)
        return; // l'image ne peut etre affichée
    if(tableOrientation%2==0)
        max_height=LED_HEIGHT;
    else
        max_height=LED_WIDTH;
    
    if (y0>=max_height) //on est en dehors du cadre
        return;
    if(y0>=0)
    {
        newy=y0;
        offsety=0;
        if(h+y0<max_height)
            height=h;
        else
            height=max_height-y0;
    }
    else
    {
        newy=0;
        offsety=-y0;
        if(h+y0<max_height)
            height=h+y0;
        else
            height=max_height;
    }
    
    
    int r;
    int g;
    int b;

   CRGB *bitmapRGB =(CRGB*)malloc(wide*height*sizeof(CRGB)) ;
    if(bitmapRGB==NULL)
    {
        Serial.println("immossibnle de créer l'image");
        return;
    }
//Serial.println("debut dessin");
    for(int y=0;y<height;y++){
        for(int x=0;x<wide;x++){
          
            
        //    r= gammar[(int)pgm_read_byte(pic+ 3*((y+offsety)*w+x+offsetx))];
         //   g= gamma8[(int)pgm_read_byte(pic+3*((y+offsety)*w+x+offsetx)+1)];
          //  b= gamma8[(int)pgm_read_byte(pic+3*((y+offsety)*w+x+offsetx)+2)];
            bitmapRGB[x+y*wide]=pica[(y+offsety)*w+w-1-x-offsetx];
            //Serial.printf("%03X ",pica[(y+offsety)*w+x+offsetx]);//I have a program that switches directly from
        }
        // memcpy(&leds[LED_WIDTH * y], &Tpic[(y - y0) *wmin - x0], nh * sizeof(CRGB));
        //Serial.println("ici create bitmaap");
      // memcpy(bitmapRGB+wide*y,&pica[(y+offsety)*w+offsetx],wide*sizeof(CRGB));
     // Serial.println();
    }
    //putOrientation(bitmapRGB,newx,newy,height,wide);
    // Serial.println("fin dessin");
    putOrientation2(bitmapRGB,height,wide,newx,newy);
   // displaypic(bitmapRGB,newx,newy,height,wide);
    free(bitmapRGB);
    
}


void putOrientation2(CRGB *pic,int h,int w,int x0,int y0)
{
    //CRGB * Tpic = (CRGB*)calloc(h*w,sizeof(CRGB));
    if(!Tpic)
    {
        Serial.println("pas possible");
        return ;//NULL;
    }
    switch(tableOrientation){
            
        case DOWN_RIGHT: //on ne fait rien
         /*   for(int y=0;y<h;y++)
            {
                for (int x=0;x<w;x++)
                {
                    if( ((x+x0)+LED_WIDTH*(y+y0)) <NUM_LEDS && x+x0 >=0 && y+y0>=0 && x+x0<LED_WIDTH && y+y0 <LED_HEIGHT)
                        Tpic[x+x0+LED_WIDTH*(y+y0)]=pic[w*y+x];
                }
            }
*/

        for(int y=0;y<h;y++)
            {

           // if(pic[w*y+x]!=bgColor)
            //{
              if((y+y0)%2==0)
              {
             /* for (int x=0;x<w;x++)
                {
                   // if( LED_WIDTH-1-x-x0+LED_WIDTH*(y+y0) <NUM_LEDS && LED_WIDTH-1-x-x0 >=0 && y+y0>=0 && LED_WIDTH-1-x-x0<LED_WIDTH && y+y0 <LED_HEIGHT)
                        //leds[LED_WIDTH-1-x-x0+LED_WIDTH*(y+y0)]=pic[w*y+x];
                      //  if(pic[w*y+x]!=bgColor)
                      leds[x+x0+LED_WIDTH*(y+y0)]=pic[w*y+x];
                }*/
               memcpy((char *) leds +(LED_WIDTH*(y+y0) +x0)*sizeof(CRGB), (char *) pic +(w*y)*sizeof(CRGB), w*sizeof(CRGB));
              }
              else
              {
                for (int x=0;x<w;x++)
                {
                   // if( LED_WIDTH-1-x-x0+LED_WIDTH*(y+y0) <NUM_LEDS && LED_WIDTH-1-x-x0 >=0 && y+y0>=0 && LED_WIDTH-1-x-x0<LED_WIDTH && y+y0 <LED_HEIGHT)
                   //leds[(LED_WIDTH <<1)* ( (y>>1) + 1) - (1 + x)]
                       // leds[LED_WIDTH-1-x-x0+ LED_WIDTH*(y+y0)]=pic[w*y+x];
                       //if(pic[w*y+x]!=bgColor) 
                       leds[ (LED_WIDTH<<1)*( ((y+y0)>>1)+1)     -( 1+x+x0)]   =pic[w*y+x];
                      
                }
               //  memcpy((char *) leds +(  (LED_WIDTH<<1) * ( ((y+y0)>>1)  + 1) - (1+x0+w-1) )*sizeof(CRGB), (char *) pic +(w*y)*sizeof(CRGB), w*sizeof(CRGB));
              }
         //  }
              
            }
            
            
            
            break;
            
        case DOWN_LEFT:
            for(int y=0;y<h;y++)
            {
                for (int x=0;x<w;x++)
                {
                    if( LED_WIDTH*(LED_HEIGHT-1-x-x0)+y+y0 <NUM_LEDS && LED_HEIGHT-1-x-x0 >=0 && y+y0>=0 && LED_HEIGHT-1-x-x0<LED_HEIGHT && y+y0 <LED_WIDTH)
                    Tpic[LED_WIDTH*(LED_HEIGHT-1-x-x0)+y+y0]=pic[w*y+x];
                }
            }
            break;
            
            
        case UP_RIGHT:
            for(int y=0;y<h;y++)
            {
                for (int x=0;x<w;x++)
                {
                    if( (LED_WIDTH*((x+x0))+LED_WIDTH-1-(y+y0)) <NUM_LEDS  && (x+x0) >=0 && LED_WIDTH-1-(y+y0)>=0 && (x+x0)<LED_HEIGHT && LED_WIDTH-1-(y+y0) <LED_WIDTH  )
                    Tpic[LED_WIDTH*((x+x0))+LED_WIDTH-1-(y+y0)]=pic[w*y+x];
                }
            }
            break;
            
        case UP_LEFT:
            for(int y=0;y<h;y++)
            {
                for (int x=0;x<w;x++)
                {
                    if( LED_WIDTH-1-(x+x0)+LED_WIDTH*(LED_HEIGHT-1-(y+y0)) <NUM_LEDS  && LED_WIDTH-1-(x+x0)>=0 && LED_WIDTH*(LED_HEIGHT-1-(y+y0))>=0 && (LED_HEIGHT-1-(y+y0))<LED_HEIGHT && LED_WIDTH-1-(x+x0) <LED_WIDTH )
                        Tpic[LED_WIDTH-1-(x+x0)+LED_WIDTH*(LED_HEIGHT-1-(y+y0))]=pic[w*y+x];
                }
            }
            break;
            
//on inverse la lecture des pixels au niveau des X on lit de gauche a droite
        case DOWN_RIGHT_INV: //on ne fait rien
      // Serial.printf("%d %d %d %d\n",x0,y0,h,w);
            for(int y=0;y<h;y++)
            {

              if((y+y0)%2==0)
              {
              for (int x=0;x<w;x++)
                {
                   // if( LED_WIDTH-1-x-x0+LED_WIDTH*(y+y0) <NUM_LEDS && LED_WIDTH-1-x-x0 >=0 && y+y0>=0 && LED_WIDTH-1-x-x0<LED_WIDTH && y+y0 <LED_HEIGHT)
                        leds[LED_WIDTH-1-x-x0+LED_WIDTH*(y+y0)]=pic[w*y+x];
                      // leds[x+x0+LED_WIDTH*(y+y0)]=pic[w*y+x];
                }
              }
              else
              {
                for (int x=0;x<w;x++)
                {
                   // if( LED_WIDTH-1-x-x0+LED_WIDTH*(y+y0) <NUM_LEDS && LED_WIDTH-1-x-x0 >=0 && y+y0>=0 && LED_WIDTH-1-x-x0<LED_WIDTH && y+y0 <LED_HEIGHT)
                   //leds[(LED_WIDTH <<1)* ( (y>>1) + 1) - (1 + x)]
                       // leds[LED_WIDTH-1-x-x0+ LED_WIDTH*(y+y0)]=pic[w*y+x];
                       leds[ (LED_WIDTH<<1)*( ((y+y0)>>1)+1)     -( 1+LED_WIDTH-1-x-x0)]   =pic[w*y+x];
                }
              }
            }
            
            break;
            
        case DOWN_LEFT_INV:
            for(int y=0;y<h;y++)
            {
                for (int x=0;x<w;x++)
                {
                    if( LED_WIDTH*(x+x0)+y+y0 <NUM_LEDS && x+x0 >=0 && y+y0>=0 && x+x0<LED_HEIGHT && y+y0 <LED_WIDTH)
                        Tpic[LED_WIDTH*(x+x0)+y+y0]=pic[w*y+x];
                }
            }
            break;
            
            
        case UP_RIGHT_INV:
            for(int y=0;y<h;y++)
            {
                for (int x=0;x<w;x++)
                {
                    if( (LED_WIDTH*((x+x0))+(y+y0)) <NUM_LEDS  && (x+x0) >=0 && (y+y0)>=0 && (x+x0)<LED_HEIGHT && (y+y0) <LED_WIDTH  )
                        Tpic[LED_WIDTH*((x+x0))+(y+y0)]=pic[w*y+x];
                }
            }
            break;
            
        case UP_LEFT_INV:
            for(int y=0;y<h;y++)
            {
                for (int x=0;x<w;x++)
                {
                    if( (x+x0)+LED_WIDTH*(LED_HEIGHT-1-(y+y0)) <NUM_LEDS  && (x+x0)>=0 && LED_WIDTH*(LED_HEIGHT-1-(y+y0))>=0 && (LED_HEIGHT-1-(y+y0))<LED_HEIGHT && (x+x0) <LED_WIDTH )
                        Tpic[(x+x0)+LED_WIDTH*(LED_HEIGHT-1-(y+y0))]=pic[w*y+x];
                }
            }
            break;
            
            
            
    }
     //return Tpic;
    
}
void setup() {
  // put your setup code here, to run once:
     Serial.begin(115200);
CRGB l[3]={CRGB::Blue,CRGB::Red,CRGB::Green};
uint8_t *d= (uint8_t*)l;
uint8_t f[3]={0,0,255};

Serial.printf("%d:%d:%d %d\n",d[0],d[1],d[2],(uint32_t)*f);
//l=l>>=1;
//Serial.printf("%d:%d:%d\n",l.g,l.r,l.b);
  
 Serial.println("init ready");
controller.initled(leds,Pins,NUM_STRIPS,NUM_LEDS_PER_STRIP);

//controller.initled(leds,Pins,NUM_STRIPS,NUM_LEDS_PER_STRIP,0); more suitable for ws2811

fill_solid(leds,NUM_LEDS,CRGB::Black);
//controller.showPixels();
controller.setBrightness(255); //to be used instead of fastled.setbritg hness 
//delay(200);
 
       
        fill_solid(leds,NUM_LEDS,CRGB(0,0,0));
       // controller.showPixels();
        delay(14);
        int k=0;
 for(int pin=0;pin<8;pin++)
   {
      for (int line=0;line<5;line++)
      {
        leds[k%NUM_LEDS_PER_STRIP+NUM_LEDS_PER_STRIP*line+pin*NUM_LEDS_PER_STRIP*5]=CRGB::Blue;
      }
    }
    Serial.printf("ledprtstrip:%d\n",NUM_LEDS_PER_STRIP);
    leds[0]=CRGB::Blue;
    leds[1]=CRGB::Green;
    leds[2]=CRGB::Red;
    leds[3]=CRGB::Blue;
    controller.showPixels();
    Serial.println("stop");
    //delay(2005);
    leds[0]=CRGB::Blue;
    leds[1]=CRGB::Green;
    leds[2]=CRGB::Red;
    leds[3]=CRGB::Red;
   // controller.showPixels();
    Serial.println("stop");
   // delay(2000);
}


   

 
int k=0;
void loop() {
 /*controller.dmaBufferActive=0;
 controller.stopSignal=false;
        controller.startTX();
 */

 fill_solid(leds,NUM_LEDS,CRGB(0,0,0));
 uint16_t i=0;
/*
 for(uint16_t pin=0;pin<8;pin++)
   {
      for (uint16_t line=0;line<5;line++)
      {
        for(uint16_t i=0;i<pin*5+(line+1);i++)
           leds[(uint16_t)((k+i)%256+256*line+pin*256*5)]=CRGB::Blue;
          // Serial.println(line+NUM_LEDS_PER_STRIP*(line+pin*NUM_LEDS_PER_STRIP*5);
       // leds[(k+1)%NUM_LEDS_PER_STRIP+NUM_LEDS_PER_STRIP*line+pin*NUM_LEDS_PER_STRIP*5]=CRGB::Red;
      }
    }*/
char mess[40];

for(int i=0;i<NUM_STRIPS;i++)
{
  int y=LED_HEIGHT_PER_STRIP*i+2;
   sprintf(mess," LINE %d ",i+1);
    afficheMessage2(mess,16-k%(7*12) ,y); //16-k%(7*16)
}

long     lastHandle = __clock_cycles();
   controller.showPixels();
 long   lasthandle2=__clock_cycles();
       Serial.printf("FPS: %f \n", (float) 240000000L/(lasthandle2 - lastHandle));
   
    k++;
  delay(20);

}
