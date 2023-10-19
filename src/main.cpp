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
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

String token = "Bearer EAAIao2QuMP4BAEg4Hs76IvHZAeBuZCYP9lCnipU1hlxDmTcw77orMZCaawAoAusMNvvDlETU5d1uxiwjvB72j2DW6UOJqxIxZCofP6apbuitVtZBjueB4HZBXZBhIY64JN75tWFVY1UQbBo9gZAZBWl776gC3khAPcgO4tiykQ1CPpZBXTBCEeRZB49dSUgZB2rYW2OYr7NWKgzEIgZDZD";
String payload = "{\"messaging_product\":\"whatsapp\",\"to\":\"527121122441\",\"type\":\"text\",\"text\": {\"body\": \"Temperatura muy alta\"}}";

const int oneWireBus = 4;
unsigned long sendDataPrevMillis = 0;
float temperatureStorage = 0.0f;
unsigned long previousMillis = 0;
const long interval = 30000;
OneWire oneWire(oneWireBus);
DallasTemperature sensorTemperature(&oneWire);
HTTPClient http;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;
bool lastInternetStatus = false;

float getTemperature();
void getMemorySize();
void initFirebase();
void saveTemperatureFirebase(float temperature);
void wifiConnect();
void connectWiFi(const char *ssid, const char *password);
void configureWiFi();
bool checkInternetStatus();
void updateInternetStatusFirebase(bool status);

void setup()
{
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  sensorTemperature.begin();
  Serial.print("Init\n");
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  getMemorySize();
  
  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);
  File configFile = SPIFFS.open("/config.json", "r");
  if (configFile)
  {
    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    configFile.close();

    DynamicJsonDocument jsonDoc(1024);
    DeserializationError error = deserializeJson(jsonDoc, buf.get());
    if (!error)
    {
      const char *ssid = jsonDoc["ssid"];
      const char *password = jsonDoc["password"];
      
      connectWiFi(ssid, password);
      initFirebase();
      return;
    }
  }

  configureWiFi();
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

  // Verifica el estado de Internet y actualiza Firebase si es necesario
  bool currentInternetStatus = checkInternetStatus();
  if (currentInternetStatus != lastInternetStatus)
  {
    lastInternetStatus = currentInternetStatus;
    updateInternetStatusFirebase(currentInternetStatus);
  }
}

bool checkInternetStatus()
{
  // Verifica si hay conexión a Internet
  return WiFi.status() == WL_CONNECTED;
}

void updateInternetStatusFirebase(bool status)
{
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    Serial.print("\nUpdating Internet Status in Firebase...");
    Firebase.setBool(fbdo, F("/internet_status"), status);
    if (Firebase.errorQueueCount(fbdo) > 0)
    {
      Serial.println("Failed to update Internet Status in Firebase.");
    }
    esp_task_wdt_reset();
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
  return NAN; 
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

void configureWiFi()
{
  
  WiFiManager wifiManager;

  
  wifiManager.autoConnect("ESP32-AP");

  
  DynamicJsonDocument jsonDoc(1024);
  jsonDoc["ssid"] = WiFi.SSID();
  jsonDoc["password"] = WiFi.psk();

  File configFile = SPIFFS.open("/config.json", "w");
  if (configFile)
  {
    serializeJson(jsonDoc, configFile);
    configFile.close();
  }
}

void connectWiFi(const char *ssid, const char *password)
{
  
  Serial.print("Conectando a: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Conectando...");
  }

  Serial.println("Conexión exitosa");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());
}