#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif
#include <ESPAsyncWebServer.h>

#define DEBUG(x) Serial.println(x)

AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);

    DEBUG("Configuring Access Point");
    DEBUG(WiFi.softAPConfig(
        IPAddress(4, 3, 2, 1),
        IPAddress(4, 3, 2, 1),
        IPAddress(255, 255, 255, 0)
    ) ? "Success" : "Failure");

    DEBUG("Starting Access Point");
    DEBUG(WiFi.softAP("Disco Node") ? "Success" : "Failure");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request){
        request->send(200, "text/plain", "Hello, World!");
    });

    server.begin();
}

void loop() {
    
}