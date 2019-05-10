 
/*******************************************************************************
 * sevenSegment: functions to write hex or floating oint values to the 
 * seven segment display
 * Demo program developed for the workshop on Iot at the
 * African Internet Summit 2019, Kampala, Uganda
 * Copyright: U. Raich
 * This program is released under GPL
/*******************************************************************************/
#include <string.h>

int latchPin = 4;
int clockPin = 7;
int dataPin  = 8; // pins connected to the shift registers
byte counter = 0;
unsigned char clearCode   = 0xff;
unsigned char minus       = 0xbf;
unsigned char rCode       = 0xaf;
unsigned char Dis_buf[]   = {0xF1,0xF2,0xF4,0xF8};
unsigned char Dis_table[] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0X80,0X90,0x88,0x83,0xc6,0xa1,0x86,0x8e};	    // LED coding for hex numbers
//          0
//       -------
//      |       |
//    5 |       | 1
//      |   6   |
//       -------
//      |       |
//    4 |       | 2
//      |   3   |
//       -------
//      
char digit[6];
int currentDigit=0;
int digitIndex = 0;
bool negative=false;
bool displayActive=true;
bool printed=false;

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

/*
 * updates the seven segment display 
 * this should be done every 8 ms in order to get a stable, non flickering display
 */

void updateDisplay() {
  unsigned char value;
  int start = millis();
  if (!displayActive)
    return;
  digitalWrite(latchPin,LOW);                                             // don't latch
                                                                          // shift in the 16 bits, most significant bit first
//  Serial.println(digitIndex);
//  Serial.println(digit[digitIndex],HEX);
/*
 * if minus sign
 */

  if (digit[digitIndex] == '-')
    value = minus;
/*
 * if 'r' in err
 */
  else if (digit[digitIndex] == 'r') 
    value = rCode;
/*
 * if between 0 and 9
 */

  else if ((digit[digitIndex] >= '0') && (digit[digitIndex] <= '9'))
    {
      
//      Serial.print("digit(0..9): ");
//      Serial.println(digit[digitIndex]);
      if (digit[currentDigit+1] == '.') {
        value = Dis_table[digit[digitIndex] - '0'] & 0x7f;
        digitIndex ++;
      }
      else 
         value = Dis_table[digit[digitIndex] - '0'];
    }   
/*         
 * if between a and f (for hex numbers
 */
    else if ((digit[digitIndex]>= 'a') && (digit[digitIndex] <= 'f'))
      value = Dis_table[digit[digitIndex]-0x57];
 
  shiftOut(dataPin,clockPin,MSBFIRST,value);                              // address the digit
  shiftOut(dataPin,clockPin,MSBFIRST,Dis_buf[currentDigit] );             // the LED code for the digit
  digitalWrite(latchPin,HIGH);                                            // latch the data
  digitIndex++;
  currentDigit++;
  if (digit[digitIndex] == '\0') {
    digitIndex = 0;
    currentDigit = 0;
  }
  if (!printed) {
    printed = true;
    Serial.print("Duration of ISR: ");
    Serial.print(millis() - start);
    Serial.println(" ms");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Multifunction board Seven Segment demo program");
  Serial.println("Uli Raich for the");
  Serial.println("Workshop of IoT, AIS 2019, Kampala, Uganda");
  clearDisplay();
  
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function below
  OCR2A = 0xAF;
  TIMSK2 |= _BV(OCIE0A);
  TCCR2B |= 0x2;   // prescaler: interrupt every 8 ms
  //TIMSK2 = (TIMSK2 & B11111110) | 0x01;
  //TCCR2B = (TCCR2B & B11111000) | 0x02;
}

// Interrupt is called once a millisecond, 
SIGNAL(TIMER2_COMPA_vect)  {
  updateDisplay();
}

ISR(TIMER2_OVF_vect) {
  updateDisplay();
}
/*
 * write a 16 bit hex value to the display
 */
void writeHex(uint16_t value) {
  sprintf(digit,"%04x",value);
}
/*
 * write a floating point value to the display
 */
bool writeFloat(float val) {
  char floatString[10];  
  if (val < -999) {
    Serial.println("value too small");
    return false;
  }
  Serial.print(val);
  Serial.println(" is a valid number");
  if (val > 9999) {
    Serial.println("value too big");
    return false;
  }
  dtostrf(val,5,3,floatString);
  strncpy(digit,floatString,5);  
  Serial.print("floatString: ");
  Serial.println(floatString);
  digit[5] = '\0';
  Serial.print("digitString: ");
  Serial.println(digit);
  return true;
}

void loop() {

  writeHex(0xabcd);
  delay(1000);
  writeFloat(-13.2);
  delay(1000);
  writeFloat(3.01); 
  delay(1000);
  writeFloat(12.34);
  delay(1000);
  writeFloat(1234);
  delay(1000);
}
