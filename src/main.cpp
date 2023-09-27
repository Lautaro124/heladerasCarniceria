#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

float getTemperature();

#define API_KEY "AIzaSyCDTNU55CiT0CePtyVRIRApBO85cLrwYto"
#define DATABASE_URL "https://heladerascarniceria-default-rtdb.firebaseio.com/"
#define USER_EMAIL "admin@admin.com"
#define USER_PASSWORD "admin1234"
#define WIFI_PASSWORD "GONZALEZ12"
#define WIFI_SSID "WIFI GONZALEZ 2.4"

const int oneWireBus = 4;
unsigned long sendDataPrevMillis = 0;
DallasTemperature sensorTemperature(&oneWire);
FirebaseAuth auth;
FirebaseConfig config;
FirebaseData fbdo;
OneWire oneWire(oneWireBus);

void setup()
{
  Serial.begin(115200);
  sensorTemperature.begin();
  Serial.print("Init");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

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
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C");

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    Serial.print("\n set freeze data...");
    Firebase.setFloat(fbdo, F("/freeze"), temperature);
  }
  delay(9000);
}

float getTemperature()
{
  Serial.print("\nSend comand to sensors...");
  float temperature = sensorTemperature.getTempCByIndex(0);
  return temperature;
}