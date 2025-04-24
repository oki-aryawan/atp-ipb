// Ready to use

#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <BH1750.h>

#define RESET_THRESHOLD 2000 // 2 seconds to detect double reset

// DHT Setup
#define DHTPIN 23
#define DHTTYPE DHT21
DHT dht(DHTPIN, DHTTYPE);

// BH1750 Light Sensor
BH1750 lightMeter;

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);

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
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wifi Config Mode");
  lcd.setCursor(0, 1);
  lcd.print("SSID: Node-Nursery");
  lcd.setCursor(0, 2);
  lcd.print("Portal: 192.168.4.1");

  WiFiManager wm;
  wm.setTimeout(180); // 3-minute portal timeout

  if (!wm.autoConnect("Node-Nursery", "password")) {
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
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    lcd.setCursor(0, 2);
    lcd.print("MQTT: Connecting...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected.");
      lcd.setCursor(0, 2);
      lcd.print("MQTT 1882 Connected ");
      lcd.setCursor(0, 3);
      lcd.print("Topic: node-nursery");
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
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
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
  float lux = lightMeter.readLightLevel();

  if (isnan(temperature) || isnan(humidity) || lux == -1) {
    Serial.println("Sensor read failed!");
    lcd.setCursor(0, 3);
    lcd.print("Sensor Error     ");
    return;
  }

  String payload = "{";
  payload += "\"temperature_dht\":" + String(temperature, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"lux\":" + String(lux, 2);
  payload += "}";

  client.publish("esp32/node-nursery", payload.c_str());

  Serial.print("Published: ");
  Serial.println(payload);

  lcd.setCursor(0, 0);
  lcd.print("T: "); lcd.print(temperature); lcd.print("C ");
  lcd.setCursor(10, 0);
  lcd.print(" H: "); lcd.print(humidity); lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Light: "); lcd.print(lux); lcd.print(" lx    ");

  delay(2000);
}
