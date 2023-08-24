#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#endif
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiUdp.h>

#include <secrets.h>

#define DEBUG_LN(x) Serial.println(x)

#define DISCO_PORT 12345
#define CONFIG_PORT 12346
#define MAX_DISCO_PACKET_SIZE 4096
#define MAX_COLOR 255

// AsyncWebServer server(80);
AsyncServer server = AsyncServer(CONFIG_PORT);
Adafruit_NeoPixel pixels(150, 2, NEO_GRB + NEO_KHZ800);
WiFiUDP udp;
byte udp_packet_buffer[MAX_DISCO_PACKET_SIZE];


static void handleNewClient(void* arg, AsyncClient* client);
uint32_t convert_bytes_to_int(byte* array, int offset);
float convert_bytes_to_float(byte* array, int offset);

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
    WiFi.hostname("chroma");
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
    auto service = MDNS.addService(0 ,"disco", "udp", DISCO_PORT);
    MDNS.addServiceTxt(service, "device", "esp8266");
    MDNS.addServiceTxt(service, "version", 1);
    MDNS.addServiceTxt(service, "preferred-ipv", 4);

	// server.onClient(&handleNewClient, NULL);
	// server.begin();

    pixels.begin();
    pixels.clear();
    pixels.show();

    udp.begin(DISCO_PORT);
}

void loop() {
#ifdef ESP8266
    MDNS.update();
#endif
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
        // Serial.printf("Received %d bytes from %s, port %d\n", packetSize, udp.remoteIP().toString().c_str(), udp.remotePort());
        int len = udp.read(udp_packet_buffer, MAX_DISCO_PACKET_SIZE);
        if (len < 12) {
            Serial.printf("Got invalid packet of length: %d, wanting 12\n", len);
            return;
        }
        uint32_t start = convert_bytes_to_int(udp_packet_buffer, 0);
        uint32_t end = convert_bytes_to_int(udp_packet_buffer, 4);
        uint32_t n_frames = convert_bytes_to_int(udp_packet_buffer, 8);
        // Serial.printf("Got start: %d, end: %d, n_frames: %d\n", start, end, n_frames);
        int pixels_per_frame = end - start;
        int total_pixels = pixels_per_frame * n_frames;
        if (len < 12 + total_pixels * 4) {
            Serial.printf("Got invalid packet of length: %d, wanting: %d\n", len, 12 + total_pixels * 4);
            return;
        }
        // TODO: deal with multiple frames
        for (int i = 0; i < pixels_per_frame; i++) {
            // float a = convert_bytes_to_float(udp_packet_buffer, 24 + i * 16);
            // uint8_t r = convert_bytes_to_float(udp_packet_buffer, 12 + i * 16) * a * MAX_COLOR;
            // uint8_t g = convert_bytes_to_float(udp_packet_buffer, 16 + i * 16) * a * MAX_COLOR;
            // uint8_t b = convert_bytes_to_float(udp_packet_buffer, 20 + i * 16) * a * MAX_COLOR;

            uint8_t r = udp_packet_buffer[12 + i * 4];
            uint8_t g = udp_packet_buffer[13 + i * 4];
            uint8_t b = udp_packet_buffer[14 + i * 4];
            uint8_t a = udp_packet_buffer[15 + i * 4];
            
            // Serial.printf("Pixel color at %d is %d, %d, %d, %f\n", start + i, r, g, b, a);
            pixels.setPixelColor(start + i, r, g, b);
        }
        pixels.show();
    }
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

uint32_t convert_bytes_to_int(byte* array, int offset) {
    union byte_int {
        byte array[4];
        uint32_t value;
    };
    byte_int data;
    data.array[0] = array[offset + 0];
    data.array[1] = array[offset + 1];
    data.array[2] = array[offset + 2];
    data.array[3] = array[offset + 3];
    return data.value;
}

float convert_bytes_to_float(byte* array, int offset) {
    union byte_float {
        byte array[4];
        float value;
    };
    byte_float data;
    data.array[0] = array[offset + 0];
    data.array[1] = array[offset + 1];
    data.array[2] = array[offset + 2];
    data.array[3] = array[offset + 3];
    return data.value;
}