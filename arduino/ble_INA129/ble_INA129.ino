/*
 *  Julius Lesonen
 *  Jere Heino
 *
 *  4.12.2018
 *  
 *      Measure current using INA129 and send it to android via bluetooth (nRF8001).
 *  
 */

// Import libraries (BLEPeripheral depends on SPI)
#include <SPI.h>
#include <BLEPeripheral.h>
#include <Wire.h>
#include <Adafruit_INA219.h>

// define pins (varies per shield/board)
#define BLE_RDY     2
#define BLE_RST     9
#define BLE_REQ     10

#define LED_GRN     3
#define LED_RED     7

// Comment next line to enable serial version
#define ANDROID

Adafruit_INA219 ina219;

float current_mA = 0;

#ifdef ANDROID
// create peripheral instance, see pinouts above
BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
// create service
BLEService service = BLEService("fff0");
// create write characteristic
BLEUnsignedShortCharacteristic writeCharacteristic 
= BLEUnsignedShortCharacteristic("fff1", BLERead | BLEWrite);

BLEUnsignedShortCharacteristic INA129ValueCharacteristic 
= BLEUnsignedShortCharacteristic("fff2", BLERead);

// create user description descriptor for characteristic
BLEDescriptor descriptor = BLEDescriptor("2901", "INA129");

// Command received from Android
byte bleCmdValue = 0;
#endif // ANDROID

void setup() 
{
    // Setup leds
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GRN, OUTPUT);
    
    // Turn on red so we see if setup hangs
    digitalWrite(LED_RED, HIGH);
    
    Serial.begin(9600);
    
#ifdef ANDROID
    blePeripheral.setLocalName("BLE INA129");
    blePeripheral.setAdvertisedServiceUuid(service.uuid());

    // set device name and appearance
    blePeripheral.setDeviceName("BLE INA129");
    blePeripheral.setAppearance(0x0080);

    // add service, characteristic, and decriptor to peripheral
    blePeripheral.addAttribute(service);
    blePeripheral.addAttribute(writeCharacteristic);
    blePeripheral.addAttribute(INA129ValueCharacteristic);
    blePeripheral.addAttribute(descriptor);

    // assign event handlers for connected, disconnected to peripheral
    blePeripheral.setEventHandler(BLEConnected, blePeripheralConnectHandler);
    blePeripheral.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

    // assign event handlers for characteristic
    writeCharacteristic.setEventHandler(BLEWritten, characteristicWritten);
    writeCharacteristic.setEventHandler(BLESubscribed, characteristicSubscribed);
    writeCharacteristic.setEventHandler(BLEUnsubscribed, characteristicUnsubscribed);
    
    // set initial value for characteristic
    writeCharacteristic.setValue(0);
    INA129ValueCharacteristic.setValue(0);

    // begin initialization
    blePeripheral.begin();
    
    Serial.print(F("BLE Peripheral"));
#endif // ANDROID
  
    // Initialize the INA219.
    // By default the initialization will use the largest range (32V, 2A).  However
    // you can call a setCalibration function to change this range (see comments).
    ina219.begin();
    // To use a slightly lower 32V, 1A range (higher precision on amps):
    //ina219.setCalibration_32V_1A();
    // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
    //ina219.setCalibration_16V_400mA();
  
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
                handleBleCmd();
                bleCmdValue = 0;
            }
            
        }
        // central disconnected
        Serial.print(F("Disconnected from central: "));
        Serial.println(central.address());
    }
#else // Serial mode

    // Measure current
    Serial.print(F("Current: "));
    Serial.print(ina219.getCurrent_mA());
    Serial.println(F(" mA"));
    
    delay(500);
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
    Serial.println(writeCharacteristic.value(), DEC);
    bleCmdValue = writeCharacteristic.value();
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

void handleBleCmd()
{
    // Measure current
    current_mA = ina219.getCurrent_mA();
    
    Serial.print(F("Current: "));
    Serial.print(current_mA);
    Serial.println(F(" mA"));
    
    // Send value to android
    INA129ValueCharacteristic.setValue((unsigned short)(current_mA*1000.f));
}
#endif // ANDROID


