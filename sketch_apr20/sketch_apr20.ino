#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include "TimerOne.h"

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

boolean RecordOn = false;

//define global variables
#define PINEN 7 //Mux Enable pin
#define PINA0 4 //Mux Address 0 pin
#define PINA1 5 //Mux Address 1 pin
#define PINA2 6 //Mux Address 2 pin
#define PINSO A2 //TCAmp Slave Out pin (MISO)
#define PINSC A3 //TCAmp Serial Clock (SCK)
#define PINCS A1  //TCAmp Chip Select Change this to match the position of the Chip Select Link

int count = 0;
int onoff = 0;
int tempdef = 375;
double timerdef = 4.50;
int fandef = 0;

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */


int SensorFail[8];
float floatTemp, floatInternalTemp;
char failMode[8];
int internalTemp, intTempFrac;
unsigned int Mask;
//char data[16];
unsigned char i, j, k, NumSensors =8, UpdateDelay=2;  //delay in seconds
char Rxchar, Rxenable, Rxptr, Cmdcomplete, R;
char Rxbuf[32];
char adrbuf[3], cmdbuf[3], valbuf[12];
int val = 0, Param;     
unsigned long int timems=0;
unsigned short int T16, mask;
String ts2="012345678901234567890123456789012"; 
const byte interruptPin = 2;
volatile byte state = LOW;
int onState=0;


long int period=2000000;
long int DesiredT; // set temp kevin had it to 337
float onTime = (0.0031246*DesiredT)-0.3109354;
int Timegoing = 0;
float Runtime; // set time in minutes  // changed to a double from float
long int numofperiods = Runtime*30;
long int TimeX = Runtime * 60000;


/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void drawFrame() //draw frame function
{
  tft.drawRect(0, 190, 140, 50, ILI9341_WHITE);
}

void STOPBtn() //stop button
{ 
  tft.fillRect(0, 190, 70, 50, ILI9341_RED);
  tft.fillRect(70, 190, 70, 50, ILI9341_BLUE);
  drawFrame();
  tft.setCursor(76,210);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("START");
  DesiredT = 0;
  Runtime = 0;
  RecordOn = false;
}

void STARTBtn() //start button
{
  tft.fillRect(70, 190, 70, 50, ILI9341_GREEN);
  tft.fillRect(0, 190, 70, 50, ILI9341_BLUE);
  drawFrame();
  tft.setCursor(12,210);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("STOP");
  DesiredT = tempdef;
  Runtime = timerdef;
  RecordOn = true;
}

void drawRec(){
  // Retrieve a point  
  TS_Point p = ts.getPoint();
  
  // Scale from ~0->4000 to tft.width using the calibration #'s
  p.x = map(p.x, 150, 3800, 0, tft.width());
  p.y = map(p.y, 130, 4000, 0, tft.height());
  
  //temp (-) button
  tft.drawRect(120, 0, 50, 50, ILI9341_BLUE); 
  tft.setCursor(132, 9);
  tft.setTextColor(ILI9341_BLUE);  tft.setTextSize(5);
  tft.println("-");
  
  //temp word location
  tft.setCursor(0, 20);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2.5);
  tft.println("Temp.(F)");

  //temp (+) button
  tft.drawRect(175, 0, 50, 50, ILI9341_RED); 
  tft.setCursor(187, 9);
  tft.setTextColor(ILI9341_RED);  tft.setTextSize(5);
  tft.println("+");

  //temp read-out box
  tft.drawRect(230, 0, 90, 50, ILI9341_WHITE); 
  tft.setCursor(305, 18);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
  tft.println("");
  
  //timer (-) button
  tft.drawRect(120, 60, 50, 50, ILI9341_BLUE); 
  tft.setCursor(132, 70);
  tft.setTextColor(ILI9341_BLUE);  tft.setTextSize(5);
  tft.println("-");

  //timer word location
  tft.setCursor(0, 80);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2.5);
  tft.println("Timer(min)");

  //timer (+) button
  tft.drawRect(175, 60, 50, 50, ILI9341_RED); 
  tft.setCursor(187, 70);
  tft.setTextColor(ILI9341_RED);  tft.setTextSize(5);
  tft.println("+");
  
  //temp read-out box
  tft.drawRect(230, 60, 90, 50, ILI9341_WHITE); 
  tft.setCursor(280, 78);
  tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(2);
  tft.println("");
}

int TEMP_MIN()
{
   if( tempdef >= 330) //min 325 Fahrenheit
   {
      int counter = tempdef - 5;//decrease step size by 5F
      tempdef = counter;
      return counter;
   }
}
int TEMP_MAX()
{
    if( tempdef <= 420) //max 420 Fahrenheit
    {
      int counter = tempdef + 5; //increase step size by 5F
      tempdef = counter;
      return counter;  
    }
}
double TIME_MIN()
{
    if( timerdef >= 0.1) //down to 0.0 min
    {
      double counter = timerdef - 0.1; //decrease step size by 0.1 min
      timerdef = counter;
      return counter;
    }
}
double TIME_ADD()
{
    if( timerdef <= 9.9) //up to 9.9 min
    {
      double counter = timerdef + 0.1; //increase step size by 0.1 min
      timerdef = counter;
      return counter;
    }
}

void setup() {
  
  Serial.begin(9600);
  tft.begin();
  if (!ts.begin()) { 
    Serial.println("Unable to start touchscreen.");
  } 
  else { 
    Serial.println("Touchscreen started."); 
  }

  tft.fillScreen(ILI9341_BLACK);
  // origin = left,top landscape (USB left upper)
  tft.setRotation(1); 
  STOPBtn();

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

pinMode(9, OUTPUT);//this sets a pin to connect to the SSR and by changing the state of the pin will turn the SSR on and off
  Timer1.attachInterrupt(callback);// Here is the function the allows fort he changing of PIN 10 state to control the SSR
 //below is just the temp scan variables
  //Serial.begin(9600);
  pinMode(PINEN, OUTPUT);     
  pinMode(PINA0, OUTPUT);    
  pinMode(PINA1, OUTPUT);    
  pinMode(PINA2, OUTPUT);    
  pinMode(PINSO, INPUT);    
  pinMode(PINCS, OUTPUT);    
  pinMode(PINSC, OUTPUT);    
 
  digitalWrite(PINEN, HIGH);   // enable on
  digitalWrite(PINA0, LOW); // low, low, low = channel 1
  digitalWrite(PINA1, LOW); 
  digitalWrite(PINA2, LOW); 
  digitalWrite(PINSC, LOW); //put clock in low
  

}//this is the function that allows for control of the temp
/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

void callback()
{
  long int T;// This is used to assign how long each part of this function
  //should run
  onState=1-onState;
 // by changing this between 1 and 0, it allow for the PIN to change
 // states, therefore turning the SSR off and on
 //HIGH is on, while LOW is off
  
  if(onState)//it is using the changing value to switch between the two parts of the
  //if/else statement
    {
    T=floor(period*onTime);//this says how long to be on during hte 2 seconds
    //EX: if onTime=.5 then it will be on for 1sec
    Timer1.setPeriod(T);
    // This is what the board uses to keep track of time so it knows when the right amount of time has passed
    digitalWrite(10, HIGH);// as said beforte this sets PIN 10 to HIGH
    //thus turning the SSR on
    }
  else
    {//now that onState has changed it switches to this part
    T=floor(period*(1.0-onTime));// using a 1-onTime will set the remaining amount
    //of the 2 seconds to LOW
    Timer1.setPeriod(T);
    digitalWrite(10,LOW);// Sets PIN 10 to low which turns SSR off then runs through the program again 
    //hence every two seconds it does a cycle
    }
}
/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */
void loop() {
//Touchscreen code
  drawRec();
  if (!ts.bufferEmpty())
  {   
    // Retrieve a point  
    TS_Point p = ts.getPoint(); 
    // Scale using the calibration #'s and rotate coordinate system
    p.x = map(p.x, 130, 4000, 0, tft.height());
    p.y = map(p.y, 150, 3800, 0, tft.width());
    int y = tft.height() - p.x;
    int x = p.y;

    if (RecordOn)
    {
      if((x > 0) && (x < (70))) {
        if ((y > 190) && (y <= (240))) {
          Serial.println("Red btn hit"); 
          STOPBtn();
        }
      }
    }
    else //Record is off (RecordOn == false)
    {
      if((x > 70) && (x < (140))) {
        if ((y > 190) && (y <= (240))) {
          Serial.println("Green btn hit"); 
          STARTBtn();
        }
      }
    }

    Serial.println(RecordOn);
  }

if(!ts.bufferEmpty()){

    TS_Point p = ts.getPoint(); 
    // Scale using the calibration #'s
    // and rotate coordinate system
    p.x = map(p.x, 130, 4000, 1, tft.height());
    p.y = map(p.y, 150, 3800, 1, tft.width());
    int y = tft.height() - p.x;
    int x = p.y;
 
  if( (p.x >= 190) && (p.x <= 240) && (p.y >= 120) && (p.y <= 170) ) //TEMP_MIN
     {   
        tft.setCursor(250, 18);
        tft.fillRect(232.5, 2.5, 85, 45, ILI9341_BLACK); 
        tft.setTextSize(3);
        tft.println(TEMP_MIN());
        return;
     }
    
  if( (p.x >= 190) && (p.x <= 240) && (p.y >= 175) && (p.y <= 225) ) //TEMP_ADD
     {
        tft.setCursor(250, 18);
        tft.fillRect(232.5, 2.5, 85, 45, ILI9341_BLACK); 
        tft.setTextSize(3);
        tft.println(TEMP_MAX());
        return;
     }

 if( (p.x >= 130) && (p.x <= 180) && (p.y >= 120) && (p.y <= 170) ) //TIME_MIN
     {       
        tft.setCursor(240, 75);
        tft.fillRect(232.5, 62.5, 85, 45, ILI9341_BLACK); 
        tft.setTextSize(3);
        tft.println(TIME_MIN());
        return;
     }
  if( (p.x >= 130) && (p.x <= 180) && (p.y >= 175) && (p.y <= 225) ) //TIME_ADD
     {
        tft.setCursor(240, 75);
        tft.fillRect(232.5, 62.5, 85, 45, ILI9341_BLACK); 
        tft.setTextSize(3);
        tft.println(TIME_ADD());
        return;
     }
}

/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */

//millis() returns the number of ms since starting sketch
  // the only way the arduino keeps track of time is by using
  //the Timer1 function, hence if Timer1 is stopped, it can't 
  //keep track of time for the callback function
 if(Timegoing==0&&timems>500){
  //here is where the Timegoing variable is used, just like
  //onState, it changes between 1 and 0 to help with setting the timer on/off
  //hence with Timegoing =0 and after 2 seconds have passed since the program started
  //it will start the Timer1
 Timer1.initialize(1000000);//This starts the Timer function allowing for the Arduino to keep track of time
 Timegoing=1;
 //Now the Timegoing is switched so that the timer can be stopped after the desired
 //length of time
  } 
  if(Timegoing==1&&timems>TimeX){
    //now when timems, which keeps track of time in milliseconds,
    //has counted up to the set time from above, it stops the Timer1
    //which stops the callback function there is no Timer1 to run it
    //thus ending the program 
    Timer1.stop();
    //Set Timegoing back to 0 so that it cna be run again when
    //the user tells it to
    Timegoing=0;
    digitalWrite(10,LOW);// turns off the SSR and the program is ready to run again
  }
  //the rest is the temp scan code


  
  if (millis() > (timems + ((unsigned int)UpdateDelay*1000)))
   {
    timems = millis();
//    SerialUSB.print("N=");
//    SerialUSB.println(count,DEC);
    count++;

   }
    Serial.println("T8");
    for(j=0;j<8;j++)
      {
      switch (j) //select channel
        {
        case 0:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, LOW);
        break;
        case 1:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, LOW);
        break;
        case 2:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, LOW);
        break;
        case 3:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, LOW);
        break;
        case 4:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, HIGH);
        break;
        case 5:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, LOW); 
          digitalWrite(PINA2, HIGH);
        break;
        case 6:
          digitalWrite(PINA0, LOW); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, HIGH);
        break;
        case 7:
          digitalWrite(PINA0, HIGH); 
          digitalWrite(PINA1, HIGH); 
          digitalWrite(PINA2, HIGH);
        break;
        }
 
       delay(5);
       digitalWrite(PINCS, LOW); //stop conversion
       delay(5);
       digitalWrite(PINCS, HIGH); //begin conversion
       delay(100);  //wait 100 ms for conversion to complete
       digitalWrite(PINCS, LOW); //stop conversion, start serial interface
       delay(1);
        /*
        Temp[j] = 0;
        failMode[j] = 0;
        SensorFail[j] = 0;
        internalTemp = 0;
        */
      
       T16=0;
       for (k=0;k<32;k++)   //for (i=31;i>=0;i--)
         {
          i=31-k;
          digitalWrite(PINSC, HIGH);
          delay(1);
 
          if ((i<=31) && (i>=18))
              {
              // these 14 bits are the thermocouple temperature data
              // bit 31 sign
              // bit 30 MSB = 2^10
              // bit 18 LSB = 2^-2 (0.25 degC)
          
              Mask = 1<<(i-18);
              if (digitalRead(PINSO)==1)
                {
                if (i == 31)
                  {
                  T16 += (0b11<<14);//pad the temp with the bit 31 value so we can read negative values correctly
                  }
               
                T16 += Mask;     
                }  
              }
         digitalWrite(PINSC, LOW);
         delay(1);     
         }  //end of loop over bits
         T16= T16 | 0x4000; // set bit 14 to a 1,   Bit 15 will be a 0 for positive 1 for negative
 //       Temp[j]=T16;
        if(j==0)
          ts2=String(T16,HEX);
        else
          {
          ts2=String(ts2 + String(T16,HEX));
          }
       } //end of j  loop over sensors
  Serial.println(ts2);
/* ------------------------------------------------------------------------------------------------------------------------------------------------------- */


}
