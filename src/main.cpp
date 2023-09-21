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
#define MAX_DISCO_PACKET_SIZE 4096
#define MAX_COLOR 255

// TODO: web server for AP configuration
// AsyncWebServer server(80);

// AsyncServer server = AsyncServer(CONFIG_PORT);
Adafruit_NeoPixel pixels(150, 2, NEO_GRB + NEO_KHZ800);
WiFiUDP udp;
byte udp_packet_buffer[MAX_DISCO_PACKET_SIZE];

void process_udp_packet(int length);
uint32_t convert_bytes_to_int(byte* array, int offset);
float convert_bytes_to_float(byte* array, int offset);

void setup() {
    Serial.begin(115200);
    delay(10);

    DEBUG_LN("TEST");

    // TODO: AP configuration
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

    // TODO: Web server for AP configuration
    // server.on("/", HTTP_GET, [](AsyncWebServerRequest* request){
    //     request->send(200, "text/plain", "Hello, World!");
    // });
    // server.begin();

    MDNS.end();
    DEBUG_LN("Starting mDNS");
    DEBUG_LN(MDNS.begin("disco", WiFi.localIP()) ? "Success" : "Failure");
    
    // Add disco data protocol service
    auto disco_udp_service = MDNS.addService(0 ,"disco", "udp", DISCO_PORT);
    MDNS.addServiceTxt(disco_udp_service, "device", "esp8266");
    MDNS.addServiceTxt(disco_udp_service, "version", 1);
    MDNS.addServiceTxt(disco_udp_service, "preferred-ipv", 4);
    
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
        if (len < 4) {
            Serial.printf("Got invalid packet of length: %d, wanting at least 4", len);
            return;
        }
        process_udp_packet(len);
    }
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

uint32_t convert_bytes_to_long(byte* array, int offset) {
    union byte_long {
        byte array[8];
        uint64_t value;
    };
    byte_long data;
    data.array[0] = array[offset + 0];
    data.array[1] = array[offset + 1];
    data.array[2] = array[offset + 2];
    data.array[3] = array[offset + 3];
    data.array[4] = array[offset + 4];
    data.array[5] = array[offset + 5];
    data.array[6] = array[offset + 6];
    data.array[7] = array[offset + 7];
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

void process_ledA_packet(int len) {
    if (len < 16) {
        Serial.printf("Got invalid packet of length: %d, wanting 16\n", len);
        return;
    }
    uint32_t start = convert_bytes_to_int(udp_packet_buffer, 4);
    uint32_t end = convert_bytes_to_int(udp_packet_buffer, 8);
    uint32_t n_frames = convert_bytes_to_int(udp_packet_buffer, 12);
    int pixels_per_frame = end - start;
    int total_pixels = pixels_per_frame * n_frames;
    if (len < 16 + total_pixels * 4) {
        Serial.printf("Got invalid packet of length: %d, wanting: %d\n", len, 16 + total_pixels * 4);
        return;
    }
    // TODO: deal with multiple frames
    for (int i = 0; i < pixels_per_frame; i++) {
        uint8_t r = udp_packet_buffer[16 + i * 4];
        uint8_t g = udp_packet_buffer[17 + i * 4];
        uint8_t b = udp_packet_buffer[18 + i * 4];
        uint8_t a = udp_packet_buffer[19 + i * 4];
        pixels.setPixelColor(start + i, r, g, b);
    }
    pixels.show();
}

void process_heartbeat_packet(int len) {
    if (len < 16) {
        Serial.printf("Got invalid packet of length: %d, wanting 16\n", len);
        return;
    }
    udp_packet_buffer[3] = 'B';
    
    uint64_t timestamp = convert_bytes_to_long(udp_packet_buffer, 4);
    Serial.printf("Got heartbeat - timestamp: %lld\n", timestamp);

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(udp_packet_buffer, len);
    Serial.printf("Send response with code: %d\n", udp.endPacket());
}

void process_udp_packet(int length) {
    std::string packet_type((char*) udp_packet_buffer, 4);
    if (packet_type == "LEDA")
        process_ledA_packet(length);
    else if (packet_type == "HRBA")
        process_heartbeat_packet(length);
}