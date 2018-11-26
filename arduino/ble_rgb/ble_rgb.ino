// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheral depends on SPI)
#include <SPI.h>
#include <BLEPeripheral.h>
#include <Wire.h>
//#include <Adafruit_ADS1015.h>

// define pins (varies per shield/board)
#define BLE_RDY     2
#define BLE_RST     9
#define BLE_REQ     10

#define LED_GRN     3
#define LED_RED     7

#define RGB_RED     5
#define RGB_GREEN   6
#define RGB_BLUE    3

#define BUTTON      4

//#define ADDR_PIN_0  5
//#define ADDR_PIN_1  6
//#define ADDR_PIN_2  8

//Adafruit_ADS1015 ads;

// create peripheral instance, see pinouts above
BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
// create service
BLEService service = BLEService("aaa0");
// create counter characteristic
BLEUnsignedLongCharacteristic testCharacteristic 
= BLEUnsignedLongCharacteristic("aaa1", BLERead | BLEWrite 
| BLEWriteWithoutResponse | BLENotify /*| BLEIndicate*/);

//BLEUnsignedLongCharacteristic adcValueCharacteristic 
//= BLEUnsignedLongCharacteristic("fff2", BLERead | BLEWrite 
//| BLEWriteWithoutResponse | BLENotify /*| BLEIndicate*/);

// create user description descriptor for characteristic
BLEDescriptor descriptor = BLEDescriptor("2901", "counter");

// last counter update time
unsigned long long lastSent = 0;

volatile int ledState = 0; 
//volatile unsigned int counter = 0; // how many times to blink the led
unsigned long bleCmdValue = 0;
//short adcValue = 0;

void setup() 
{
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GRN, OUTPUT);
    pinMode(BUTTON, INPUT_PULLUP);
    
    digitalWrite(LED_RED, HIGH);
    
    Serial.begin(9600);
#if defined (__AVR_ATmega32U4__)
    delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#endif

    blePeripheral.setLocalName("BLE RGB");
    blePeripheral.setAdvertisedServiceUuid(service.uuid());

    // set device name and appearance
    blePeripheral.setDeviceName("BLE RGB");
    blePeripheral.setAppearance(0x0080);

    // add service, characteristic, and decriptor to peripheral
    blePeripheral.addAttribute(service);
    blePeripheral.addAttribute(testCharacteristic);
    //blePeripheral.addAttribute(adcValueCharacteristic);
    blePeripheral.addAttribute(descriptor);

    // assign event handlers for connected, disconnected to peripheral
    blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
    blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

    // assign event handlers for characteristic
    testCharacteristic.setEventHandler(BLEWritten, characteristicWritten);
    testCharacteristic.setEventHandler(BLESubscribed, characteristicSubscribed);
    testCharacteristic.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);
    //testCharacteristic.setEventHandler(BLENotify, characteristicNotify);

    // set initial value for characteristic
    testCharacteristic.setValue(0);
    //adcValueCharacteristic.setValue(0);

    // begin initialization
    blePeripheral.begin();
    
    Serial.print(F("BLE Peripheral"));
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
     //ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  // ads.setSPS(SPS_128);
  // ads.setSPS(SPS_250);
  // ads.setSPS(SPS_490);
  // ads.setSPS(SPS_920);
     //ads.setSPS(SPS_1600);
  // ads.setSPS(SPS_2400);
  // ads.setSPS(SPS_3300);
  
  //ads.begin();
    
////set timer1 interrupt at 1Hz
//    TCCR1A = 0;// set entire TCCR1A register to 0
//    TCCR1B = 0;// same for TCCR1B
//    TCNT1  = 0;//initialize counter value to 0
//    // set compare match register for 1hz increments
//    OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
//    // turn on CTC mode
//    TCCR1B |= (1 << WGM12);
//    
//    // Set CS10, CS11 and CS12 bits for prescaler
//    // TCCR1B |= (1 << CS10);                // clk / 1 = 16 MHz
//    // TCCR1B |= (1 << CS11);                // clk / 8 = 2 MHz
//    // TCCR1B |= (1 << CS11) | (1 << CS10);  // clk / 64 = 250 kHz
//       TCCR1B |= (1 << CS12);                // clk / 256 = 62 500 Hz
//    // TCCR1B |= (1 << CS12) | (1 << CS10);  // clk / 1024 = 15 625 Hz
//    
//    // enable timer compare interrupt
//    TIMSK1 |= (1 << OCIE1A);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GRN, HIGH);
    delay(250);
    digitalWrite(LED_GRN, LOW);
}

void rgbWrite(byte r, byte g, byte b)
{
    analogWrite(RGB_RED, r);
    analogWrite(RGB_GREEN, g);
    analogWrite(RGB_BLUE, b);
    delay(10);
}

void loop() 
{
    static byte r = 255;
    static byte g = 0;
    static byte b = 0;
    
    for (g = 0; g < 255; g++) // Green 0->255
    {
        rgbWrite(r, g, b);
    }
    for (r = 255; r > 0; r--) // Red 255->0
    {
        rgbWrite(r, g, b);
    }
    for (b = 0; b < 255; b++) // Blue 0->255
    {
        rgbWrite(r, g, b);
    }
    for (g = 255; g > 0; g--) // Green 255->0
    {
        rgbWrite(r, g, b);
    }
    for (r = 0; r < 255; r++) // Red 0->255
    {
        rgbWrite(r, g, b);
    }
    for (b = 255; b > 0; b--) // Blue 255->0
    {
        rgbWrite(r, g, b);
    }
    
    BLECentral central = blePeripheral.central();

    if (central) 
    {
        // central connected to peripheral
        Serial.print(F("Connected to central: "));
        Serial.println(central.address());

        // reset counter value
        //testCharacteristic.setValue(0);

        while (central.connected()) 
        {
            // central still connected to peripheral
            //if (testCharacteristic.written()) 
            //{
                // central wrote new value to characteristic
                //Serial.println(F("counter written, reset"));

                // reset counter value
                //lastSent = 0;
                //testCharacteristic.setValue(0);
            //}
            //if (millis() > 1000 && (millis() - 1000) > lastSent) 
            //{
                // atleast one second has passed since last increment
                //lastSent = millis();

                // increment characteristic value
                //testCharacteristic.setValue(testCharacteristic.value() + 1);

                //Serial.print(F("counter = "));
                //Serial.println(testCharacteristic.value(), DEC);
            //}
            // Handle commands sent from Android
            if (bleCmdValue > 0)
            {
                handleBleCmd(bleCmdValue);
                bleCmdValue = 0;
            }
            
        }
        // central disconnected
        Serial.print(F("Disconnected from central: "));
        Serial.println(central.address());
    }
}

void blePeripheralConnectHandler(BLECentral& central)
{
    // central connected event handler
    Serial.print(F("Connected event, central: "));
    Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLECentral& central) 
{
    // central disconnected event handler
    Serial.print(F("Disconnected event, central: "));
    Serial.println(central.address());
}

void characteristicWritten(BLECentral& central, BLECharacteristic& characteristic)
{
    // characteristic value written event handler
    Serial.print(F("Characteristic event, writen: "));
    Serial.println(testCharacteristic.value(), DEC);
    bleCmdValue = testCharacteristic.value();
}

void characteristicSubscribed(BLECentral& central, BLECharacteristic& characteristic)
{
    // characteristic subscribed event handler
    Serial.println(F("Characteristic event, subscribed"));
}

void characteristicUnsubscribed(BLECentral& central, BLECharacteristic& characteristic)
{
    // characteristic unsubscribed event handler
    Serial.println(F("Characteristic event, unsubscribed"));
}

// timer1 interrupt every 250 ms, toggle led state
#if 0
ISR(TIMER1_COMPA_vect)
{
    if (counter > 0)
    {
        if (ledState)
        {
            digitalWrite(LED_RED, HIGH);
            ledState = 0;
        }
        else
        {
            digitalWrite(LED_RED, LOW);
            ledState = 1;
            counter--;
        }
    }
}
#endif

void handleBleCmd(unsigned long value)
{
    int r, g, b;
    r = value >>  0 & 0xFF;
    g = value >>  8 & 0xFF;
    b = value >> 16 & 0xFF;
    Serial.print(r);
    Serial.print(" ");
    Serial.print(g);
    Serial.print(" ");
    Serial.print(b);
    Serial.print("\n");
    
    analogWrite(RGB_RED, r);
    analogWrite(RGB_GREEN, g);
    analogWrite(RGB_BLUE, b);
}

//void selectChannel(byte channel)
//{
//    digitalWrite(ADDR_PIN_0, (channel & 0x01) >> 0);
//    digitalWrite(ADDR_PIN_1, (channel & 0x02) >> 1);
//    digitalWrite(ADDR_PIN_2, (channel & 0x04) >> 2);
//}





