/*
multifunction.ino
A program to connection the Arduino multifunction board to Cayenne
Written for the worksgop on Iot at the African Internet Summit 2019, Kampala, Uganda
This program is based on the Basic Demo program from the Cayenne-MQTT-Arduino library
 
The programm connects to Cayenne using an ESP8266 based WiFi shield and sends/receives data from the multifunction board
The following devices are implemented:
1. 4 user LEDs
2. 2 push buttons implemented as a 3 bit number
3. seven segment display. This can be used as a 4 byte hex display or as a floating point display
4. potentiometer voltage measures with the AVR 10 bit ADC

The CayenneMQTT Library is required to run this sketch. If you have not already done so you can install it from the Arduino IDE Library Manager.

Steps:
1. Set the Cayenne authentication info to match the authentication info from the Dashboard.
2. Set the network name and password.
3. Compile and upload the sketch.
4. A temporary widget will be automatically generated in the Cayenne Dashboard. To make the widget permanent click the plus sign on the widget.
*/

#include <ESP8266SerialLibrary.h>
#include <avr/pgmspace.h>


#define CAYENNE_DEBUG          // Uncomment to show debug messages
#define CAYENNE_PRINT Serial1  // Comment this out to disable prints and save space

#if !defined(SERIAL_PORT_HARDWARE1) && defined(CAYENNE_PRINT)
#include <SoftwareSerial.h>
#define RX_PIN 10
#define TX_PIN 11
SoftwareSerial Serial1 = SoftwareSerial(RX_PIN,TX_PIN);
#endif

#include <CayenneMQTTESP8266Shield.h>

// WiFi network info.

const char ssid[] = "WIFI_SSID";
const char wifiPassword[] = "WIFI_PASSWORD";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
const char username[] = "CAYENNE_USERNAME";
const char password[] = "CAYENNE_PASSWORD";
const char clientID[] = "CAYENNE_CLIENT_ID";

#define SWITCHES_CHANNEL        0
#define LED_CHANNEL             1
#define SEVEN_SEG_HEX_CHANNEL   2
#define SEVEN_SEG_ON_CHANNEL    3
#define SEVEN_SEG_FLOAT_CHANNEL 4
#define POT_CHANNEL             5

//
// definitions for the multifunction board hardware interface
//
int switches[] = {A1,A2,A3};
int leds[4] = {10,11,12,13};
int pot = A0;

int counter = 0;
int updateCounter = 0;
int latchPin = 4;
int clockPin = 7;
int dataPin  = 8; // pins connected to the shift registers
unsigned char clearCode   = 0xff;
unsigned char minus       = 0xbf;
unsigned char rCode       = 0xaf;
unsigned char Dis_buf[]   = {0xF1,0xF2,0xF4,0xF8};
unsigned char Dis_table[] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0X80,0X90,0x88,0x83,0xc6,0xa1,0x86,0x8e};
// LED coding for hex numbers
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
bool displayActive=false;
bool displayFloat = false;          // when this flag is false the seven segment display takes heex values
                                    // if it is true it takes float values
float displayFloatValue=0.0;
uint16_t displayHexValue=0;

// When starting, the contents of the register driving the seven segment display is undefined
// producing some random display. We clear this out

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
//    delay(2); 
  }
  delay(500);
}
/*
 * updates the seven segment display 
 * this should be done every 5 ms in order to get a stable, non flickering display
 */

void updateDisplay() {
  unsigned char value;
  if (!displayActive && !displayFloat) {
//    CAYENNE_PRINT.println("display is not active");
    return;
  }
  digitalWrite(latchPin,LOW);                                             // don't latch
                                                                          // shift in the 16 bits, most significant bit first
  //CAYENNE_PRINT.println(digitIndex);
  //CAYENNE_PRINT.println(digit[digitIndex],HEX);
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
  //for (int i=0;i <4;i++)
    //CAYENNE_PRINT.print(digit[i]);
  //CAYENNE_PRINT.println();
  digitIndex++;
  currentDigit++;
  if (digit[digitIndex] == '\0') {
    digitIndex = 0;
    currentDigit = 0;
  }
  updateCounter ++;
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
#ifdef CAYENNE_PRINT
    CAYENNE_PRINT.println("value too small");
#endif
    return false;
  }
if (val > 9999) {  
#ifdef CAYENNE_PRINT    
    CAYENNE_PRINT.println("value too big");
#endif    
    return false;
  }
#ifdef CAYENNE_PRINT
  CAYENNE_PRINT.print(val);
  CAYENNE_PRINT.println(" is a valid number");
#endif
  dtostrf(val,5,3,floatString);
  strncpy(digit,floatString,5);  
#ifdef CAYENNE_PRINT  
  CAYENNE_PRINT.print("floatString: ");
  CAYENNE_PRINT.println(floatString);
#endif
  digit[5] = '\0';
#ifdef CAYENNE_PRINT   
  CAYENNE_PRINT.print("digitString: ");
  CAYENNE_PRINT.println(digit);
#endif  
  return true;
}

void displayLEDs(byte value) {
  byte mask=1;
  value %=16;
#ifdef CAYENNE_PRINT 
  CAYENNE_PRINT.print("Displaying the value ");
  CAYENNE_PRINT.print(value);
  CAYENNE_PRINT.println(" on the 4 Leds");
#endif
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
  
  clearDisplay();
  // write 0 to seven segment display
  writeHex(0);
  // setup the GPIO lines
  // first the push buttons

  // init GPIOs for switches
  for (int i=0;i<3;i++) 
    pinMode(switches[i],INPUT);
    
  // init GPIOs for LEDs  
  for (int i=0;i<4;i++)
    pinMode(leds[i],OUTPUT); 

  // init the pin for potentiometer readout
  pinMode(pot,INPUT);
  
  // clear the LEDs
  displayLEDs(0);     
  // initialize serial line for communication with WiFi module

#if defined(CAYENNE_PRINT)
	CAYENNE_PRINT.begin(115200);
  CAYENNE_PRINT.println("Cayenne program giving access to all devices on the multifunction board");
  CAYENNE_PRINT.println("Uli Raich for the");
  CAYENNE_PRINT.println("Workshop of IoT, AIS 2019, Kampala, Uganda");
  CAYENNE_LOG("Starting program");
#endif

  Serial.begin(9600);
  ESP8266 wifi=ESP8266(&Serial);
  wifi.restart();
  //CAYENNE_PRINT.println(wifi.getVersion());
  Cayenne.begin(username, password, clientID, wifi, ssid, wifiPassword);
  // Timer2 is used for seven segment display update 
  OCR2A = 0xAF;
  TIMSK2 |= _BV(OCIE0A);
  TCCR2B |= 0x2;
}

// Interrupt is called once every 8 milliseconds, 
SIGNAL(TIMER2_COMPA_vect)  {
  updateDisplay();
}

void loop() {
  unsigned long start;
//  start = millis();
  Cayenne.loop();
/*  
  CAYENNE_PRINT.print("in loop: ");
  CAYENNE_PRINT.print(millis() - start);
  CAYENNE_PRINT.println(" ms");
*/
}

// Default function for sending sensor data at intervals to Cayenne.
// You can also use functions for specific channels, e.g CAYENNE_OUT(1) for sending channel 1 data.
CAYENNE_OUT_DEFAULT()
{
  byte switchValue = 0;
  uint16_t potValue;
  bool displayState;
  byte mask = 4;
  for (int i=0;i<3;i++) {
    if (!digitalRead(switches[i]))
      switchValue |=mask;
    mask >>= 1;
  }
  potValue = analogRead(pot);
#ifdef CAYENNE_PRINT  
  CAYENNE_PRINT.print("Switch value: ");
  CAYENNE_PRINT.println(switchValue);
  CAYENNE_PRINT.print("pot value: ");
  CAYENNE_PRINT.print(potValue);
  CAYENNE_PRINT.println(" mV");
  
#endif 
  // Write data to Cayenne here. This example just sends the current uptime in milliseconds on virtual channel 0.
  
  // switch off display update until data are sent

  Cayenne.virtualWrite(SWITCHES_CHANNEL, switchValue);
  Cayenne.voltageWrite(POT_CHANNEL,(float)potValue * 3300.0 /1024.0);
  displayFloatValue = (float)potValue * 3.30 /1024.0;
  CAYENNE_PRINT.print("Voltage: ");
  CAYENNE_PRINT.print(displayFloatValue);
  CAYENNE_PRINT.println(" V");
  if (displayFloat)
    writeFloat((float)potValue * 3.30 /1024.0);
  // Some examples of other functions you can use to send data.
  // Cayenne.celsiusWrite(1, 22.0);
  // Cayenne.luxWrite(2, 700);
  // Cayenne.virtualWrite(3, 50, TYPE_PROXIMITY, UNIT_CENTIMETER);

}

// Default function for processing actuator commands from the Cayenne Dashboard.
// You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.
CAYENNE_IN_DEFAULT()
{
  int ledValue,sevenSegOnOff,floatOnOff;
  long hexValue;
  String hexString;
  CAYENNE_LOG("Channel %u, value %s", request.channel, getValue.asString());
  switch (request.channel) {
  case LED_CHANNEL:
    ledValue = getValue.asInt();
    displayLEDs(ledValue);
    break;
  case SEVEN_SEG_ON_CHANNEL:
    sevenSegOnOff = getValue.asInt();
    if (sevenSegOnOff)
      displayActive = true;
    else {
      displayActive = false;
      clearDisplay();
    }
    break;
  case SEVEN_SEG_FLOAT_CHANNEL:
    floatOnOff = getValue.asInt();
    CAYENNE_PRINT.print("float on/off: ");
    CAYENNE_PRINT.println(floatOnOff);
    if (floatOnOff == 0) {
      displayFloat = false;
      CAYENNE_PRINT.print("Float off");
      writeHex(displayHexValue);
      if (!displayActive)
          clearDisplay();
    }
    else {
      displayFloat = true;
      CAYENNE_PRINT.print("Float on");
      writeFloat(displayFloatValue);
    }
    break;
  case SEVEN_SEG_HEX_CHANNEL:
    if (displayFloat)
      break;  
    hexString = getValue.asString();
    hexValue = getValue.asLong();
    CAYENNE_PRINT.print("hexValue: ");
    CAYENNE_PRINT.println(hexValue,HEX);
    CAYENNE_PRINT.print("hexString: ");
    CAYENNE_PRINT.println(hexString);
    if (hexValue == -1) {
      displayActive = false;
      clearDisplay();
    }
    else {
      displayActive = true;
      writeHex((uint16_t)hexValue);
      displayHexValue = (uint16_t)hexValue;
    }
    break;
  }
}
