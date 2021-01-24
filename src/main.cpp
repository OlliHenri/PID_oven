/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Reflow oven temperature sensor with 128x64 LCD display

    20210117

  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   */

#include <Arduino.h>

#include <Thermocouple.h>
#include <MAX6675_Thermocouple.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <PID_v1.h>

#include <U8g2lib.h>

#define SCK_PIN 3
#define CS_PIN 4
#define SO_PIN 5

#define GREENLED 13

void draw(void);
void oven_simulator(uint8_t simRelayOnOff, double *simCount);

unsigned long oldTime;
static unsigned long my_seconds = 0;
static unsigned long simOldTime = millis();   // for oven simulator
uint8_t simOvenStarted;     // for oven simulator
static uint8_t simRelayTime = 0;  // for simulator

// LCD display variables
static char celsius_buff [12];
static char fahrenheit_buff [12];
static char timer_buff [12];
const char DEGREE_SYMBOL[] = { (char) 0xB0, '\0'};    //  degree symbol NULL terminated '\0'

Thermocouple* thermocouple;

U8G2_ST7920_128X64_1_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* CS=*/ 10);

// PID

//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
double Kp=2, Ki=5, Kd=1;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, P_ON_M, DIRECT);    //PID(Input, Output, Setpoint, Kp, Ki, Kd, P_ON_E, ControllerDirection)

int WindowSize = 5000;
unsigned long windowStartTime;
// end PID

void show_LED(int pin, int delayPeriod, bool ledStatus)
{
  if (ledStatus)
  {
    digitalWrite(GREENLED, HIGH);
    Serial.println("The LED is on");
    delay(delayPeriod);
  }
  else
  {
    digitalWrite(GREENLED, LOW);
    Serial.println("The green LED is off");
    delay(delayPeriod);
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(GREENLED, OUTPUT);
  Serial.println("Start Serial active");

  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFont(u8g2_font_helvB10_tf); 
  u8g2.setColorIndex(1);  

  thermocouple = new MAX6675_Thermocouple(SCK_PIN, CS_PIN, SO_PIN);
  // PID
  
  windowStartTime = millis();

  //initialize the variables we're linked to
  Setpoint = 75;    // set the setpoint of the PID 

  //tell the PID to range between 0 and the full window size
  myPID.SetOutputLimits(0, WindowSize);

  //turn the PID on
  myPID.SetMode(AUTOMATIC);

  // set sample time
  myPID.SetSampleTime(1000);      // sampletime in ms

  // clamp output limits
  myPID.SetOutputLimits(0, 10);    //set output values from - to (double Min, double Max);
  // end PID
}

void loop()
{
   // Reads temperature
  double celsius = 50;
  double kelvin;
  double fahrenheit;
  static uint8_t simRelayOnOff = 1;     // for oven simulator
  static double my_output = 0;

  oldTime = millis();
  my_seconds = 0;

  

  show_LED(GREENLED, 400, HIGH);
  show_LED(GREENLED, 300, LOW);

     while (1)   {

// PID
  Input = celsius;
   //myPID.Compute();

  /************************************************
   * turn the output pin on/off based on pid output
   ************************************************/
  // if (millis() - windowStartTime > WindowSize)
  // { //time to shift the Relay Window
  //   windowStartTime += WindowSize;
  // }
  // if (Output < millis() - windowStartTime) 
  //   simRelayOnOff = 0;
  // else {
  //   simRelayTime = 0;
  //   simRelayOnOff = 1;
  // }
//  Serial.print("Output = ");
//  Serial.println(Output);



// PID

       if((millis() - oldTime) > 1000)    // Only process counters once per second
      {
        my_seconds += (millis() - oldTime) % 1000;
        oldTime = millis();
        itoa(my_seconds, timer_buff, 10);   // calculate seconds for timer display

        // Reads temperature
        //celsius = thermocouple->readCelsius();      // only comment for simulation
        simOvenStarted = 1;
        // if (celsius > 50)
        // {
        //   simRelayOnOff = 0;
        // }
         myPID.Compute();
         Serial.print("Output = ");
        Serial.println(Output);
        // my_output = Output;
          if (Output < 4) 
            simRelayOnOff = 0;
          else {
            //simRelayTime = 0;
            simRelayOnOff = 1;
          }


        oven_simulator(simRelayOnOff,  &celsius);     // comment this if simulation if over
        kelvin = thermocouple->readKelvin();
        fahrenheit = thermocouple->readFahrenheit();

        // Output of information
        Serial.print("Temperature: ");
        Serial.print(celsius);
        Serial.print(" C, ");
        Serial.print(kelvin);
        Serial.print(" K, ");
        Serial.print(fahrenheit);
        Serial.println(" F");
        Serial.print(my_seconds);
        Serial.println(" time since start ");

        // test to set new setpoint after period of 100seconds
        if (my_seconds > 85)
        {
          Setpoint = 100;
        }
        if (my_seconds > 120)
        {
          Setpoint = 60;
        }
        
              
        // print on ST7920 LCD display
        dtostrf(celsius, 6, 2, celsius_buff);
        dtostrf(fahrenheit, 8, 2, fahrenheit_buff);
        
        u8g2.firstPage();
        do {   
          draw();
        } while( u8g2.nextPage() );
     } // end if millis
  } // end while
}

// ST7920 LCD with U8g2 library
void draw(){

  u8g2.drawFrame(0,0,128,31);         
  u8g2.drawFrame(0,33,128,31);           
  
  u8g2.drawStr( 15, 13, "Oven Celsius");   
  u8g2.drawStr( 25, 28, celsius_buff);   
  u8g2.drawUTF8(70, 28, DEGREE_SYMBOL);
  u8g2.drawStr(77,28, "C");
  //u8g2.drawUTF8(100, 28, "ml");

  u8g2.drawStr(4,46, "time since start");         // instead of temperature in Fahrenheit it is better to show an timer in the secondline
  u8g2.drawStr( 15, 61, timer_buff); 
  //u8g2.drawUTF8(70, 61, DEGREE_SYMBOL);
  u8g2.drawStr(77,61, "sec");
  //u8g2.drawStr(100,61, "ml");  
}

void oven_simulator(uint8_t simRelayOnOff, double *simCount) {
  
  static uint8_t simOvershootCount;
  static uint8_t simTempPeak;
  
  Serial.println("oven simulator");
  if((millis() - simOldTime) > 1000)    // Only process counters once per second
  {  
    simOldTime = millis();
    Serial.print("oven simulator if entered");
    Serial.println(*simCount);
    if (simOvenStarted)
    {
      Serial.print("oven started");
      if (simRelayOnOff == 1)  {
        Serial.print("simRelay ON !");
        Serial.print("simRelayTime = ");
        Serial.println( simRelayTime );
        (*simCount)++;
        simRelayTime++;
        simTempPeak = false;     
      } else { 
        Serial.print("simRelay OFF !"); 
        Serial.print("simRelayTime = ");
        Serial.println( simRelayTime );      
          if( simTempPeak == false) {
            if ( simRelayTime > 20 )  {
              (*simCount)++;
              simOvershootCount++;
              
            }  else {
              simTempPeak = true;
              simRelayTime = 0;
            }
            if( simOvershootCount > 20) {
                simOvershootCount = 0;
                simTempPeak = true;
              }
          } else {
              if ((*simCount > 20) && (simTempPeak == true)) {
                (*simCount)--;
              }
          }
        }
      }
    }
  }


