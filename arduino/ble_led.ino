// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//#define SHOW_FREE_MEMORY

#ifdef SHOW_FREE_MEMORY
#include <MemoryFree.h>
#endif

// Import libraries (BLEPeripheral depends on SPI)
#include <SPI.h>
#include <BLEPeripheral.h>

// define pins (varies per shield/board)
#define BLE_REQ   10
#define BLE_RDY   2
#define BLE_RST   9
#define LED       3

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

void setup() 
{
    Serial.begin(9600);
#if defined (__AVR_ATmega32U4__)
    delay(5000);  //5 seconds delay for enabling to see the start up comments on the serial board
#endif

    blePeripheral.setLocalName("test");
#if 1
    blePeripheral.setAdvertisedServiceUuid(testService.uuid());
#else
    const char manufacturerData[4] = {0x12, 0x34, 0x56, 0x78};
    blePeripheral.setManufacturerData(manufacturerData, sizeof(manufacturerData));
#endif

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

    // set initial value for characteristic
    testCharacteristic.setValue(0);

    // begin initialization
    blePeripheral.begin();

    Serial.println(F("BLE Peripheral"));

#ifdef SHOW_FREE_MEMORY
    Serial.print(F("Free memory = "));
    Serial.println(freeMemory());
#endif

//set timer1 interrupt at 1Hz
    TCCR1A = 0;// set entire TCCR1A register to 0
    TCCR1B = 0;// same for TCCR1B
    TCNT1  = 0;//initialize counter value to 0
    // set compare match register for 1hz increments
    OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
    // turn on CTC mode
    TCCR1B |= (1 << WGM12);
    // Set CS10, CS11 and CS12 bits for prescaler
    //TCCR1B |= (1 << CS10);                // clk / 1 = 16 MHz
    //TCCR1B |= (1 << CS11);                // clk / 8 = 2 MHz
    //TCCR1B |= (1 << CS11) | (1 << CS10);  // clk / 64 = 250 kHz
    TCCR1B |= (1 << CS12);                // clk / 256 = 62 500 Hz
    //TCCR1B |= (1 << CS12) | (1 << CS10);  // clk / 1024 = 15 625 Hz
    
    // enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);

    pinMode(LED, OUTPUT);
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
                testCharacteristic.setValue(testCharacteristic.value() + 1);

                Serial.print(F("counter = "));
                Serial.println(testCharacteristic.value(), DEC);
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
    counter = testCharacteristic.value();
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
ISR(TIMER1_COMPA_vect)
{
    if (counter > 0)
    {
        if (ledState)
        {
            digitalWrite(LED, HIGH);
            ledState = 0;
        }
        else
        {
            digitalWrite(LED, LOW);
            ledState = 1;
            counter--;
        }
    }
}