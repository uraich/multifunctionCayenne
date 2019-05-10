/*******************************************************************************
 * switches: A program reading out the user switches
 * Demo program developed for the workshop on Iot at the
 * African Internet Summit 2019, Kampala, Uganda
 * Copyright: U. Raich
 * This program is released under GPL
/*******************************************************************************/

int switches[] = {A1,A2,A3};
int counter = 0;
int latchPin = 4;
int clockPin = 7;
int dataPin  = 8; // pins connected to the shift registers
unsigned char clearCode   = 0xff;
unsigned char Dis_buf[]   = {0xF1,0xF2,0xF4,0xF8};

void clearDisplay() {
  pinMode(latchPin,OUTPUT);
  pinMode(clockPin,OUTPUT);
  pinMode(dataPin,OUTPUT);                                       // set the 7-segment control pins to output
  for(int i=0; i<=3; i++)                                        // loop over the 4 digits
  {
    digitalWrite(latchPin,LOW);                                  // don't latch
    shiftOut(dataPin,clockPin,MSBFIRST,clearCode);               // address the digt
    shiftOut(dataPin,clockPin,MSBFIRST,Dis_buf[i] );             // the LED code for the digit
    digitalWrite(latchPin,HIGH);                                 // latch the data   
    delay(2); 
  }
  delay(500);
}

void setup() {
  clearDisplay();
  Serial.begin(115200);
  Serial.println("Read out the switch closures and return them as a 3 bit number");
  Serial.println("Uli Raich for the");
  Serial.println("Workshop of IoT, AIS 2019, Kampala, Uganda");

  for (int i=0;i<3;i++) 
    pinMode(switches[i],INPUT);
}

void loop() {
  byte value = 0;
  byte mask = 4;
  for (int i=0;i<3;i++) {
    if (!digitalRead(switches[i]))
      value |=mask;
    mask >>= 1;
  }
  Serial.print("Switch value: ");
  Serial.println(value);
  delay(1000);
}
  
  
