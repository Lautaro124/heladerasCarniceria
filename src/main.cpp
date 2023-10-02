#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>

float getTemperature();
void getMemorySize();
void saveTemperatureFirebase(float temperature);
void initPage();

#define API_KEY "AIzaSyCDTNU55CiT0CePtyVRIRApBO85cLrwYto"
#define DATABASE_URL "https://heladerascarniceria-default-rtdb.firebaseio.com/"
#define USER_EMAIL "admin@admin.com"
#define USER_PASSWORD "admin1234"
#define WIFI_PASSWORD "GONZALEZ12"
#define WIFI_SSID "WIFI GONZALEZ 2.4"
#define PORT 80

const int oneWireBus = 4;
unsigned long sendDataPrevMillis = 0;
const char *hostName = "monitoreo";
OneWire oneWire(oneWireBus);
DallasTemperature sensorTemperature(&oneWire);
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;
AsyncWebServer server(PORT);

void setup()
{
  Serial.begin(115200);
  sensorTemperature.begin();
  Serial.print("Init\n");
  getMemorySize();

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("\nConnecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\nConnected with IP: ");
  Serial.println(WiFi.localIP());
  initPage();

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

void loop()
{
  float temperature = getTemperature();
  Serial.print("\nTemperature: ");
  Serial.print(temperature);
  Serial.print("°C");
  saveTemperatureFirebase(temperature);

  delay(5000);
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

  Serial.print("\nGet memory size");
  Serial.print(esp_get_free_internal_heap_size());
}

void saveTemperatureFirebase(float temperature)
{
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    Serial.print("\n set freeze data...");
    Firebase.setFloat(fbdo, F("/freeze"), temperature);
  }
}

void initPage()
{
  if (!MDNS.begin(hostName))
  {
    Serial.print("Error mal configurado el DNS");
  }
  Serial.print("DNS configurado");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
     File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      request->send(404, "text/plain", "Archivo no encontrado");
      return;
    }

    String htmlContent = file.readString();
    file.close();
    request->send(200, "text/html", htmlContent); });

  server.serveStatic("/", SPIFFS, "/");
  server.begin();

  MDNS.addService("http", "tcp", PORT);
}