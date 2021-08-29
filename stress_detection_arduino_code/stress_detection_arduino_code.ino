#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into digital pin 2 on the Arduino
#define ONE_WIRE_BUS 2
// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);  
// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);
const int ecgPin = A0;
int upperThreshold = 100;
int lowerThreshold = 0;
int ecgOffset = 400;
float beatsPerMinute = 0.0;
bool alreadyPeaked = false;
unsigned long firstPeakTime = 0;
unsigned long secondPeakTime = 0;
unsigned long rrInterval = 0;
int numRRDetected = 0;
bool hrvStarted = false;
bool hrvUpdate = false;
bool hrvComplete = false;
unsigned long hrvStartTime = 0;
unsigned long rrIntervalPrevious = 0;
float rrDiff = 0.0;
float rrDiffSquaredTotal = 0.0;
float diffCount = 0.0;
float rmssd = -1.0;
const int GSR=A1;

int sensorValue=0;

int gsr_average=0;

const int groundpin = 18;             // analog input pin 4 -- ground

const int powerpin = 19;              // analog input pin 5 -- voltage

const int xpin = A3;                  // x-axis of the accelerometer

const int ypin = A2;                  // y-axis

const int zpin = A1;                  // z-axis (only on 3-axis models)
void setup() {
  
  sensors.begin();
  Serial.begin(9600);
  pinMode(10, INPUT); // Setup for leads off detection LO +
  pinMode(11, INPUT);
  Serial.println("CLEARDATA");
  Serial.println("LABEL,time,timer,ecg1,ecg2,ecg3,ecg4,x,y,z,gsr,temp-c,temp-f");
  //pinMode(LED_BUILTIN, OUTPUT);
   pinMode(groundpin, OUTPUT);

  pinMode(powerpin, OUTPUT);
 

  digitalWrite(groundpin, LOW);

  digitalWrite(powerpin, HIGH);
}

void loop() { 
  int ecgReading = analogRead(ecgPin) - ecgOffset; 
  // Measure the ECG reading minus an offset to bring it into the same
  // range as the heart rate (i.e. around 60 to 100 bpm)

  if (ecgReading > upperThreshold && alreadyPeaked == false) { 
  // Check if the ECG reading is above the upper threshold and that
  // we aren't already in an existing peak
      if (firstPeakTime == 0) {
        // If this is the very first peak, set the first peak time
        firstPeakTime = millis();
        digitalWrite(LED_BUILTIN, HIGH);
      }
      else {
        // Otherwise set the second peak time and calculate the 
        // R-to-R interval. Once calculated we shift the second
        // peak to become our first peak and start the process
        // again
        secondPeakTime = millis();
        // Store the previous R-to-R interval for use in HRV calculations
        rrIntervalPrevious = rrInterval;
        // Calculate new R-to-R interval and set HRV update flag
        rrInterval = secondPeakTime - firstPeakTime;
        firstPeakTime = secondPeakTime;
        hrvUpdate = true;
        numRRDetected = numRRDetected + 1;
        digitalWrite(LED_BUILTIN, HIGH);
      }
      alreadyPeaked = true;
  }

  if (ecgReading < lowerThreshold) {
  // Check if the ECG reading has fallen below the lower threshold
  // and if we are ready to detect another peak
    alreadyPeaked = false;
    digitalWrite(LED_BUILTIN, LOW);
  }  

  // Calculate the beats per minute, rrInterval is measured in
  // milliseconds so we must multiply by 1000
  beatsPerMinute = (1.0/rrInterval) * 60.0 * 1000;
  
  // Once two consecutive R-to-R intervals have been detected, 
  // start the 5 minute HRV window
  if (!hrvStarted && numRRDetected >= 2) {
    hrvStarted = true;
    hrvStartTime = millis();
  }
  
  // If a new R-to-R interval has been detected, update the HRV measure
  if (hrvUpdate && hrvStarted) {
    // Add the square of successive differences between 
    // R-to-R intervals to the running total
    rrDiff = float(rrInterval) - float(rrIntervalPrevious);
    rrDiffSquaredTotal = rrDiffSquaredTotal + sq(rrDiff);
    // Count the number of successive differences for averaging
    diffCount = diffCount + 1.0;
    // Reset the hrvUpdate flag
    hrvUpdate = false;
  }

  // Once five minute window has elapsed, calculate the RMSSD
  if (millis() - hrvStartTime >= 60000 && !hrvComplete) {
    rmssd = sqrt(rrDiffSquaredTotal/diffCount);
    hrvComplete = true;
  } 

  // Print the final values to be read by the serial plotter
  // RMSSD will print -1 until the 5 minute window has elapsed
  // Important notes on using this code:
  // 1. RMSSD can be significantly affected by bad sensor
  // readings in short term recordings so ensure to check for 
  // incorrectly detected RR intervals if you are getting 
  // very large RMSSD values.
  // 2. Due to the function of the internal Arduino clock, the 
  // real-world elapsed time for HRV computation may exceed 5 minutes. 
  // This should not make a practical difference for RMSSD calculation, 
  // but it is important to note if critical precision is required.
  
  //GRS CODE
   long sum=0;
  for(int i=0;i<10;i++)           //Average the 10 measurements to remove the glitch
      {
      sensorValue=analogRead(GSR);
      sum += sensorValue;
      delay(5);
      }
   gsr_average = sum/10;

//Requesting temperature
  sensors.requestTemperatures(); 

  
 //Printing values
  Serial.print("DATA,TIME,TIMER,");
  Serial.print(analogRead(ecgPin));
  Serial.print(",");
  Serial.print(ecgReading);
  Serial.print(",");
  Serial.print(beatsPerMinute);
  Serial.print(",");
  Serial.print(rmssd);
  Serial.print(",");
  Serial.print(analogRead(xpin));
  Serial.print(",");
  Serial.print(analogRead(ypin));
  Serial.print(",");
  Serial.print(analogRead(zpin));
  Serial.print(",");
  Serial.print(gsr_average);
  Serial.print(",");
  Serial.print(sensors.getTempCByIndex(0));
  //print the temperature in Celsius
  Serial.print(",");
  //print the temperature in Fahrenheit
  Serial.print((sensors.getTempCByIndex(0) * 9.0) / 5.0 + 32.0);
  Serial.println();
  delay(6000);
 
}
