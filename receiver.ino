#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"
#include <TimeLib.h>

#define WIFI_SSID "Multilaser"
#define WIFI_PASS "eletro123"
#define API_KEY "AIzaSyAcWdnO4bmLH15I6RaVJtP-hA5jFzQ8QJ8"
#define DATABASE_URL "https://esp32-firebase-temp-31c09-default-rtdb.firebaseio.com/"
#define ARRAY_SIZE 999
#define PROJECT_ID "esp32-firebase-temp-31c09"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

bool signUpOk = false;
int epochTime;
const char* ntpServer = "pool.ntp.org";

typedef struct struct_message {
  float temp;
  String id;
  int timestamp;
} struct_message;


struct_message myData;
struct struct_message myDatas[ARRAY_SIZE];

int receiveCount = 0;
int sendCount = 0;

unsigned long getEpochTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return(0);
  }
  time(&now);
  return now;
}

String epochTimeToString(unsigned long epochTime) {
  // Configurar o valor do tempo
  setTime(epochTime);
  
  // Obter os componentes da data
  int dia = day();
  int mes = month();
  int ano = year();
  
  // Construir a string de data no formato desejado (dia/mês/ano)
  String data = String(dia) + "/" + String(mes) + "/" + String(ano);
  
  return data;
}

void initWiFi() {
    WiFi.mode(WIFI_MODE_STA);
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
    memcpy(&myData, data, sizeof(struct_message));
    myDatas[receiveCount].id = myData.id;
    myDatas[receiveCount].temp = myData.temp;
    myDatas[receiveCount].timestamp = getEpochTime();
    
    Serial.printf("Data Received: [%s] - Temperatura: %f\n", myData.id.c_str(), myData.temp);
    if(receiveCount + 1 == ARRAY_SIZE) {
      receiveCount = 0;
    }
    else {
       receiveCount++;
    }
}

void sendDataToFirebase() {
    json.set("temperature", String(myDatas[sendCount].temp));
    json.set("timestamp", String(myDatas[sendCount].timestamp));

    String currentDate = epochTimeToString(myDatas[sendCount].timestamp);
    String documentPath = myDatas[sendCount].id;

    FirebaseJson content;
    content.set("fields/temperature/doubleValue", String(myDatas[sendCount].temp).c_str());
    content.set("fields/timestamp/doubleValue", String(myDatas[sendCount].timestamp).c_str());
    content.set("fields/date/stringValue", String(currentDate).c_str());

    if(Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "",  documentPath.c_str(),  content.raw())){
      Serial.println("Document Created in Firestore");
    } else {
      Serial.println("Error: " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setJSON(&fbdo, myDatas[sendCount].id, &json)) {
      Serial.printf("Data Updated: [%s] - Temperatura: %f\n", myDatas[sendCount].id.c_str(), myDatas[sendCount].temp);
      if(sendCount + 1 == ARRAY_SIZE) {
        sendCount = 0;
      }
      else {
        sendCount++;
      }
    }
    else {
      Serial.println("Error: " + fbdo.errorReason());
    }
  
}

void initEspNow() {
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP NOW failed to initialize");
        while (1);
    }
    esp_now_register_recv_cb(onReceive);
}

void initFirebase(){
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if(Firebase.signUp(&config, &auth, "", "")){
    Serial.println("Firebase signUp Ok!");
    signUpOk = true;
  }else {
    Serial.println("Firebase connection Error!");
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void setup() {
    Serial.begin(115200);
    delay(500);
    configTime(0, 0, ntpServer);
    Serial.println(WiFi.macAddress());
    initWiFi();
    initFirebase();
    initEspNow();
}

void loop() {
   if(Firebase.ready() && signUpOk && sendCount != receiveCount){
     sendDataToFirebase();
   }
}