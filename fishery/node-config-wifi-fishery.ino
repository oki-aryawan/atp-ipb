#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define RESET_THRESHOLD 2000 // 2 seconds to detect double reset

// DHT Setup
#define DHTPIN 23
#define DHTTYPE DHT21
DHT dht(DHTPIN, DHTTYPE);

// DS18B20 Temperature Sensors
#define ONE_WIRE_BUS_1 14
#define ONE_WIRE_BUS_2 27
OneWire oneWire1(ONE_WIRE_BUS_1);
OneWire oneWire2(ONE_WIRE_BUS_2);
DallasTemperature ds18b20_1(&oneWire1);
DallasTemperature ds18b20_2(&oneWire2);

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);

// MQTT Config
const char* mqtt_server = "103.127.133.9";
const int mqtt_port = 1884;

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
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wifi Config Mode");
  lcd.setCursor(0, 1);
  lcd.print("SSID: Node-Config");
  lcd.setCursor(0, 2);
  lcd.print("Portal: 192.168.4.1");

  WiFiManager wm;
  wm.setTimeout(180); // 3-minute portal timeout

  if (!wm.autoConnect("Node-Config", "password")) {
    Serial.println("Failed to connect via portal. Restarting...");
    lcd.setCursor(0, 1);
    lcd.print("Connect failed...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("WiFi connected via config portal.");
  lcd.setCursor(0, 1);
  lcd.print("WiFi Connected");
}

void setup_wifi() {
  Serial.print("WiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
  // lcd.setCursor(0, 2);
  // lcd.print("IP: ");
  // lcd.print(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    lcd.setCursor(0, 2);
    lcd.print("MQTT: Connecting.........");
    if (client.connect("ESP32Client")) {
      Serial.println("connected.");
      lcd.setCursor(0, 2);
      lcd.print("MQTT 1884 Connected");
      lcd.setCursor(0, 3);
      lcd.print("Topic: node-fishery");
    } else {
      Serial.print("failed (rc=");
      Serial.print(client.state());
      Serial.println("). Trying again...");
      lcd.setCursor(0, 3);
      lcd.print(" Connection Failed ");
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("ESP32 Starting...");

  dht.begin();
  ds18b20_1.begin();
  ds18b20_2.begin();
  client.setServer(mqtt_server, mqtt_port);

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

  ds18b20_1.requestTemperatures();
  ds18b20_2.requestTemperatures();
  float temp1 = ds18b20_1.getTempCByIndex(0);
  float temp2 = ds18b20_2.getTempCByIndex(0);

  if (isnan(temperature) || isnan(humidity) || temp1 == DEVICE_DISCONNECTED_C || temp2 == DEVICE_DISCONNECTED_C) {
    Serial.println("Sensor read failed!");
    lcd.setCursor(0, 3);
    lcd.print("Sensor Error     ");
    return;
  }

  String payload = "{";
  payload += "\"temperature_dht\":" + String(temperature, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"temperature_ds1\":" + String(temp1, 2) + ",";
  payload += "\"temperature_ds2\":" + String(temp2, 2);
  payload += "}";

  client.publish("esp32/node-fishery", payload.c_str());

  Serial.print("Published: ");
  Serial.println(payload);

  lcd.setCursor(0, 0);
  lcd.print("T1:"); lcd.print(temp1); lcd.print("C ");
  lcd.setCursor(10, 0);
  lcd.print("T2:"); lcd.print(temp2); lcd.print("C ");
  lcd.setCursor(0, 1);
  lcd.print("DHT T:"); lcd.print(temperature); lcd.print(" H:"); lcd.print(humidity);

  delay(1000);
}
