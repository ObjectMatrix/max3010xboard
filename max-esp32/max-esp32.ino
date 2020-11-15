#include <Wire.h>
#include "MAX30105.h" //sparkfun MAX3010X library
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <HTTPClient.h>

MAX30105 particleSensor;

// #define MAX30105 
// if you have Sparkfun's MAX30105 breakout board , 
// try #define MAX30105 

#define USEFIFO

const char* ssid = "";
const char* password = "";

void sendDataToThingSpeak(double val, String field) {
      const char* serverName = "http://api.thingspeak.com/update";
      String apiKey = "";
      HTTPClient http;
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverName);
      
      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Data to send with HTTP POST
      String httpRequestData = "api_key=" + apiKey + "&"+String(field)+"=" + String(val);           
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.print(field);
      Serial.println(val);
      // Free resources
      http.end();   
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  /**   
   * Initialize sensor over I2C bus
   * 400 kbit fastmode and – since 1998 – a high speed 3.4 Mbit
   * option available. Recently, fast mode plus a transfer rate
   * between this has been specified.
   */
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30102 was not found. Please check wiring/power/solder jumper at MH-ET LIVE MAX30102 board. ");
    while (1);
  }

  /**
   * Debugging
   * setup the plotter (for Arduino based IDE)
   */
  byte ledBrightness = 0x7F; // Options: 0=Off to 255=50mA
  byte sampleAverage = 4; // Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  // Options: 1 = IR only, 2 = Red + IR on MH-ET LIVE MAX30102 board
  int sampleRate = 200; // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; // Options: 69, 118, 215, 411
  int adcRange = 16384; // Options: 2048, 4096, 8192, 16384
  // Set up the wanted parameters, configure sensor with these settings
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); 
}
double avered = 0;
double aveir = 0;
double sumirrms = 0;
double sumredrms = 0;
int i = 0;
int Num = 100; // calculate SpO2 by this sampling interval

double ESpO2 = 95.0;// initial value of estimated SpO2
double FSpO2 = 0.7; // filter factor for estimated SpO2
double frate = 0.95; // low pass filter for IR/red LED value to eliminate AC component
#define TIMETOBOOT 3000 // wait for this time(msec) to output SpO2
#define SCALE 88.0 // adjust to display heart beat and SpO2 in the same scale
#define SAMPLING 5 // if you want to see heart beat more precisely , set SAMPLING to 1
#define FINGER_ON 30000 // if red signal is lower than this , it indicates your finger is not on the sensor
#define MINIMUM_SPO2 80.0

void loop()
{
  uint32_t ir, red , green;
  double fred, fir;
  /**
   * raw SpO2 before low pass filtered (LPF)
   * LPF is a filter that passes signals
   * with a frequency lower than a selected cutoff frequency and attenuates
   * signals with frequencies higher than the cutoff frequency.
   */ 
  double SpO2 = 0; 

#ifdef USEFIFO
  particleSensor.check(); // Check the sensor, read up to 3 samples

  while (particleSensor.available()) { // do we have new data
#ifdef MAX30105
    red = particleSensor.getFIFORed(); // Sparkfun's MAX30105
    ir = particleSensor.getFIFOIR();  // Sparkfun's MAX30105
#else
    red = particleSensor.getFIFOIR(); // why getFOFOIR output Red data by MAX30102 on MH-ET LIVE breakout board
    ir = particleSensor.getFIFORed(); // why getFIFORed output IR data by MAX30102 on MH-ET LIVE breakout board
#endif
    i++;
    fred = (double)red;
    fir = (double)ir;
    avered = avered * frate + (double)red * (1.0 - frate);// average red level by low pass filter
    aveir = aveir * frate + (double)ir * (1.0 - frate); // average IR level by low pass filter
    sumredrms += (fred - avered) * (fred - avered); // square sum of alternate component of red level
    sumirrms += (fir - aveir) * (fir - aveir); // square sum of alternate component of IR level
    if ((i % SAMPLING) == 0) { // slow down graph plotting speed for arduino Serial plotter by thin out
      if ( millis() > TIMETOBOOT) {
        float ir_forGraph = (2.0 * fir - aveir) / aveir * SCALE;
        float red_forGraph = (2.0 * fred - avered) / avered * SCALE;
        // trancation for Serial plotter's autoscaling
        if ( ir_forGraph > 100.0) ir_forGraph = 100.0;
        if (ESpO2 > 100.0) ESpO2 = 100.0;
        if ( ir_forGraph < 80.0) ir_forGraph = 80.0;
        if ( red_forGraph > 100.0 ) red_forGraph = 100.0;
        if ( red_forGraph < 80.0 ) red_forGraph = 80.0;
        // Serial.print(red); Serial.print(","); Serial.print(ir);Serial.print(".");
        if (ir < FINGER_ON) ESpO2 = MINIMUM_SPO2; //indicator for finger detached
        if (ir >= FINGER_ON) {
        //        Serial.print(ir_forGraph); // to display pulse wave at the same time with SpO2 data
        //        Serial.print(","); Serial.print(red_forGraph); // to display pulse wave at the same time with SpO2 data
        //        Serial.print(",");
        
                  
                  Serial.print("sendDataToThingSpeak: ");
                  // float pulses = (2.0 * fred - avered) / avered * SCALE;
                  float pulses = (2.0 * fir - aveir) / aveir * SCALE;
                  Serial.println(ESpO2); // low pass filtered SpO2
                  // Serial.println(pulses);
                  String field1 = "field1";
                  sendDataToThingSpeak(ESpO2, field1);
                  // String field2 = "field2";
                  // sendDataToThingSpeak(pulses, field2);
        }
        //        Serial.print(","); Serial.print(85.0); // reference SpO2 line
        //        Serial.print(","); Serial.print(90.0); // warning SpO2 line
        //        Serial.print(","); Serial.print(95.0); // safe SpO2 line
        //        Serial.print(","); Serial.println(100.0); // max SpO2 line
      }
    }
    if ((i % Num) == 0) {
      double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
      // Serial.println(R);
      SpO2 = -23.3 * (R - 0.4) + 100; // source: http://ww1.microchip.com/downloads/jp/AppNotes/00001525B_JP.pdf
      ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2;// low pass filter
      // Serial.print(SpO2);Serial.print(","); Serial.println(ESpO2);
      sumredrms = 0.0;
      sumirrms = 0.0;
      i = 0;
      break;
    }
    // move to next sample
    particleSensor.nextSample(); 
  }
#else
  // // do we have new data from sensors
  while (1) {
  #ifdef MAX30105
    red = particleSensor.getRed();  // Sparkfun's MAX30105
      ir = particleSensor.getIR();  // Sparkfun's MAX30105
  #else
      red = particleSensor.getIR(); // why getFOFOIR outputs Red data by MAX30102 on MH-ET LIVE breakout board
      ir = particleSensor.getRed(); // why getFIFORed outputs IR data by MAX30102 on MH-ET LIVE breakout board
  #endif
    i++;
    fred = (double)red;
    fir = (double)ir;
    avered = avered * frate + (double)red * (1.0 - frate); // average red level by low pass filter
    aveir = aveir * frate + (double)ir * (1.0 - frate); // average IR level by low pass filter
    sumredrms += (fred - avered) * (fred - avered); // square sum of alternate component of red level
    sumirrms += (fir - aveir) * (fir - aveir); // square sum of alternate component of IR level
    if ((i % SAMPLING) == 0) { // slow down graph plotting speed for arduino IDE toos menu by thin out
      //#if 0
      if ( millis() > TIMETOBOOT) {
        float ir_forGraph = (2.0 * fir - aveir) / aveir * SCALE;
        float red_forGraph = (2.0 * fred - avered) / avered * SCALE;
        //trancation for Serial plotter's autoscaling
        if ( ir_forGraph > 100.0) ir_forGraph = 100.0;
        if ( ir_forGraph < 80.0) ir_forGraph = 80.0;
        if ( red_forGraph > 100.0 ) red_forGraph = 100.0;
        if ( red_forGraph < 80.0 ) red_forGraph = 80.0;
        // Serial.print(red); Serial.print(","); Serial.print(ir);Serial.print(".");
        if (ir < FINGER_ON) ESpO2 = MINIMUM_SPO2; //indicator for finger detached
        if (ir >= FINGER_ON) {
        // Serial.print((2.0 * fir - aveir) / aveir * SCALE); // to display pulse wave at the same time with SpO2 data
        // Serial.print(","); Serial.print((2.0 * fred - avered) / avered * SCALE); // to display pulse wave at the same time with SpO2 data
        // Serial.print(",");
        // float pulses = (2.0 * fred - avered) / avered * SCALE;
          float pulses = (2.0 * fir - aveir) / aveir * SCALE;
          Serial.print("sendDataToThingSpeak: ");
          Serial.println(ESpO2); // low pass filtered SpO2
          // Serial.println(pulses); 
          String field1 = "field1";
          sendDataToThingSpeak(ESpO2, field1);
        // String field2 = "field2";
        // sendDataToThingSpeak(pulses, field2);
        }
        //        Serial.print(","); Serial.print(85.0); //
        //        Serial.print(","); Serial.print(90.0); // warning SpO2 line
        //        Serial.print(","); Serial.print(95.0); // safe SpO2 line
        //        Serial.print(","); Serial.println(100.0); // max SpO2 line
        //#endif
      }
    }
    if ((i % Num) == 0) {
      double R = (sqrt(sumredrms) / avered) / (sqrt(sumirrms) / aveir);
      // Serial.println(R);
      SpO2 = -23.3 * (R - 0.4) + 100; // http://ww1.microchip.com/downloads/jp/AppNotes/00001525B_JP.pdf
      ESpO2 = FSpO2 * ESpO2 + (1.0 - FSpO2) * SpO2;
      //  Serial.print(SpO2);Serial.print(",");Serial.println(ESpO2);
      sumredrms = 0.0; sumirrms = 0.0; i = 0;
      break;
    }
    // We're finished with this sample so move to next sample
    particleSensor.nextSample();
  }
  #endif
}
