#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"

#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"
#define API_KEY "AIzaSyAcWdnO4bmLH15I6RaVJtP-hA5jFzQ8QJ8"
#define DATABASE_URL "https://esp32-firebase-temp-31c09-default-rtdb.firebaseio.com/"
#define CONTAINER_ID "container-id-1"
#define DELAY_TO_UPDATE_TEMP_MS 5000

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

unsigned long sendDataPrevMillis = 0;
bool signUpOk = false;
int ldrData = 0;
float voltage = 0.0;
int epochTime;
const char* ntpServer = "pool.ntp.org";

//get current epoch time
unsigned long GetEpochTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return(0);
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");

  while(WiFi.status() != WL_CONNECTED){
    Serial.print("."); delay(300);
  }

  configTime(0, 0, ntpServer); // configure 'getLocalTime' to get current epoch time

  Serial.println();
  Serial.println("Connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if(Firebase.signUp(&config, &auth, "", "")){
    Serial.println("SignUp Ok!");
    signUpOk = true;
  }else {
    Serial.println("Connection Error!");
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if(Firebase.ready() && signUpOk && (millis() - sendDataPrevMillis > DELAY_TO_UPDATE_TEMP_MS || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    epochTime = GetEpochTime();

    json.set("temperature", String(sendDataPrevMillis));
    json.set("timestamp", String(epochTime));
    if(Firebase.RTDB.setJSON(&fbdo, CONTAINER_ID, &json)) {
      Serial.println("Data Updated");
    }
    else {
       Serial.println("Error: " + fbdo.errorReason());
    }
  }
}
