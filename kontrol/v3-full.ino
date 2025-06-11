#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <sys/time.h>

// WiFi credentials
const char* ssid = "IRC-ACCESS";
const char* password = "IRCJAYA2024";

// MQTT Broker
const char* mqtt_server = "103.127.133.9";
const int mqtt_port = 1882;

// Unique ID for this ESP32
String deviceId = "XRA";

// Relay pin definitions
#define RELAY_PUMP1 5
#define RELAY_PUMP2 15
#define PIN14 14

WiFiClient espClient;
PubSubClient client(espClient);

// NTP config
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600; // GMT+7
const int daylightOffset_sec = 0;

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

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void initTime() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.print("Waiting for NTP time sync");
  time_t now = time(nullptr);
  while (now < 100000) {
    delay(100);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println(" done.");
  Serial.print("Current time: ");
  Serial.println(now);
}

void handlePumpControl(int relayPin, const String& statusTopic, const String& payloadStr) {
  String param = "";
  unsigned long long t1 = 0;

  int paramIndex = payloadStr.indexOf("\"param\":\"");
  int t1Index = payloadStr.indexOf("\"t1\":");

  if (paramIndex != -1 && t1Index != -1) {
    int paramStart = paramIndex + 9;
    int paramEnd = payloadStr.indexOf("\"", paramStart);
    param = payloadStr.substring(paramStart, paramEnd);

    int t1Start = t1Index + 5;
    int t1End = payloadStr.indexOf("}", t1Start);
    String t1Str = payloadStr.substring(t1Start, t1End);
    t1 = strtoull(t1Str.c_str(), NULL, 10);
  }

  if (param == "ON") {
    digitalWrite(relayPin, HIGH);
    Serial.println("Relay turned ON");
  } else if (param == "OFF") {
    digitalWrite(relayPin, LOW);
    Serial.println("Relay turned OFF");
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);
  unsigned long long t3 = ((unsigned long long)tv.tv_sec * 1000) + (tv.tv_usec / 1000);

  String response = "{";
  response += "\"param\":\"" + param + "\",";
  response += "\"t1\":" + String(t1) + ",";
  response += "\"t3\":" + String(t3);
  response += "}";

  client.publish(statusTopic.c_str(), (const uint8_t*)response.c_str(), response.length(), false);  // retained = true

  Serial.print("Published response to ");
  Serial.print(statusTopic);
  Serial.print(": ");
  Serial.println(response);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  for (unsigned int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(messageTemp);

  String topicStr = String(topic);
  if (topicStr == "kontrol/pump1") {
    handlePumpControl(RELAY_PUMP1, "pump1/status", messageTemp);
  } else if (topicStr == "kontrol/pump2") {
    handlePumpControl(RELAY_PUMP2, "pump2/status", messageTemp);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(deviceId.c_str())) {
      Serial.println("connected");
      client.subscribe("kontrol/pump1", 1);
      client.subscribe("kontrol/pump2", 1);
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
  pinMode(RELAY_PUMP1, OUTPUT);
  pinMode(RELAY_PUMP2, OUTPUT);
  pinMode(PIN14, OUTPUT);
  digitalWrite(RELAY_PUMP1, LOW); // Start OFF
  digitalWrite(RELAY_PUMP2, LOW); // Start OFF
  digitalWrite(PIN14, LOW); // Start OFF

  setup_wifi();
  initTime();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
