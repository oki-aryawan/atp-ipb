#include <WiFi.h>
#include <PubSubClient.h>

// WiFi Credentials
const char* ssid = "okinawa";
const char* password = "1234567890";

// MQTT Broker
const char* mqtt_server = "192.168.0.225"; // e.g., "192.168.1.100"

WiFiClient espClient;
PubSubClient client(espClient);

// LED Pins
const int led1Pin = 12;
const int led2Pin = 16;

// MQTT Callback
void callback(char* topic, byte* message, unsigned int length) {
  String msgStr;
  for (unsigned int i = 0; i < length; i++) {
    msgStr += (char)message[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(msgStr);

  bool state = (msgStr == "true");

  if (String(topic) == "lampu1") {
    digitalWrite(led1Pin, state ? HIGH : LOW);
    Serial.println("Lampu 1 -> " + String(state));
  } else if (String(topic) == "lampu2") {
    digitalWrite(led2Pin, state ? HIGH : LOW);
    Serial.println("Lampu 2 -> " + String(state));
  }
}

void setup_wifi() {
  delay(10);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");

      // Subscribe to control topics
      client.subscribe("lampu1");
      client.subscribe("lampu2");
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

  // Setup LED pins
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
