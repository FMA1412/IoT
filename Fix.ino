#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Konfigurasi sensor DHT
#define DHTPIN 4          // Pin untuk DHT22
#define DHTTYPE DHT22     // Tipe DHT
DHT dht(DHTPIN, DHTTYPE);

// Konfigurasi sensor MQ-135
#define MQ135PIN 7       // Pin untuk MQ-135

// Konfigurasi sensor level air
#define WATER_LEVEL_PIN 5 // Pin untuk sensor level air

// Konfigurasi sensor turbidity
#define TURBIDITY_PIN 6   // Pin untuk sensor turbidity

// Konfigurasi pin untuk fan
#define FAN_PIN 8         // Pin untuk fan

// Konfigurasi pin untuk buzzer
#define BUZZER_PIN 9      // Pin untuk buzzer

// Konfigurasi pin untuk pompa
#define PUMP_PIN 10       // Pin untuk pompa

// Konfigurasi WiFi
const char* ssid = "OPPO A17k";       // Nama WiFi
const char* password = "w5mx9cvf"; // Password WiFi

// Konfigurasi MQTT ThingsBoard
const char* mqtt_server = "demo.thingsboard.io"; // Server ThingsBoard
const char* token = "xDsoM7f6wIM9RO1gYA4o";         // Token dari ThingsBoard
WiFiClient espClient;
PubSubClient client(espClient);

// Fungsi untuk menghubungkan ke WiFi
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Fungsi untuk menghubungkan ke ThingsBoard
void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to ThingsBoard...");
    if (client.connect("ESP32Client", token, "")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(MQ135PIN, INPUT); // Set pin MQ-135 sebagai input
  pinMode(WATER_LEVEL_PIN, INPUT); // Set pin level air sebagai input
  pinMode(TURBIDITY_PIN, INPUT); // Set pin turbidity sebagai input
  pinMode(FAN_PIN, OUTPUT); // Set pin fan sebagai output
  pinMode(BUZZER_PIN, OUTPUT); // Set pin buzzer sebagai output
  pinMode(PUMP_PIN, OUTPUT); // Set pin pompa sebagai output
  digitalWrite(FAN_PIN, LOW); // Pastikan fan mati di awal
  digitalWrite(BUZZER_PIN, LOW); // Pastikan buzzer mati di awal
  digitalWrite(PUMP_PIN, LOW); // Pastikan pompa mati di awal

  connectToWiFi();
  client.setServer(mqtt_server, 1883); // Port MQTT default 1883
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  // Membaca data dari sensor DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Cek apakah pembacaan gagal
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Menghidupkan atau mematikan fan berdasarkan suhu
  if (temperature > 30) {
    digitalWrite(FAN_PIN, HIGH); // Hidupkan fan
    Serial.println("Fan ON");
  } else {
    digitalWrite(FAN_PIN, LOW); // Matikan fan
    Serial.println("Fan OFF");
  }

  // Membaca data dari sensor MQ-135
  int mq135Value = analogRead(MQ135PIN);
  float gasConcentration = map(mq135Value, 0, 1023, 0, 200); // Peta nilai ke konsentrasi gas (ppm)

  // Membaca data dari sensor level air
  int waterLevel = digitalRead(WATER_LEVEL_PIN); // 1 jika air terdeteksi, 0 jika tidak

  // Menghidupkan atau mematikan buzzer berdasarkan level air
  if (waterLevel == HIGH) { // Jika air terdeteksi
    digitalWrite(BUZZER_PIN, HIGH); // Hidupkan buzzer
    Serial.println("Buzzer ON - Water Detected");
  } else {
    digitalWrite(BUZZER_PIN, LOW); // Matikan buzzer
    Serial.println("Buzzer OFF - No Water Detected");
  }

  // Membaca data dari sensor turbidity
  int turbidityValue = analogRead(TURBIDITY_PIN); // Membaca nilai analog dari sensor turbidity

  // Menghidupkan atau mematikan pompa berdasarkan nilai turbidity
  if (turbidityValue > 50) { // Jika nilai turbidity melebihi 50
    digitalWrite(PUMP_PIN, HIGH); // Hidupkan pompa
    Serial.println("Pump ON - Turbidity High");
  } else {
    digitalWrite(PUMP_PIN, LOW); // Matikan pompa
    Serial.println("Pump OFF - Turbidity Normal");
  }

  // Menampilkan data ke Serial Monitor
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");
  
  Serial.print("MQ-135 Value: ");
  Serial.print(mq135Value);
  Serial.print(" (mapped to ppm: ");
  Serial.print(gasConcentration);
  Serial.println(" ppm)");

  Serial.print("Water Level: ");
  Serial.print(waterLevel == HIGH ? "Water Detected" : "No Water Detected");
  Serial.println();

  Serial.print("Turbidity Value: ");
  Serial.println(turbidityValue); // Menampilkan nilai turbidity

  // Membuat payload JSON
  String payload = "{";
  payload += "\"temperature\":" + String(temperature) + ",";
  payload += "\"humidity\":" + String(humidity) + ",";
  payload += "\"gas_concentration\":" + String(gasConcentration) + ",";
  payload += "\"water_level\":" + String(waterLevel) + ",";
  payload += "\"turbidity\":" + String(turbidityValue);
  payload += "}";

  // Mengirim data ke ThingsBoard
  if (client.publish("v1/devices/me/telemetry", payload.c_str())) {
    Serial.println("Data sent to ThingsBoard");
  } else {
    Serial.println("Failed to send data to ThingsBoard");
  }

  // Delay sebelum pengulangan
  delay(2000);
}