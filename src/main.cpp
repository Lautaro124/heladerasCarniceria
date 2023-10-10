#include "main.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <WiFiManager.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

float getTemperature();
void getMemorySize();
void initFirebase();
void saveTemperatureFirebase(float temperature);
void wifiConnect();

const int oneWireBus = 4;
unsigned long sendDataPrevMillis = 0;
float temperatureStorage = 0.0f;
unsigned long previousMillis = 0;
const long interval = 3000;
OneWire oneWire(oneWireBus);
DallasTemperature sensorTemperature(&oneWire);
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;

void setup()
{
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  sensorTemperature.begin();
  Serial.print("Init\n");

  WiFiManager wiFiManager;
  wiFiManager.resetSettings();

  bool res;
  res = wiFiManager.autoConnect("AutoConnect", "12345678");
  if (!res)
  {
    Serial.print("AutoConnect failed");
  }
  getMemorySize();
  // wifiConnect();
  initFirebase();
}

void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    float temperature = getTemperature();
    Serial.print("\nTemperature: ");
    Serial.print(temperature);
    Serial.print("°C");
    if (fabs(temperature - temperatureStorage) > 0.0001)
    {
      temperatureStorage = temperature;
      saveTemperatureFirebase(temperature);
      if (temperature > 30)
      {
        Serial.print("The temperature is to hot");
      }
    }
  }
}

float getTemperature()
{
  Serial.print("\nSend comand to sensors...");
  sensorTemperature.requestTemperatures();
  Serial.print("\nDone!");
  float temperature = sensorTemperature.getTempCByIndex(0);
  if (temperature != DEVICE_DISCONNECTED_C)
  {
    return temperature;
  }
  Serial.println("\nError: Could not read temperature data");
  return temperature;
}

void getMemorySize()
{
  static const char *TAG = "Main File";
  ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

  Serial.print("\nMemory size: ");
  Serial.print(esp_get_free_internal_heap_size());
  Serial.print(" bytes");
}

void saveTemperatureFirebase(float temperature)
{
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    Serial.print("\n Saving temperature in firebase...");
    Firebase.setFloat(fbdo, F("/freeze"), temperature);
  }
}

void wifiConnect()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("\nConnecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\nConnected with IP: ");
  Serial.println(WiFi.localIP());
}

void initFirebase()
{
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  config.token_status_callback = tokenStatusCallback;
  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
}
