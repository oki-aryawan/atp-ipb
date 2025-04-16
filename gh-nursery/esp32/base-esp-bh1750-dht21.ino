#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>

// Define DHT11 settings
#define DHTPIN 13          // Connect DHT11 data pin to GPIO4
#define DHTTYPE DHT21     // DHT 11

DHT dht(DHTPIN, DHTTYPE);

// Create BH1750 object
BH1750 lightMeter;

void setup() {
  Serial.begin(115200);
  Wire.begin();  // Initialize I2C

  // Initialize sensors
  dht.begin();
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 started");
  } else {
    Serial.println("Could not find BH1750 sensor!");
  }
}

void loop() {
  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Read light level
  float lux = lightMeter.readLightLevel();

  // Print readings
  Serial.println("----- Sensor Readings -----");
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT11 sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }

  Serial.print("Light Intensity: ");
  Serial.print(lux);
  Serial.println(" lx");

  Serial.println("----------------------------\n");

  delay(2000); // Wait for 2 seconds before next reading
}
