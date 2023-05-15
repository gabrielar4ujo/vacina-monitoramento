#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  double temp;
  String id;
} struct_message;


// Create a struct_message called myData
struct_message myData;


constexpr char WIFI_SSID[] = "Multilaser";
constexpr char WIFI_PASS[] = "eletro123";

void initWiFi() {

    WiFi.mode(WIFI_MODE_APSTA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.printf("Connecting to %s .", WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) { Serial.print("."); delay(200); }
    Serial.println(" ok");

    IPAddress ip = WiFi.localIP();

    Serial.printf("SSID: %s\n", WIFI_SSID);
    Serial.printf("Channel: %u\n", WiFi.channel());
    Serial.printf("IP: %u.%u.%u.%u\n", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, ip >> 24);
}

void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
    memcpy(&myData, data, sizeof(data));
    Serial.printf("Temp received: %f from esp with id: %s.\n", myData.temp, myData.id);
}

void initEspNow() {

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP NOW failed to initialize");
        while (1);
    }

    esp_now_register_recv_cb(onReceive);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(WiFi.macAddress());
    initWiFi();
    initEspNow();

}

void loop() {}