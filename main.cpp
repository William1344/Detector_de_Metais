#include <Arduino.h>

bool sound    = true;
bool debug    = true;

int npulse    = 3;
int pin_pulse = A0;
int pin_cap   = A1;
int pin_LED1  = 7;
int pin_LED2  = 6;
int pin_tone  = 4;

void setup() {
  Serial.begin(9600);
  pinMode(pin_cap,        INPUT);  
  pinMode(pin_pulse,      OUTPUT); 
  pinMode(pin_LED1,       OUTPUT);
  pinMode(pin_LED2,       OUTPUT);
  pinMode(pin_tone,       OUTPUT);
  digitalWrite(pin_pulse, LOW);
  digitalWrite(pin_LED1,  LOW);
  digitalWrite(pin_LED2,  LOW);
  digitalWrite(pin_tone,  LOW);
}

int nmeas                     = 256;  //measurements to take
long int sumsum               = 0;    //running sum of 64 sums 
long int skip                 = 0;    //number of skipped sums
long int diff                 = 0;    //difference between sum and avgsum
long int flash_period         = 0;    //period (in ms) 
long unsigned int prev_flash  = 0;    //time stamp of previous flash

void loop() {

  int minval  = 1023;
  int maxval  = 0;
  
  //perform measurement
  long unsigned int sum = 0;
  
  for (int imeas = 0 ; imeas < nmeas+2 ; imeas++){
    //reset the capacitor
    pinMode(pin_cap,      OUTPUT);
    digitalWrite(pin_cap, LOW);
    delayMicroseconds(20);
    pinMode(pin_cap, INPUT);
    //apply pulses
    for (int ipulse = 0 ; ipulse < npulse ; ipulse++) {
      digitalWrite(pin_pulse, HIGH); //takes 3.5 microseconds
      delayMicroseconds(3);
      digitalWrite(pin_pulse, LOW);  //takes 3.5 microseconds
      delayMicroseconds(3);
    }
    //read the charge on the capacitor
    int val = analogRead( pin_cap ); //takes 13x8=104 microseconds
    minval  = min( val, minval );
    maxval  = max( val, maxval );
    sum += val;
    //determine if LEDs should be on or off
    long unsigned int timestamp = millis();
    int ledstat = 0;
    if (timestamp < prev_flash + 10){
      if (diff > 0) ledstat = 1;
      if (diff < 0) ledstat = 2;
    }
    if (timestamp > prev_flash + flash_period){
      if (diff > 0) ledstat = 1;
      if (diff < 0) ledstat = 2;
      prev_flash = timestamp;   
    }
    if (flash_period > 1000) ledstat = 0;

    //switch the LEDs to this setting
    if (ledstat == 0){
      digitalWrite(pin_LED1, LOW);
      digitalWrite(pin_LED2, LOW);
      if(sound) noTone(pin_tone);
    }
    if (ledstat == 1){
      digitalWrite(pin_LED1, HIGH);
      digitalWrite(pin_LED2, LOW);
      if(sound) tone(pin_tone, 2000);
    }
    if (ledstat == 2){
      digitalWrite(pin_LED1,LOW);
      digitalWrite(pin_LED2,HIGH);
      if(sound) tone(pin_tone,500);
    }
  
  }

  //subtract minimum and maximum value to remove spikes
  sum -= minval; 
  sum -= maxval;
  
  //process
  if (sumsum == 0) sumsum = sum << 6; //set sumsum to expected value
  long int avgsum = (sumsum + 32) >> 6; 
  diff = sum - avgsum;
  if (abs(diff) < avgsum >> 10){      //adjust for small changes
    sumsum = sumsum + sum - avgsum;
    skip = 0;
  } else {
    skip++;
  }
  if (skip > 64){     // break off in case of prolonged skipping
    sumsum = sum << 6;
    skip = 0;
  }

  // one permille change = 2 ticks/s
  if (diff == 0) flash_period = 1000000;
  else flash_period = avgsum / ( 2 * abs(diff));    
    
  if (debug){
    Serial.print(nmeas); 
    Serial.print(" ");
    Serial.print(minval); 
    Serial.print(" ");
    Serial.print(maxval); 
    Serial.print(" ");
    Serial.print(sum); 
    Serial.print(" ");
    Serial.print(avgsum); 
    Serial.print(" ");
    Serial.print(diff); 
    Serial.print(" ");
    Serial.print(flash_period); 
    Serial.println();
  }
}
