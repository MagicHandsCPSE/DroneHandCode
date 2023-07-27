// https://circuitdigest.com/microcontroller-projects/esp32-ble-server-how-to-use-gatt-services-for-battery-level-indication

#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include <ESP32Servo.h>
#include <ESP32Tone.h>
#include <ESP32PWM.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

/////////////////////////////
// MAKE SURE THESE ARE SAME AS DRONE HAND CODE COPY PASTE
#define DEVICE_NAME         "6a2017c6-d018-4930-8d1a-913289dfebf7"
#define SERVICE_UUID        "778f40a1-2937-4639-a10b-e0c112b04b0e"
#define SERVO1_CHAR_UUID    "086bf84b-736f-45e0-8e35-6adcd6cc0ec4"
#define SERVO2_CHAR_UUID    "f676b321-31a8-4179-8640-ce5699cf0721"
#define SERVO3_CHAR_UUID    "c0b627e5-c0ce-4b5f-b590-a096c3514db7"
// These are well-known UUIDs - don't change
#define BATTERY_SERVICE BLEUUID((uint16_t)0x180F)
#define BATTERY_CHARACT BLEUUID((uint16_t)0x2A19)
/////////////////////////////

BLECharacteristic* servo1Char;
BLECharacteristic* servo2Char;
BLECharacteristic* servo3Char;
BLECharacteristic* batteryChar;
Servo servo1;
Servo servo2;
Servo servo3;

SFE_MAX1704X battery(MAX1704X_MAX17048);

class showconnect : public BLEServerCallbacks {
    void onConnect(BLEServer* _){
        Serial.println("Connected");
    }
    void onDisconnect(BLEServer* _) {
        Serial.println("Disonnected, resuming advertising");
        BLEDevice::startAdvertising();
    }
};

class servowriter1 : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(uint8_t*)c->getData();
        servo1.write(a);
        Serial.printf("Wrote servo 1 to %i\n", a);
    }
};

class servowriter2 : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(uint8_t*)c->getData();
        servo2.write(a);
        Serial.printf("Wrote servo 2 to %i\n", a);
    }
};

class servowriter3 : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(uint8_t*)c->getData();
        servo3.write(a);
        Serial.printf("Wrote servo 3 to %i\n", a);
    }
};

void setupBLE() {
    BLEDevice::init(DEVICE_NAME);
    BLEServer* server = BLEDevice::createServer();
    server->setCallbacks(new showconnect());
    BLEService* service = server->createService(SERVICE_UUID);
    servo1Char = service->createCharacteristic(
                     SERVO1_CHAR_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE
                 );
    servo1Char->setCallbacks(new servowriter1());
    servo2Char = service->createCharacteristic(
                     SERVO2_CHAR_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE
                 );
    servo2Char->setCallbacks(new servowriter2());
    servo3Char = service->createCharacteristic(
                     SERVO3_CHAR_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE
                 );

    servo3Char->setCallbacks(new servowriter3());
    service->start();
    BLEService* bservice = server->createService(BATTERY_SERVICE);
    batteryChar = bservice->createCharacteristic(
                      BATTERY_CHARACT,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                  );
    batteryChar->addDescriptor(new BLE2902());
    bservice->start();
    BLEAdvertising* ad = BLEDevice::getAdvertising();
    ad->addServiceUUID(SERVICE_UUID);
    ad->addServiceUUID(BATTERY_SERVICE);
    ad->setScanResponse(true);
    ad->setMinPreferred(0x06);  
    ad->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();
}

bool hasBattery;
void setup() {
    Serial.begin(115200);
    servo1.attach(25);
    servo2.attach(26);
    servo3.attach(27);
    hasBattery = battery.begin();
    setupBLE();
}


void loop() {
    yield();
    if (!hasBattery) return;
    static uint8_t oldbvalue = 0;
    uint8_t battvalue = (uint8_t)battery.getSOC();
    if (battvalue != oldbvalue) {
        oldbvalue = battvalue;
        batteryChar->setValue(&battvalue, 1);
        batteryChar->notify();
    }
}
