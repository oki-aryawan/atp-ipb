#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <DFRobot_B_LUX_V30B.h>

#define RESET_THRESHOLD 2000 // 2 seconds to detect double reset

// Lux sensor on custom I2C (SDA = 32, SCL = 33)
DFRobot_B_LUX_V30B myLux(/*cEN=*/0, /*scl=*/33, /*sda=*/32);

// DHT Setup
#define DHTPIN 14
#define DHTTYPE DHT21
DHT dht(DHTPIN, DHTTYPE);


// LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);

// MQTT Config
const char* mqtt_server = "103.127.133.9";
const int mqtt_port = 1882;
String nodeId = "anggrek1";
String mqttTopic = "esp32/node-" + nodeId;

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
  lcd.print("SSID: Node-Anggrek1");
  lcd.setCursor(0, 2);
  lcd.print("Portal: 192.168.4.1");

  WiFiManager wm;
  wm.setTimeout(180); // 3-minute portal timeout

  if (!wm.autoConnect("Node-Melon", "password")) {
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
      lcd.print("MQTT Connected: "); lcd.print(mqtt_port);
      lcd.setCursor(0, 3);
      lcd.print("Topic: node-"); lcd.print(nodeId);
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
  analogReadResolution(12);  // 0-4095
  Wire.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Starting...");
  lcd.setCursor(0, 1);
  lcd.print("Node Anggrek1");
  delay(2000);
  lcd.clear();

  dht.begin();
   // Lux sensor
  myLux.begin();
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
  int lux = (int)myLux.lightStrengthLux();

  if (isnan(temperature) || isnan(humidity) || lux < 0) {
    Serial.println("Sensor read failed!");
    lcd.setCursor(0, 3);
    lcd.print("Sensor Error     ");
    return;
  }

  String payload = "{";
  payload += "\"temperature_dht\":" + String(temperature, 2) + ",";
  payload += "\"humidity\":" + String(humidity, 2) + ",";
  payload += "\"lux\":" + String(lux);
  payload += "}";

  client.publish(mqttTopic.c_str(), payload.c_str());

  Serial.print("Published: ");
  Serial.println(payload);

  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temperature); lcd.print("C ");
  lcd.setCursor(10, 0);
  lcd.print(" H: "); lcd.print(humidity); lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Light:"); lcd.print(lux); lcd.print("lx         ");

  delay(2000);
}
