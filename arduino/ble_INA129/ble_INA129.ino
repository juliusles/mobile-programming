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
#include <Adafruit_INA219.h>

// define pins (varies per shield/board)
#define BLE_RDY     2
#define BLE_RST     9
#define BLE_REQ     10

#define LED_GRN     3
#define LED_RED     7

/**************************************************
    Comment next line to use serial monitor only
***************************************************/
#define ANDROID



Adafruit_INA219 ina219;

#ifdef ANDROID
// create peripheral instance, see pinouts above
BLEPeripheral blePeripheral = BLEPeripheral(BLE_REQ, BLE_RDY, BLE_RST);
// create service
BLEService service = BLEService("fff0");
// create write characteristic
BLEUnsignedShortCharacteristic writeCharacteristic 
= BLEUnsignedShortCharacteristic("fff1", BLERead | BLEWrite);

BLEFloatCharacteristic INA129ValueCharacteristic 
= BLEFloatCharacteristic("fff2", BLERead);

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

    // assign event handlers for characteristic
    writeCharacteristic.setEventHandler(BLEWritten, characteristicWritten);
    
    // set initial value for characteristic
    writeCharacteristic.setValue(0);
    INA129ValueCharacteristic.setValue(0);

    // begin initialization
    blePeripheral.begin();
    
#else // Initialize serial only when not using BLE
    Serial.begin(9600);
#endif
  
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
        while (central.connected()) 
        {
            // Handle commands sent from Android
            if (bleCmdValue > 0)
            {
                handleBleCmd(bleCmdValue);
                bleCmdValue = 0;
            }        
        }
    }
#else // Serial mode

    // Measure current
    Serial.print(F("Current: "));
    Serial.print(ina219.getCurrent_mA());
    Serial.println(F(" mA"));
    // Measure power
    Serial.print(F("Power:"));
    Serial.print(ina219.getPower_mW());
    Serial.println(F(" mW"));
    
    delay(500);
#endif
}
#ifdef ANDROID
void characteristicWritten(BLECentral& central, BLECharacteristic& characteristic)
{
    // characteristic value written event handler
    bleCmdValue = writeCharacteristic.value();
}

void handleBleCmd(byte cmd)
{
    // Measure current
    if (cmd == 1)
    {
        // Send value to android
        INA129ValueCharacteristic.setValue(ina219.getCurrent_mA());
    }
    
    // Measure power
    else if (cmd == 2)
    {
        // Send value to android
        INA129ValueCharacteristic.setValue(ina219.getPower_mW());
    }
}
#endif // ANDROID


