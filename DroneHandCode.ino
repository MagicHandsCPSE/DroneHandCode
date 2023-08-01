#include "pins.h"
#include <Wire.h>
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
#define DEVICE_NAME         "MagicHands"
#define SERVICE_UUID        "778f40a1-2937-4639-a10b-e0c112b04b0e"
#define SERVO1_CHAR_UUID    "086bf84b-736f-45e0-8e35-6adcd6cc0ec4"
#define SERVO2_CHAR_UUID    "f676b321-31a8-4179-8640-ce5699cf0721"
#define SERVO3_CHAR_UUID    "c0b627e5-c0ce-4b5f-b590-a096c3514db7"
#define DRONE_SERVICE_UUID  "6a2017c6-d018-4930-8d1a-913289dfebf7"
#define DRONE_X_CHAR_UUID   "23312560-c77d-4022-90f0-341837850a5c"
#define DRONE_Y_CHAR_UUID   "94fb83bb-6868-4945-b477-b510e27eeb31"
#define DRONE_A_CHAR_UUID   "839f6d24-f852-405e-b4ee-5cf2e538eca2"
// These are well-known UUIDs - don't change
#define BATTERY_SERVICE BLEUUID((uint16_t)0x180F)
#define BATTERY_CHARACT BLEUUID((uint16_t)0x2A19)
/////////////////////////////

BLECharacteristic* batteryChar;
Servo servo1;
Servo servo2;
Servo servo3;

SFE_MAX1704X battery(MAX1704X_MAX17048);

class showconnect: public BLEServerCallbacks {
    void onConnect(BLEServer* _){
        Serial.println("Connected");
    }
    void onDisconnect(BLEServer* _) {
        Serial.println("Disonnected, resuming advertising");
        BLEDevice::startAdvertising();
    }
};

class servowriter1: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(uint8_t*)c->getData();
        servo1.write(a);
        Serial.printf("Wrote servo 1 to %i\n", a);
    }
};

class servowriter2: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(uint8_t*)c->getData();
        servo2.write(a);
        Serial.printf("Wrote servo 2 to %i\n", a);
    }
};

class servowriter3: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(uint8_t*)c->getData();
        servo3.write(a);
        Serial.printf("Wrote servo 3 to %i\n", a);
    }
};

class xcontroller: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(int8_t*)c->getData();
        Serial1.printf("x=%i\n", a);
        Serial.printf("X thrust is %i\n", a);
    }
};

class ycontroller: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(int8_t*)c->getData();
        Serial1.printf("y=%i\n", a);
        Serial.printf("Y thrust is %i\n", a);
    }
};

class acontroller: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(int8_t*)c->getData();
        Serial1.printf("a=%i\n", a);
        Serial.printf("Altitude is %i\n", a);
    }
};

void setupBLE() {
    BLEDevice::init(DEVICE_NAME);
    BLEServer* server = BLEDevice::createServer();
    server->setCallbacks(new showconnect());
    BLEService* service = server->createService(SERVICE_UUID);
    BLECharacteristic* servo1Char = service->createCharacteristic(
                                        SERVO1_CHAR_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_WRITE
                                    );
    servo1Char->setCallbacks(new servowriter1());
    BLECharacteristic* servo2Char = service->createCharacteristic(
                                        SERVO2_CHAR_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_WRITE
                                    );
    servo2Char->setCallbacks(new servowriter2());
    BLECharacteristic* servo3Char = service->createCharacteristic(
                                        SERVO3_CHAR_UUID,
                                        BLECharacteristic::PROPERTY_READ |
                                        BLECharacteristic::PROPERTY_WRITE
                                    );

    servo3Char->setCallbacks(new servowriter3());
    service->start();
    BLEService* dservice = server->createService(DRONE_SERVICE_UUID);
    BLECharacteristic* droneX = service->createCharacteristic(
                                    DRONE_X_CHAR_UUID,
                                    BLECharacteristic::PROPERTY_READ |
                                    BLECharacteristic::PROPERTY_WRITE
                                );
    droneX->setCallbacks(new xcontroller());
    BLECharacteristic* droneY = service->createCharacteristic(
                                    DRONE_Y_CHAR_UUID,
                                    BLECharacteristic::PROPERTY_READ |
                                    BLECharacteristic::PROPERTY_WRITE
                                );
    droneY->setCallbacks(new ycontroller());
    BLECharacteristic* droneA = service->createCharacteristic(
                                    DRONE_A_CHAR_UUID,
                                    BLECharacteristic::PROPERTY_READ |
                                    BLECharacteristic::PROPERTY_WRITE
                                );
    droneA->setCallbacks(new acontroller());
    dservice->start();
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
    ad->addServiceUUID(DRONE_SERVICE_UUID);
    ad->setScanResponse(true);
    ad->setMinPreferred(0x06);  
    ad->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();
}

bool hasBattery;
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    servo1.attach(SERVO1);
    servo2.attach(SERVO2);
    servo3.attach(SERVO3);
    Wire.begin();
    hasBattery = battery.begin();
    if (!hasBattery) Serial.println("No battery...");
    setupBLE();
}

int timeout = 2000;
void loop() {
    yield();
    delay(1);
    if (!hasBattery) return;
    static uint8_t oldbvalue = 0;
    uint8_t battvalue = (uint8_t)(int)battery.getSOC();
    timeout--;
    if (battvalue != oldbvalue) {
        timeout = 2000;
        oldbvalue = battvalue;
        Serial.printf("Battery level: %hhu%\n", battvalue);
        batteryChar->setValue(&battvalue, 1);
        batteryChar->notify();
    }
    if (timeout <= 0) {
        batteryChar->notify();
        timeout = 2000;
    }
}
