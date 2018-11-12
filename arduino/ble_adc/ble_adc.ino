// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheral depends on SPI)
#include <SPI.h>
#include <BLEPeripheral.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

#define ENABLE_ADC 0

// define pins (varies per shield/board)
#define BLE_REQ   10
#define BLE_RDY   2
#define BLE_RST   9

#define LED_GRN   3
#define LED_RED   7

Adafruit_ADS1015 ads;

// create peripheral instance, see pinouts above
BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
// create service
BLEService testService = BLEService("fff0");
// create counter characteristic
BLEUnsignedShortCharacteristic testCharacteristic 
= BLEUnsignedShortCharacteristic("fff1", BLERead | BLEWrite 
| BLEWriteWithoutResponse | BLENotify /*| BLEIndicate*/);

// create user description descriptor for characteristic
BLEDescriptor testDescriptor = BLEDescriptor("2901", "counter");

// last counter update time
unsigned long long lastSent = 0;

volatile int ledState = 0; 
volatile unsigned int counter = 0; // how many times to blink the led
byte bleCmdValue = 0;
short adcValue = 0;

void setup() 
{
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GRN, OUTPUT);
    
    digitalWrite(LED_RED, HIGH);
    
    Serial.begin(9600);
#if defined (__AVR_ATmega32U4__)
    delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#endif

    blePeripheral.setLocalName("BLE paske");
    blePeripheral.setAdvertisedServiceUuid(testService.uuid());

    // set device name and appearance
    blePeripheral.setDeviceName("BLE paske");
    blePeripheral.setAppearance(0x0080);

    // add service, characteristic, and decriptor to peripheral
    blePeripheral.addAttribute(testService);
    blePeripheral.addAttribute(testCharacteristic);
    blePeripheral.addAttribute(testDescriptor);

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

    // begin initialization
    blePeripheral.begin();
    
    Serial.print(F("BLE Peripheral"));
  // The ADC input range (or gain) can be changed via the following
  // functions, but be careful never to exceed VDD +0.3V max, or to
  // exceed the upper and lower limits if you adjust the input range!
  // Setting these values incorrectly may destroy your ADC!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
     ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

  // ads.setSPS(SPS_128);
  // ads.setSPS(SPS_250);
  // ads.setSPS(SPS_490);
  // ads.setSPS(SPS_920);
     ads.setSPS(SPS_1600);
  // ads.setSPS(SPS_2400);
  // ads.setSPS(SPS_3300);
  
  ads.begin();
    
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

void loop() 
{
    BLECentral central = blePeripheral.central();

    if (central) 
    {
        // central connected to peripheral
        Serial.print(F("Connected to central: "));
        Serial.println(central.address());

        // reset counter value
        testCharacteristic.setValue(0);

        while (central.connected()) 
        {
            // central still connected to peripheral
            if (testCharacteristic.written()) 
            {
                // central wrote new value to characteristic
                Serial.println(F("counter written, reset"));

                // reset counter value
                lastSent = 0;
                testCharacteristic.setValue(0);
            }
            if (millis() > 1000 && (millis() - 1000) > lastSent) 
            {
                // atleast one second has passed since last increment
                lastSent = millis();

                // increment characteristic value
                //testCharacteristic.setValue(testCharacteristic.value() + 1);

                //Serial.print(F("counter = "));
                //Serial.println(testCharacteristic.value(), DEC);
            }
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

void handleBleCmd(byte value)
{
  int t = (value & 0x40) / (value & 0x40);
  Serial.println(t);
    // Read command
    if (value & 0x40) // 0100 0000
    {        
        Serial.print(F("ADC: "));
        Serial.println(ads.readADC_SingleEnded(0));
    }
    // Gain
    Serial.print(F("Gain set to "));
    switch ((value & 0x38) >> 3) // 0011 1000
    {
        case 1: 
            ads.setGain(GAIN_TWOTHIRDS); 
            Serial.println(F("2/3")); 
            break;
        case 2: 
            ads.setGain(GAIN_ONE);       
            Serial.println(F("1"));   
            break;
        case 3: 
            ads.setGain(GAIN_TWO);       
            Serial.println(F("2"));   
            break;
        case 4: 
            ads.setGain(GAIN_FOUR);      
            Serial.println(F("4"));   
            break;
        case 5: 
            ads.setGain(GAIN_EIGHT);     
            Serial.println(F("8"));   
            break;
        case 6: 
            ads.setGain(GAIN_SIXTEEN);   
            Serial.println(F("16"));  
            break;
        default: 
            break;
    }
    // Samples per second
    Serial.print(F("SPS set to "));
    switch (value & 0x07) // 0000 0111
    {
        case 1: 
            ads.setSPS(SPS_128);
            Serial.println(F("128"));
            break;
        case 2: 
            ads.setSPS(SPS_250);
            Serial.println(F("250"));
            break;
        case 3: 
            ads.setSPS(SPS_490);
            Serial.println(F("490"));
            break;
        case 4: 
            ads.setSPS(SPS_920);
            Serial.println(F("920"));
            break;
        case 5: 
            ads.setSPS(SPS_1600);
            Serial.println(F("1600"));
            break;
        case 6: 
            ads.setSPS(SPS_2400);
            Serial.println(F("2400"));
            break;
        case 7: 
            ads.setSPS(SPS_3300);
            Serial.println(F("3300"));
            break;
        default: 
            break;
    }
}













