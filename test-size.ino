#include <WiFi.h>
#include <PubSubClient.h>

// WiFi credentials
const char* ssid = "IRC-ACCESS";
const char* password = "IRCJAYA2024";

// MQTT broker
const char* mqtt_server = "103.127.133.9";
const int mqtt_port = 1884;

// Node IDs and topics
String nodeId1 = "gh-anggur"; 
String nodeId2 = "plant-factory"; 
String mqttTopic1 = "node-" + nodeId1;
String mqttTopic2 = "node-" + nodeId2;

WiFiClient espClient;
PubSubClient client(espClient);

// Dummy sensor values (replace with real sensor readings)
float temperature1 = 26.4;
float temperature2 = 20.5;
float humidity1 = 60.1;
float humidity2 = 80.5;
int moisture = 80;
int lux1 = 30000;
int lux2 = 1300;
float water = 18.9;

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
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until connected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  String payload1 = "{";
  payload1 += "\"t\":" + String(temperature1, 2) + ",";
  payload1 += "\"h\":" + String(humidity1, 2) + ",";
  payload1 += "\"m\":" + String(moisture) + ",";
  payload1 += "\"l\":" + String(lux1);
  payload1 += "}";

  String payload2 = "{";
  payload2 += "\"t\":" + String(temperature2, 2) + ",";
  payload2 += "\"h\":" + String(humidity2, 2) + ",";
  payload2 += "\"w\":" + String(water, 2) + ",";
  payload2 += "\"l\":" + String(lux2);
  payload2 += "}";

  // Convert to const char*
  const char* payload_cstr1 = payload1.c_str();
  const char* payload_cstr2 = payload2.c_str();

  // Publish to both topics
  client.publish(mqttTopic1.c_str(), payload_cstr1);
  client.publish(mqttTopic2.c_str(), payload_cstr2);

  Serial.println("Published to both topics:");
  Serial.println(mqttTopic1);
  Serial.println(mqttTopic2);
  Serial.println(payload1);
  Serial.println(payload2);

  delay(2000);  // Publish every 5 seconds
}
