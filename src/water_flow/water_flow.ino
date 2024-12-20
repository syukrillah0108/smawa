#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define WIFI_SSID       "SmartWatering"

#define MQTT_SERVER     "192.168.12.1" // Sesuaikan dengan alamat broker MQTT Anda
#define MQTT_PORT       1883
#define MQTT_USER       "user1"
#define MQTT_PASSWORD   "1234567890"
#define CLIENT_ID       "ESP8266WaterFlow"
#define TOPIC_PUBLISH_FLOW "/air/kecepatan"
#define TOPIC_PUBLISH_TOTAL "/air/total"
#define TOPIC_PUBLISH_TARGET "/air/target"
#define TOPIC_SUBSCRIBE "/set/siram"

#define FLOW_SENSOR_PIN D1 // Pin sensor water flow
#define RELAY_PIN       D2 // Pin relay untuk penyiraman
#define CALIBRATION_FACTOR 7.5 // Faktor kalibrasi YF-S201

volatile int pulseCount = 0;
float flowRate = 0;
float totalLiters = 0;
float targetLiters = 0;
int wateringActive = 0;

WiFiClient espClient;
PubSubClient client(espClient);

// Fungsi interrupt untuk menghitung pulse dari sensor water flow
void pulseCounter() {
    pulseCount++;
}

// Fungsi untuk menghitung kecepatan aliran air
void calculateFlowRate() {
    flowRate = (pulseCount / CALIBRATION_FACTOR); // L/min
    pulseCount = 0;
    totalLiters += (flowRate / 60); // Tambah liter per detik
}

// Fungsi untuk publish data ke MQTT
void publishData(const char* topic, float value) {
    char payload[50];
    snprintf(payload, sizeof(payload), "%.2f", value);
    client.publish(topic, payload);
}

void publishAllData() {
    publishData(TOPIC_PUBLISH_FLOW, flowRate);
    publishData(TOPIC_PUBLISH_TOTAL, totalLiters);
    publishData(TOPIC_PUBLISH_TARGET, targetLiters);
}

// Fungsi untuk menangani pesan yang diterima
void callback(char* topic, byte* payload, unsigned int length) {
    char msg[length + 1];
    memcpy(msg, payload, length);
    msg[length] = '\0';

    Serial.printf("Pesan diterima dari topik %s: %s\n", topic, msg);

    if (strcmp(topic, TOPIC_SUBSCRIBE) == 0) {
        float value = atof(msg);
        if (value > 0) {
            targetLiters = value;
            Serial.printf("Target volume penyiraman diterima: %.2f liter\n", targetLiters);
            totalLiters = 0;
            wateringActive = 1;
        } else {
            Serial.println("Data tidak valid");
        }
    }
}

// Fungsi untuk koneksi ke WiFi tanpa password
void setupWiFi() {
    delay(10);
    Serial.println();
    Serial.print("Menghubungkan ke ");
    Serial.println(WIFI_SSID);
    WiFi.begin(WIFI_SSID); // Tanpa password

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi terhubung");
}

// Fungsi untuk koneksi ke MQTT broker
void setupMQTT() {
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);

    while (!client.connected()) {
        Serial.print("Menghubungkan ke broker MQTT...");
        if (client.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("Terhubung");
            client.subscribe(TOPIC_SUBSCRIBE);
        } else {
            Serial.print("Gagal, rc=");
            Serial.print(client.state());
            Serial.println(" mencoba lagi dalam 5 detik");
            delay(1000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    setupWiFi();
    setupMQTT();

    pinMode(FLOW_SENSOR_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);
}

void loop() {
    if (!client.connected()) {
        setupMQTT();
    }
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    client.loop();

    calculateFlowRate();
    publishAllData();

    Serial.printf("Kecepatan Aliran: %.2f L/min, Total: %.2f L, Target: %.2f L\n", flowRate, totalLiters, targetLiters);

    if (wateringActive) {
        digitalWrite(RELAY_PIN, HIGH);
        if (totalLiters >= targetLiters) {
            digitalWrite(RELAY_PIN, LOW);
            Serial.printf("Target penyiraman tercapai: %.2f liter\n", totalLiters);
            wateringActive = 0;
        }
    }
    digitalWrite(LED_BUILTIN, LOW);
    delay(500); // Delay 1 detik
}
