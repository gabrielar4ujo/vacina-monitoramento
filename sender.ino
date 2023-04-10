#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

#define CONTAINER_ID "CONTAINER-ID-1"
#define DELAY 1000

typedef struct struct_message {
  float temp;
  String id;
} struct_message;

struct_message data;
constexpr char WIFI_SSID[] = "Multilaser";

constexpr uint8_t ESP_NOW_RECEIVER[] = { 0x84, 0xcc, 0xa8, 0x64, 0xc6, 0xcc };
esp_now_peer_info_t peerInfo;
uint32_t last = 0;

int32_t getWiFiChannel(const char *ssid) {

    if (int32_t n = WiFi.scanNetworks()) {
        for (uint8_t i=0; i<n; i++) {
            if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
                return WiFi.channel(i);
            }
        }
    }

    return 0;
}

void initWiFi() {
    WiFi.mode(WIFI_MODE_STA);
    int32_t channel = getWiFiChannel(WIFI_SSID);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
}

float generateRandomData() {
  int randomNumber = random(1024);
  float randomTemp = (float)randomNumber / 10.0;
  return randomTemp;
}

void initEspNow() {
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP NOW failed to initialize");
        while (1);
    }

    memcpy(peerInfo.peer_addr, ESP_NOW_RECEIVER, 6);
    peerInfo.ifidx   = WIFI_IF_STA;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("ESP NOW pairing failure");
        while (1);
    }
}

void setup() {
    Serial.begin(115200);

    initWiFi();
    initEspNow();

    data.id = CONTAINER_ID; 
}

void loop() {
    if (millis() - last > DELAY) {
      data.temp = generateRandomData();

      esp_now_send(ESP_NOW_RECEIVER, (uint8_t *) &data, sizeof(data));
      
      Serial.printf("Temp: %f\n", data.temp);
      Serial.printf("Id: %s\n", data.id.c_str());
      Serial.printf("Sent to channel: %u\n\n", WiFi.channel());
      last = millis();
    }
}
