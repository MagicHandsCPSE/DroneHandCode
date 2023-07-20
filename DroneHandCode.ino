// https://learn.sparkfun.com/tutorials/sending-sensor-data-via-bluetooth
#include <ESP32Servo.h>
#include <ESP32Tone.h>
#include <ESP32PWM.h>
#include <BLEDevice.h>
#include "task.h"

/////////////////////////////
// COPY THESE TO GLOVE CODE AFTER CHANGING
#define DEVICE_NAME         "24f7ad15-fdde-4c83-8327-400a87de818c"
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define THUMB_CHAR_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a8"
/////////////////////////////

static BLEUUID serviceUUID(SERVICE_UUID);
static BLEUUID charUUID(THUMB_CHAR_UUID);
static BLERemoteCharacteristic* thumbCharacteristic;
static BLEAdvertisedDevice* glove;

Servo thumb;

static void thumbNotify(BLERemoteCharacteristic* characteristic, uint8_t* data, size_t len, bool notify) {
    Serial.printf("thumbNotify(): %s sent %zu bytes\n", characteristic->getUUID().toString().c_str(), len);
    int pos = *(int*)data;
    Serial.printf("Servo pos is %i\n", pos);
    thumb.write(pos);
    yield();
}

bool oktoconnect = false;
bool connected = false;
class freezeOnDisconnect : public BLEClientCallbacks {
    void onConnect(BLEClient* _) {
        Serial.println("Connected callback!!");
    }
    void onDisconnect(BLEClient* _) {
        //freeze();
        Serial.println("Disconnected!!");
        connected = false;
    }
};

bool connect() {
    Serial.println("connecting...");
    BLEClient* client = BLEDevice::createClient();
    client->setClientCallbacks(new freezeOnDisconnect());
    client->connect(glove);
    client->setMTU(517); // maximum
    BLERemoteService* service = client->getService(serviceUUID);
    if (service == NULL) {
        Serial.println("error getting service");
        client->disconnect();
        return false;
    }
    thumbCharacteristic = service->getCharacteristic(charUUID);
    if (thumbCharacteristic == NULL) {
        Serial.println("error getting thumbCharacteristic");
        client->disconnect();
        return false;
    }
    if (!thumbCharacteristic->canNotify()) {
        Serial.println("whoops it isn't a notify");
        client->disconnect();
        return false;
    }
    thumbCharacteristic->registerForNotify(thumbNotify);
    Serial.println("Connetcting complete!");
    return true;
}

class chooseWithService : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice device) {
        Serial.printf("got advertise: %s\n", device.toString().c_str());
        if (device.haveServiceUUID() && device.isAdvertisingService(serviceUUID)) {
            BLEDevice::getScan()->stop();
            glove = new BLEAdvertisedDevice(device);
            oktoconnect = true;
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("scanning for glove...");
    BLEDevice::init("");
    BLEScan* scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(new chooseWithService());
    scan->setInterval(1349);
    scan->setWindow(449);
    scan->setActiveScan(true);
    scan->start(5, false);
    thumb.attach(25);
}

void loop() {
    if (oktoconnect && !connected) {
        connected = connect();
        if (connected) {
            Serial.println("connected OK!!");
        }
    }
    if (!connected) {
        BLEDevice::getScan()->start(0);
    }
    yield();
}
