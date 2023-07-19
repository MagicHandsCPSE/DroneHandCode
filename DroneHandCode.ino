#include <WiFi.h>
#include <DNSServer.h>
#include "task.h"

const char* name = "24f7ad15-fdde-4c83-8327-400a87de818c";
const char* pass = "foo";//"9e267cbc-7e40-4d95-be9a-c8f75ba197fa";

const char* hostname = "MagicHands";

WiFiServer server(80);
IPAddress myIP(8,8,4,4);
IPAddress netMask(255,255,255,0);
DNSServer dnsServer;


String getLine(WiFiClient* client) {
    String line = "";
    char ch;
    do {
        ch = client->read();
        if (ch != '\r' && ch != '\n') line += ch;
    } while (ch != '\n');
    return line;
}

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
    String firstLine = "";
    while (client.connected()) {
        String line = getLine(&client);
        if (firstLine.length() == 0) firstLine = line;
        Serial.println(line);
        if (!line.length()) {
            Serial.println("*** END OF HEADERS, got payload ***");
            Serial.println(getAll(&client));

            int status = 200;
            String name = "OK";
            String content = "<h1>Hello</h1><pre>" + firstLine + "</pre>";
            String headers = "refresh: 1; url=/";
            content = content + "<p>Pin 19 is " + (digitalRead(19) ? "HIGH" : "LOW") + "</p>";

            if (firstLine.indexOf("/favicon.ico") != -1) {
                status = 404;
                name = "NOT FOUND";
                content = "NOT FOUND";
                headers = "";
            }

            char* b;
            
            asprintf(&b, "HTTP/1.1 %i %s\r\ncontent-type: text/html\r\nconnnection: close%s%s\r\n\r\n%s\r\n\r\n",
                status, name.c_str(),
                headers.length() == 0 ? "" : "\r\n",
                headers.c_str(),
                content.c_str());
            Serial.println("Sending response...");
            client.print(b);
            Serial.print(b);
            client.stop();
            free(b);
        }
    }
});

Task dnsTask("dns", 0, NULL, [](void* arg) -> void {
      dnsServer.processNextRequest();
});



void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(name, pass);
    Serial.println("Wait for server setup...");
    delay(100);
    WiFi.softAPConfig(myIP, myIP, netMask);
    Serial.println("Wifi setup OK");
    WiFi.softAPsetHostname(hostname);
    Serial.printf("Set HOSTNAME to %s\n", hostname);
    dnsServer.start(53, "*", myIP);
    Serial.println("DNS server started");
    server.begin();
    Serial.println("Web server started");
}

void loop() {
    serverTask.run();
    dnsTask.run();
}
