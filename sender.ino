#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Define o pino ao qual o sensor de temperatura está conectado
#define ONE_WIRE_BUS 32

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define CONTAINER_ID "refrigerador-1"
#define DELAY 1000

typedef struct struct_message {
  float temp;
  String id;
} struct_message;

struct_message data;
constexpr char WIFI_SSID[] = "Multilaser";

constexpr uint8_t ESP_NOW_RECEIVER[] = { 0x34, 0x86, 0x5D, 0xFF, 0x38, 0x48 };
esp_now_peer_info_t peerInfo;
uint32_t last = 0;

float previousTemp = NULL;
float currentTemp =  NULL; 

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

float getCurrentTemp() {
  sensors.requestTemperatures(); 
  float temperaturaC = sensors.getTempCByIndex(0);

  return temperaturaC;
}

void setup() {
    Serial.begin(115200);

    initWiFi();
    initEspNow();
    sensors.begin();

    data.id = CONTAINER_ID; 
}

void loop() {
    if (millis() - last > DELAY) {
      currentTemp = getCurrentTemp();

      if(currentTemp < 40 && currentTemp > -5)
        {
        if(previousTemp == NULL) {
          previousTemp = getCurrentTemp();
        }

        if(abs(currentTemp - previousTemp) > 30) {
          Serial.println("Diferença de temperatura muito grande. Erro!");
        }

        else {
          previousTemp = currentTemp;
          data.temp = (float)currentTemp;
          esp_now_send(ESP_NOW_RECEIVER, (uint8_t *) &data, sizeof(data));
          Serial.printf("Id: %s\n", data.id.c_str());
          Serial.printf("Sent to channel: %u\n\n", WiFi.channel());
          last = millis();
        }
      }
    else {
       Serial.println("Temperatura fora do escopo normal. Erro!");
    }
    Serial.printf("Temp: %f\n", currentTemp);
    }
}
