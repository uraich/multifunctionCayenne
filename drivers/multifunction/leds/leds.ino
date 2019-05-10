/*******************************************************************************
 * leds: a counter on the LEDs
 * Demo program developed for the workshop on Iot at the
 * African Internet Summit 2019, Kampala, Uganda
 * Copyright: U. Raich
 * This program is released under GPL
/*******************************************************************************/

int leds[4] = {10,11,12,13};
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

void displayLEDs(byte value) {
  byte mask=1;
  value %=16;
  Serial.print("Displaying the value ");
  Serial.print(value);
  Serial.println(" on the 4 Leds");
  for (int i=0;i<4;i++) {
//    Serial.print("value, mask: ");
//    Serial.print(value);
//    Serial.print(",");
//     Serial.println(mask);   
    if (value & mask)
      digitalWrite(leds[i],LOW);
    else
      digitalWrite(leds[i],HIGH);     
    mask = mask << 1;
  }  
}

void setup() {
  Serial.begin(115200);
  Serial.println("Multifunction board LED demo program");
  Serial.println("Workshop of IoT, AIS 2019, Kampala, Uganda");
  for (int i=0;i<4;i++)
    pinMode(leds[i],OUTPUT);
  clearDisplay();
}

void loop() {
  displayLEDs(counter);
  delay(1000);
  counter ++;
}
