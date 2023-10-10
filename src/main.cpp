#include "main.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <WiFiManager.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <esp_task_wdt.h>

const int oneWireBus = 4;
unsigned long sendDataPrevMillis = 0;
float temperatureStorage = 0.0f;
unsigned long previousMillis = 0;
const long interval = 30000;
OneWire oneWire(oneWireBus);
DallasTemperature sensorTemperature(&oneWire);
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;

float getTemperature();
void getMemorySize();
void initFirebase();
void saveTemperatureFirebase(float temperature);
void wifiConnect();

void setup()
{
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  sensorTemperature.begin();
  Serial.print("Init\n");

  getMemorySize();
  wifiConnect();
  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);
  initFirebase();
}

void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    float temperature = getTemperature();
    if (isnan(temperature))
    {
      Serial.println("Error reading temperature.");
      // Handle the error as needed (e.g., reset the sensor, retry, etc.).
    }
    else
    {
      Serial.print("\nTemperature: ");
      Serial.print(temperature);
      Serial.print("°C");
      if (fabs(temperature - temperatureStorage) > 0.0001)
      {
        temperatureStorage = temperature;
        saveTemperatureFirebase(temperature);
        if (temperature > 30)
        {
          Serial.print("The temperature is too hot");
        }
      }
    }
  }
}

float getTemperature()
{
  Serial.print("\nSend command to sensors...");
  sensorTemperature.requestTemperatures();
  Serial.print("\nDone!");
  float temperature = sensorTemperature.getTempCByIndex(0);
  if (temperature != DEVICE_DISCONNECTED_C)
  {
    esp_task_wdt_reset();
    return temperature;
  }
  Serial.println("\nError: Could not read temperature data");
  return NAN; // Return a NaN value to indicate an error.
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
    Serial.print("\n Saving temperature in Firebase...");
    Firebase.setFloat(fbdo, F("/freeze"), temperature);
    if (Firebase.errorQueueCount(fbdo) > 0)
    {
      Serial.println("Failed to save temperature to Firebase.");
      // Handle the failure to save to Firebase (e.g., retry, log the error, etc.).
    }
    esp_task_wdt_reset();
  }
}

void wifiConnect()
{
  WiFiManager wiFiManager;
  wiFiManager.resetSettings();

  bool res;
  res = wiFiManager.autoConnect("Esp32 Wifi", "12345678");
  if (!res)
  {
    Serial.println("AutoConnect failed");
    // Handle the failure to connect to WiFi (e.g., retry, reset the ESP32, etc.).
  }
  else
  {
    Serial.println("\nConnected");
  }
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
