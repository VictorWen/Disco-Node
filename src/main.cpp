#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

#include <secrets.h>

#define DEBUG_LN(x) Serial.println(x)

// AsyncWebServer server(80);
AsyncServer server = AsyncServer(12346);

static void handleNewClient(void* arg, AsyncClient* client);

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

    // server.on("/", HTTP_GET, [](AsyncWebServerRequest* request){
    //     request->send(200, "text/plain", "Hello, World!");
    // });
    // server.begin();

    // MDNS.end();
    DEBUG_LN("Starting mDNS");
    DEBUG_LN(MDNS.begin("chroma", WiFi.localIP()) ? "Success" : "Failure");
    
    // MDNS.addService("http", "tcp", 80);
    MDNS.addService("disco", "udp", 12345);
    MDNS.addService("discoConnect", "tcp", 12346);

    // start listening on tcp port 7050
	server.onClient(&handleNewClient, NULL);
	server.begin();
}

void loop() {
#ifdef ESP8266
    MDNS.update();
#endif
}

static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
	Serial.printf("\n data received from client %s \n", client->remoteIP().toString().c_str());
	Serial.write((uint8_t*)data, len);

    if (strcmp((char*)data, "DISCO CONNECT\n") == 0 && client->space() > 32 && client->canSend()) {
        char reply[] = "DISCO READY\n";
		client->add(reply, strlen(reply));
		client->send();
    }
}

static void handleNewClient(void* arg, AsyncClient* client) {
	Serial.printf("\n new client has been connected to server, ip: %s", client->remoteIP().toString().c_str());
	
	// register events
	client->onData(&handleData, NULL);;
}