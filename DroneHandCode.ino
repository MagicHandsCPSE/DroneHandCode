#include <ESP32Servo.h>
#include <ESP32Tone.h>
#include <ESP32PWM.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "task.h"

/////////////////////////////
// COPY THESE TO GLOVE CODE AFTER CHANGING
const char* name = "24f7ad15-fdde-4c83-8327-400a87de818c";
const char* pass = "9e267cbc-7e40-4d95-be9a-c8f75ba197fa";
/////////////////////////////

WiFiServer server(80);
DNSServer dnsServer;
IPAddress local_IP(192, 168, 1, 156);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);


Servo thumb;

String getAll(WiFiClient* client) {
    String all = "";
    char ch;
    while (client->available()) all += client->read();
    return all;
}

Task serverTask("server", 0, NULL, [](void* arg) -> void {
    WiFiClient client = server.available();
    if (!client) return;
    Serial.print("Got an HTTP client at ");
    Serial.println(client.remoteIP());
    String all = getAll(&client);
    if (all.indexOf("HTTP") != -1) {
        client.println("HTTP/1.1 500 PLEASE DON'T\r\n");
        client.stop();
        return;
    }
    int thumbPos = -1;
    int read = sscanf(all.c_str(), "thumb=%i", &thumbPos);
    if (read == 1) {
        client.printf("thumb at %i\n", thumbPos);
        thumb.write(thumbPos);        
    } else {
        client.println("sscanf error");
    }
    client.stop();
});

Task dnsTask("dns", 0, NULL, [](void* arg) -> void {
      dnsServer.processNextRequest();
});

void setup() {
    Serial.begin(115200);
    WiFi.softAP(name, pass);
    Serial.println("Wait for server setup...");
    delay(100);
    Serial.println("Wifi setup OK");
    server.begin();
    Serial.println("Web server started");
    dnsServer.start(53, "*", primaryDNS);
    Serial.println("DNS capture started");

    thumb.attach(25);
}

void loop() {
    serverTask.run();
    dnsTask.run();
}
