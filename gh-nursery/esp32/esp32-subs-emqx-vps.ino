#include <WiFi.h>
#include <PubSubClient.h>

// WiFi Credentials
const char* ssid = "okinawa";
const char* password = "1234567890";

// EMQX Public Broker
const char* mqtt_server = "broker.emqx.io";

WiFiClient espClient;
PubSubClient client(espClient);

// Relay Pins
const int pumpPin = 12;
const int lampPin = 16;

void callback(char* topic, byte* message, unsigned int length) {
  String msgStr;
  for (int i = 0; i < length; i++) {
    msgStr += (char)message[i];
  }

  Serial.print("Topic: ");
  Serial.print(topic);
  Serial.print(" | Message: ");
  Serial.println(msgStr);

  bool state = (msgStr == "true");

  if (String(topic) == "oki/pump") {
    digitalWrite(pumpPin, state ? HIGH : LOW);
    Serial.println("Pump -> " + String(state));
  } else if (String(topic) == "oki/lamp") {
    digitalWrite(lampPin, state ? HIGH : LOW);
    Serial.println("Lamp -> " + String(state));
  }
}

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client_001")) {
      Serial.println("connected");
      client.subscribe("oki/pump");
      client.subscribe("oki/lamp");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(pumpPin, OUTPUT);
  pinMode(lampPin, OUTPUT);
  digitalWrite(pumpPin, LOW);
  digitalWrite(lampPin, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
