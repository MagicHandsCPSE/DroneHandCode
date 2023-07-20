#include <ESP32Servo.h>
#include <ESP32Tone.h>
#include <ESP32PWM.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "task.h"

/////////////////////////////
// MAKE SURE THESE ARE SAME AS DRONE HAND CODE COPY PASTE
#define DEVICE_NAME         "24f7ad15-fdde-4c83-8327-400a87de818c"
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SERVO1_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SERVO2_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define SERVO3_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
/////////////////////////////

BLECharacteristic* servo1Char;
BLECharacteristic* servo2Char;
BLECharacteristic* servo3Char;
Servo servo1;
Servo servo2;
Servo servo3;

class showconnect : public BLEServerCallbacks {
    void onConnect(BLEServer* _){
        Serial.println("Connected");
    }
    void onDisconnect(BLEServer* _) {
        Serial.println("Disonnected");
    }
};

class servowriter1 : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(int*)c->getData();
        servo1.write(a);
        Serial.printf("Wrote servo 1 to %i\n", a);
    }
};

class servowriter2 : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(int*)c->getData();
        servo2.write(a);
        Serial.printf("Wrote servo 2 to %i\n", a);
    }
};

class servowriter3 : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* c) {
        int a = *(int*)c->getData();
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
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_NOTIFY
                 );
    servo1Char->setCallbacks(new servowriter1());
    servo2Char = service->createCharacteristic(
                     SERVO2_CHAR_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE |
                     BLECharacteristic::PROPERTY_NOTIFY
                 );
    servo2Char->setCallbacks(new servowriter2());
    servo3Char = service->createCharacteristic(
                     SERVO3_CHAR_UUID,
                     BLECharacteristic::PROPERTY_READ |
                     BLECharacteristic::PROPERTY_WRITE |
                    BLECharacteristic::PROPERTY_NOTIFY
                 );

    servo3Char->setCallbacks(new servowriter3());
    service->start();
    BLEAdvertising* ad = BLEDevice::getAdvertising();
    ad->addServiceUUID(SERVICE_UUID);
    ad->setScanResponse(true);
    ad->setMinPreferred(0x06);  
    ad->setMaxPreferred(0x12);
    BLEDevice::startAdvertising();
}

void setup() {
    Serial.begin(115200);
    servo1.attach(25);
    servo2.attach(26);
    servo3.attach(27);
    setupBLE();
}


void loop() {
    yield();
    // empty loop - values are handled by write handlers
}
