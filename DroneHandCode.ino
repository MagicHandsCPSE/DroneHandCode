#include "pins.h"
#include <Wire.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include <ESP32Servo.h>
#include <ESP32Tone.h>
#include <ESP32PWM.h>
#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

/////////////////////////////
// MAKE SURE THESE ARE SAME AS DRONE HAND CODE COPY PASTE
#define DEVICE_NAME         "MagicHands"
#define SERVICE_UUID        "778f40a1-2937-4639-a10b-e0c112b04b0e"
#define SERVO_CHAR_UUID     "086bf84b-736f-45e0-8e35-6adcd6cc0ec4"
#define DRONE_SERVICE_UUID  "6a2017c6-d018-4930-8d1a-913289dfebf7"
#define DRONE_CHAR_UUID     "23312560-c77d-4022-90f0-341837850a5c"
// These are well-known UUIDs - don't change
#define BATTERY_SERVICE BLEUUID((uint16_t)0x180F)
#define BATTERY_CHARACT BLEUUID((uint16_t)0x2A19)
/////////////////////////////

BLECharacteristic* batteryChar;
Servo servo1;
Servo servo2;
Servo servo3;

SFE_MAX1704X battery(MAX1704X_MAX17048);
HardwareSerial DroneSerial(1);

bool connected = false;

class showconnect: public BLEServerCallbacks {
    void onConnect(BLEServer* _){
        Serial.println("Connected");
        connected = true;
    }
    void onDisconnect(BLEServer* _) {
        Serial.println("Disonnected, resuming advertising");
        BLEDevice::startAdvertising();
        connected = false;
    }
};

class servowriter: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* ch) {
        int d = *(int*)ch->getData();
        int a = (d & 0xFF0000UL) >> 16;
        int b = (d & 0xFF00UL) >> 8;
        int c = (d & 0xFFUL);
        servo1.write(a);
        servo2.write(b);
        servo3.write(c);
        Serial.printf("Wrote servo 1 to %i, 2 to %i, 3 to %i (packed=%#x)\n", a, b, c, d);
    }
};

class xcontroller: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* ch) {
        int d = *(int*)ch->getData();
        int a = (d & 0xFF0000UL) >> 16;
        int b = (d & 0xFF00UL) >> 8;
        int c = (d & 0xFFUL);
        if (a & 0x80) a -= 256;
        if (b & 0x80) b -= 256;
        if (c & 0x80) c -= 256;
        DroneSerial.printf("x=%i\ny=%i\na=%i\n", a, b, c);
        Serial.printf("X thrust is %i, Y is %i, A is %i (packed=%#x)\n", a, b, c, d);
    }
};

void setupBLE() {
    BLEDevice::init(DEVICE_NAME);
    BLEServer* server = BLEDevice::createServer();
    server->setCallbacks(new showconnect());
    BLEService* service = server->createService(SERVICE_UUID);
    BLECharacteristic* servoChar = service->createCharacteristic(
                                       SERVO_CHAR_UUID,
                                       BLECharacteristic::PROPERTY_READ |
                                       BLECharacteristic::PROPERTY_WRITE
                                   );
    servoChar->setCallbacks(new servowriter());
    service->start();
    BLEService* dservice = server->createService(DRONE_SERVICE_UUID);
    BLECharacteristic* droneC = dservice->createCharacteristic(
                                    DRONE_CHAR_UUID,
                                    BLECharacteristic::PROPERTY_READ |
                                    BLECharacteristic::PROPERTY_WRITE
                                );
    droneC->setCallbacks(new xcontroller());
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
    DroneSerial.begin(115200, SERIAL_8N1, RXPIN, TXPIN);
    servo1.attach(SERVO1);
    servo2.attach(SERVO2);
    servo3.attach(SERVO3);
    Wire.begin();
    hasBattery = battery.begin();
    if (!hasBattery) Serial.println("No battery...");
    Serial.println("Before setupBLE()");
    setupBLE();
}

int timeout = 2000;
void loop() {
    yield();
    delay(1);
    neopixelWrite(NEOPIXEL_BUILTIN, 0, connected ? 255 : 0, connected ? 0 : (millis() / 500) & 1 ? 255 : 0);
    if (!hasBattery) return;
    static uint8_t oldbvalue = 0;
    uint8_t battvalue = (uint8_t)(int)battery.getSOC();
    timeout--;
    if (battvalue != oldbvalue) {
        timeout = 2000;
        oldbvalue = battvalue;
        batteryChar->setValue(&battvalue, 1);
        batteryChar->notify();
    }
    if (timeout <= 0) {
        Serial.printf("Battery level: %hhu%\n", battvalue);
        batteryChar->notify();
        timeout = 2000;
        // Also print blank lines to make it not compain about nothing in buffer on RPi Serial port
        DroneSerial.println();
    }
}
