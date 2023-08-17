#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#include <secrets.h>

#define DEBUG_LN(x) Serial.println(x)

AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);
    delay(10);

    // DEBUG_LN("Configuring Access Point");
    // DEBUG_LN(WiFi.softAPConfig(
    //     IPAddress(4, 3, 2, 1),
    //     IPAddress(4, 3, 2, 1),
    //     IPAddress(255, 255, 255, 0)
    // ) ? "Success" : "Failure");

    // DEBUG_LN("Starting Access Point");
    // DEBUG_LN(WiFi.softAP("Disco Node") ? "Success" : "Failure");

    DEBUG_LN("Connecting to WiFi");
    // while (wifi_status != WL_CONNECTED)
    WiFi.hostname("chroma");
    //DEBUG_LN(WiFi.begin(WIFI_SSID, WIFI_PASS) == WL_CONNECTED ? "Success" : "Failure");
    WiFi.begin(WIFI_SSID, WIFI_PASS);           // Connect to the network
    Serial.print("Connecting to ");
    Serial.print(WIFI_SSID); Serial.println(" ...");

    int i = 0;
    while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
        delay(1000);
        Serial.print(++i); Serial.print(' ');
    }

    Serial.println('\n');
    Serial.println("Connection established!");  
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request){
        request->send(200, "text/plain", "Hello, World!");
    });
    server.begin();

    // MDNS.end();
    DEBUG_LN("Starting mDNS");
    DEBUG_LN(MDNS.begin("chroma", WiFi.localIP()) ? "Success" : "Failure");
    // MDNS.addService("http", "tcp", 80);
    // MDNS.addService("chroma", "tcp", 80);
}

void loop() {
#ifdef ESP8266
    MDNS.update();
#endif
}