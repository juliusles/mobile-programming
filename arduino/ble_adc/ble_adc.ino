// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheral depends on SPI)
#include <SPI.h>
#include <BLEPeripheral.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

// define pins (varies per shield/board)
#define BLE_RDY     2
#define BLE_RST     9
#define BLE_REQ     10

#define LED_GRN     3
#define LED_RED     7

#define ADDR_PIN_0  5
#define ADDR_PIN_1  6
#define ADDR_PIN_2  8

//#define ANDROID

Adafruit_ADS1015 ads;

#ifdef ANDROID
// create peripheral instance, see pinouts above
BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
// create service
BLEService service = BLEService("fff0");
// create counter characteristic
BLEUnsignedShortCharacteristic testCharacteristic 
= BLEUnsignedShortCharacteristic("fff1", BLERead | BLEWrite 
| BLEWriteWithoutResponse | BLENotify /*| BLEIndicate*/);

BLEUnsignedShortCharacteristic adcValueCharacteristic 
= BLEUnsignedShortCharacteristic("fff2", BLERead | BLEWrite 
| BLEWriteWithoutResponse | BLENotify /*| BLEIndicate*/);

// create user description descriptor for characteristic
BLEDescriptor descriptor = BLEDescriptor("2901", "counter");

// Command received from Android
byte bleCmdValue = 0;
#else // Serial command
enum setOptions
{
    Read,
    Gain,
    Sps
};
#endif

void setup() 
{
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GRN, OUTPUT);
    
    digitalWrite(LED_RED, HIGH);
    
    Serial.begin(9600);
#ifdef ANDROID
    blePeripheral.setLocalName("BLE paske");
    blePeripheral.setAdvertisedServiceUuid(service.uuid());

    // set device name and appearance
    blePeripheral.setDeviceName("BLE paske");
    blePeripheral.setAppearance(0x0080);

    // add service, characteristic, and decriptor to peripheral
    blePeripheral.addAttribute(service);
    blePeripheral.addAttribute(testCharacteristic);
    blePeripheral.addAttribute(adcValueCharacteristic);
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
    adcValueCharacteristic.setValue(0);

    // begin initialization
    blePeripheral.begin();
    
    Serial.print(F("BLE Peripheral"));
#endif // ANDROID
    
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
  
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GRN, HIGH);
    delay(250);
    digitalWrite(LED_GRN, LOW);
}

void loop() 
{
#ifdef ANDROID
    BLECentral central = blePeripheral.central();

    if (central) 
    {
        // central connected to peripheral
        Serial.print(F("Connected to central: "));
        Serial.println(central.address());

        while (central.connected()) 
        {
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
#else // Serial control

    int serialCmd = 0;
    
    Serial.println(F("Enter a command:"));
    Serial.println(F("[1] to Read ADC value"));
    Serial.println(F("[2] to set Gain"));
    Serial.println(F("[3] to set SPS"));
    while (!(Serial.available() > 0));
    
    serialCmd = Serial.parseInt();
    if (serialCmd == 1) // Select gain
    {
        Serial.println(F("Select ADC channel [1-4]:"));
        
        while (!(Serial.available() > 0));
        
        serialCmd = Serial.parseInt();
        handleSerialCmd(Read, serialCmd);
        
    }
    else if (serialCmd == 2) // Select gain
    {
        Serial.println(F("Set gain:"));
        Serial.println(F("[1] G = 2/3"));
        Serial.println(F("[2] G = 1"));
        Serial.println(F("[3] G = 2"));
        Serial.println(F("[4] G = 4"));
        Serial.println(F("[5] G = 8"));
        Serial.println(F("[6] G = 16"));
        
        while (!(Serial.available() > 0));
        
        serialCmd = Serial.parseInt();
        handleSerialCmd(Gain, serialCmd);
        
    }
    else if (serialCmd == 3) // Select SPS
    {
        Serial.println(F("Set SPS:"));
        Serial.println(F("[1] SPS = 128"));
        Serial.println(F("[2] SPS = 250"));
        Serial.println(F("[3] SPS = 490"));
        Serial.println(F("[4] SPS = 920"));
        Serial.println(F("[5] SPS = 1600"));
        Serial.println(F("[6] SPS = 2400"));
        Serial.println(F("[7] SPS = 3300"));
        
        while (!(Serial.available() > 0));
        
        serialCmd = Serial.parseInt();
        handleSerialCmd(Sps, serialCmd);
        
    }
    else
    {
        Serial.println(F("Invalid choice"));
    }
    Serial.println(F("========================================"));
    
#endif
}
#ifdef ANDROID
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

void handleBleCmd(byte value)
{
    // Read command
    if (!(value & 0x3F) || value == 0xFF) // if xx00 0000 or 1111 1111)
    {
        int adcValue = 0;
        
        switch (value)
        {
            case 0x40:
                selectChannel(0);
                Serial.println(F("Channel: A0"));
                break;
            case 0x80:
                selectChannel(1);
                Serial.println(F("Channel: A1"));
                break;
            case 0xC0:
                selectChannel(2);
                Serial.println(F("Channel: A2"));
                break;    
            case 0xFF:
                selectChannel(3);
                Serial.println(F("Channel: A3"));
                break;
            default: 
                break;
        }
        //delay(20);
        //adcValue = analogRead(A1);
        //Serial.print(F("ADC0: "));
        //Serial.println(adcValue);
        //
        //delay(20);
        //adcValue = analogRead(A2);
        //Serial.print(F("ADC1: "));
        //Serial.println(adcValue);
        //
        //delay(20);
        //adcValue = analogRead(A3);
        //Serial.print(F("ADC2: "));
        //Serial.println(adcValue);
        //
        //delay(20);
        //adcValue = analogRead(A4);
        //Serial.print(F("ADC3: "));
        //Serial.println(adcValue);
        //
        //
        //delay(20);
        
        adcValue = ads.readADC_SingleEnded(0);
        //adcValue = analogRead(A0);
        
        Serial.print(F("ADC: "));
        Serial.println(adcValue);
        Serial.print("\n");

        adcValueCharacteristic.setValue(adcValue);
    }
    else
    {
        // Gain
        switch (value & 0x07) // 0000 0111
        {
            case 1: 
                ads.setGain(GAIN_TWOTHIRDS); 
                Serial.println(F("Gain set to 2/3")); 
                break;
            case 2: 
                ads.setGain(GAIN_ONE);       
                Serial.println(F("Gain set to 1"));   
                break;
            case 3: 
                ads.setGain(GAIN_TWO);       
                Serial.println(F("Gain set to 2"));   
                break;
            case 4: 
                ads.setGain(GAIN_FOUR);      
                Serial.println(F("Gain set to 4"));   
                break;
            case 5: 
                ads.setGain(GAIN_EIGHT);     
                Serial.println(F("Gain set to 8"));   
                break;
            case 6: 
                ads.setGain(GAIN_SIXTEEN);   
                Serial.println(F("Gain set to 16"));  
                break;
            default: 
                break;
        }
        // Samples per second
        switch ((value & 0x38) >> 3) // 0011 1000
        {
            case 1: 
                ads.setSPS(SPS_128);
                Serial.println(F("SPS set to 128"));
                break;
            case 2: 
                ads.setSPS(SPS_250);
                Serial.println(F("SPS set to 250"));
                break;
            case 3: 
                ads.setSPS(SPS_490);
                Serial.println(F("SPS set to 490"));
                break;
            case 4: 
                ads.setSPS(SPS_920);
                Serial.println(F("SPS set to 920"));
                break;
            case 5: 
                ads.setSPS(SPS_1600);
                Serial.println(F("SPS set to 1600"));
                break;
            case 6: 
                ads.setSPS(SPS_2400);
                Serial.println(F("SPS set to 2400"));
                break;
            case 7: 
                ads.setSPS(SPS_3300);
                Serial.println(F("SPS set to 3300"));
                break;
            default: 
                break;
        }
    }
}
#else // Serial command
void handleSerialCmd(byte options, byte serialCmd)
{
    if (options == Read)
    {
        int adcValue = 0;
        
        switch (serialCmd)
        {
            case 1:
                selectChannel(0);
                Serial.println(F("Channel: A0"));
                break;
            case 2:
                selectChannel(1);
                Serial.println(F("Channel: A1"));
                break;
            case 3:
                selectChannel(2);
                Serial.println(F("Channel: A2"));
                break;    
            case 4:
                selectChannel(3);
                Serial.println(F("Channel: A3"));
                break;
            default: 
                Serial.println(F("Invalid choice"));
                return;
        }
        
        adcValue = ads.readADC_SingleEnded(0);
        
        Serial.print(F("ADC: "));
        Serial.println(adcValue);
        Serial.print("\n");
    }
    else if (options == Gain)
    {
        // Set gain
        switch (serialCmd)
        {
            case 1: 
                ads.setGain(GAIN_TWOTHIRDS); 
                Serial.println(F("Gain set to 2/3")); 
                break;
            case 2: 
                ads.setGain(GAIN_ONE);       
                Serial.println(F("Gain set to 1"));   
                break;
            case 3: 
                ads.setGain(GAIN_TWO);       
                Serial.println(F("Gain set to 2"));   
                break;
            case 4: 
                ads.setGain(GAIN_FOUR);      
                Serial.println(F("Gain set to 4"));   
                break;
            case 5: 
                ads.setGain(GAIN_EIGHT);     
                Serial.println(F("Gain set to 8"));   
                break;
            case 6: 
                ads.setGain(GAIN_SIXTEEN);   
                Serial.println(F("Gain set to 16"));  
                break;
            default: 
                Serial.println(F("Invalid choice"));
                break;
        }
    }
    else if (options == Sps)
    {
        // Samples per second
        switch (serialCmd)
        {
            case 1: 
                ads.setSPS(SPS_128);
                Serial.println(F("SPS set to 128"));
                break;
            case 2: 
                ads.setSPS(SPS_250);
                Serial.println(F("SPS set to 250"));
                break;
            case 3: 
                ads.setSPS(SPS_490);
                Serial.println(F("SPS set to 490"));
                break;
            case 4: 
                ads.setSPS(SPS_920);
                Serial.println(F("SPS set to 920"));
                break;
            case 5: 
                ads.setSPS(SPS_1600);
                Serial.println(F("SPS set to 1600"));
                break;
            case 6: 
                ads.setSPS(SPS_2400);
                Serial.println(F("SPS set to 2400"));
                break;
            case 7: 
                ads.setSPS(SPS_3300);
                Serial.println(F("SPS set to 3300"));
                break;
            default: 
                Serial.println(F("Invalid choice"));
                break;
        }
    }
}
#endif

void selectChannel(byte channel)
{
    digitalWrite(ADDR_PIN_0, (channel & 0x01) >> 0);
    digitalWrite(ADDR_PIN_1, (channel & 0x02) >> 1);
    digitalWrite(ADDR_PIN_2, (channel & 0x04) >> 2);
}




