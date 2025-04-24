#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// WiFi Credentials
const char* ssid = "Digital Faming";
const char* password = "atpipb2024";

// MQTT Broker
const char* mqtt_server = "103.127.133.9";
const int mqtt_port = 1882;

// DHT11 Setup
#define DHTPIN 13
#define DHTTYPE DHT21
DHT dht(DHTPIN, DHTTYPE);

// BH1750
BH1750 lightMeter;

// DS18B20 Setup
#define ONE_WIRE_BUS 14  // GPIO for DS18B20 data line
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

// WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Function to connect to Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to connect to MQTT Broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  dht.begin();
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  ds18b20.begin();  // Initialize DS18B20

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float lux = lightMeter.readLightLevel();

  // Read DS18B20
  ds18b20.requestTemperatures();
  float ds18b20Temp = ds18b20.getTempCByIndex(0);

  if (isnan(temperature) || isnan(humidity) || isnan(lux) || ds18b20Temp == DEVICE_DISCONNECTED_C) {
    Serial.println("Sensor read failed!");
    return;
  }

  // Format to JSON-style payload
  String payload = "{";
  payload += "\"temperature_dht\":" + String(temperature, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"lux\":" + String(lux, 2) + ",";
  payload += "\"temperature_ds18b20\":" + String(ds18b20Temp, 2);
  payload += "}";

  // Publish to MQTT
  client.publish("esp32/node-pf", payload.c_str());

  Serial.print("Published: ");
  Serial.println(payload);

  delay(1000); // Adjust delay as needed
}
