#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define RESET_THRESHOLD 2000 // 2 seconds to detect double reset

// DHT Setup
#define DHTPIN 13
#define DHTTYPE DHT21
DHT dht(DHTPIN, DHTTYPE);

// BH1750 Light Sensor
BH1750 lightMeter;

// DS18B20 Temperature Sensor
#define ONE_WIRE_BUS 14
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// MQTT Config
const char* mqtt_server = "103.127.133.9";
const int mqtt_port = 1882;

// WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Function to check and handle double reset
bool checkDoubleReset() {
  static unsigned long lastResetTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastResetTime < RESET_THRESHOLD) {
    return true; // Double reset detected
  }
  
  lastResetTime = currentTime;
  return false;
}

void startWiFiConfigPortal() {
  Serial.println("Starting WiFiManager config portal...");

  WiFiManager wm;
  wm.setTimeout(180); // 3-minute portal timeout

  if (!wm.autoConnect("ESP32-Config", "password")) {
    Serial.println("Failed to connect via portal. Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("WiFi connected via config portal.");
}

void setup_wifi() {
  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected.");
    } else {
      Serial.print("failed (rc=");
      Serial.print(client.state());
      Serial.println("). Trying again...");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  dht.begin();
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  ds18b20.begin();
  client.setServer(mqtt_server, mqtt_port);

  // Check for double reset (using simple millis comparison)
  if (checkDoubleReset()) {
    Serial.println("Double Reset Detected – Entering WiFi Config Mode");
    startWiFiConfigPortal();
  } else {
    Serial.println("Normal Boot – Attempting WiFi Connection...");
    WiFi.begin();
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      Serial.print(".");
      delay(500);
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\nWiFi Failed – Falling back to Config Mode");
      startWiFiConfigPortal();
    }
  }

  setup_wifi();
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float lux = lightMeter.readLightLevel();

  ds18b20.requestTemperatures();
  float ds18b20Temp = ds18b20.getTempCByIndex(0);

  if (isnan(temperature) || isnan(humidity) || isnan(lux) || ds18b20Temp == DEVICE_DISCONNECTED_C) {
    Serial.println("Sensor read failed!");
    return;
  }

  String payload = "{";
  payload += "\"temperature_dht\":" + String(temperature, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"lux\":" + String(lux, 2) + ",";
  payload += "\"temperature_ds18b20\":" + String(ds18b20Temp, 2);
  payload += "}";

  client.publish("esp32/node-pf", payload.c_str());

  Serial.print("Published: ");
  Serial.println(payload);

  delay(1000);
}
