#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Konfigurasi WiFi
const char* ssid = "SmartWatering";

// Konfigurasi MQTT
const char* mqtt_server = "192.168.25.2";
const char* mqtt_user = "user1";
const char* mqtt_password = "1234567890";
const char* tank_utama = "/air/utama";
const char* tank_nutrisi = "/air/nutrisi";
const char* relay1_topic = "/relay/1";
const char* relay2_topic = "/relay/2";

WiFiClient espClient;
PubSubClient client(espClient);

// Konfigurasi Ultrasonik dan Relay
const int trigPin1 = D5;
const int echoPin1 = D6;
const int trigPin2 = D7;
const int echoPin2 = D8;
const int relay1Pin = D1;
const int relay2Pin = D2;
#define LED_BUILTIN D3

#define MAX_HEIGHT_NUTRISI 70.0  // Ketinggian maksimum tanki air nutrisi (cm)
#define MAX_HEIGHT_UTAMA 80.0  // Ketinggian maksimum tanki air utama (cm)

// Fungsi untuk menghubungkan ke WiFi
void setup_wifi() {
    delay(10);
    Serial.println("Menghubungkan ke WiFi...");
    WiFi.begin(ssid, "Smawa@PWD");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
        Serial.println("\nWiFi terhubung");
    }

// Fungsi untuk membaca jarak dari sensor ultrasonik
float readDistance(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    long duration = pulseIn(echoPin, HIGH);
    float distance = (duration * 0.034) / 2;  // Konversi menjadi cm
    return distance;
}

// Fungsi callback saat menerima pesan MQTT
void callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    if (String(topic) == relay1_topic) {
        if (message == "ON") {
            digitalWrite(relay1Pin, HIGH);  // Relay ON
        } else if (message == "OFF") {
            digitalWrite(relay1Pin, LOW);  // Relay OFF
        }
    } else if (String(topic) == relay2_topic) {
        if (message == "ON") {
            digitalWrite(relay2Pin, HIGH);  // Relay ON
        } else if (message == "OFF") {
            digitalWrite(relay2Pin, LOW);  // Relay OFF
        }
    }
}

// Fungsi untuk menghubungkan ke MQTT
void reconnect() {
    while (!client.connected()) {
    Serial.println("Menghubungkan ke MQTT...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
        Serial.println("MQTT terhubung");
        client.subscribe(relay1_topic);
        client.subscribe(relay2_topic);
    } else {
        Serial.print("Gagal, rc=");
        Serial.print(client.state());
        Serial.println(" mencoba lagi dalam 5 detik");
        delay(5000);
    }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // Inisialisasi WiFi dan MQTT
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    // Inisialisasi pin
    pinMode(trigPin1, OUTPUT);
    pinMode(echoPin1, INPUT);
    pinMode(trigPin2, OUTPUT);
    pinMode(echoPin2, INPUT);
    pinMode(relay1Pin, OUTPUT);
    pinMode(relay2Pin, OUTPUT);

    digitalWrite(relay1Pin, LOW);
    digitalWrite(relay2Pin, LOW);
}

void loop() {
    if (!client.connected()) {
        digitalWrite(relay1Pin, LOW);
        digitalWrite(relay2Pin, LOW);
        reconnect();
    }
    delay(400);
    digitalWrite(LED_BUILTIN, LOW);
    client.loop();

    // Baca sensor ultrasonik untuk tanki utama
    float distance1 = readDistance(trigPin1, echoPin1);
    float level1 = MAX_HEIGHT_UTAMA - distance1;
    float percentage1 = (level1 / MAX_HEIGHT_UTAMA) * 100.0;
    if (percentage1 < 0) percentage1 = 0;
    if (percentage1 > 100) percentage1 = 100;

    // Baca sensor ultrasonik untuk tanki nutrisi
    float distance2 = readDistance(trigPin2, echoPin2);
    float level2 = MAX_HEIGHT_NUTRISI - distance2;
    float percentage2 = (level2 / MAX_HEIGHT_NUTRISI) * 100.0;
    if (percentage2 < 0) percentage2 = 0;
    if (percentage2 > 100) percentage2 = 100;

    // Kirim data ke MQTT
    char msg1[50];
    char msg2[50];
    snprintf(msg1, 50, "%.2f", percentage1);
    snprintf(msg2, 50, "%.2f", percentage2);

    client.publish(tank_utama, msg1);
    client.publish(tank_nutrisi, msg2);

    Serial.print("Tanki 1: ");
    Serial.print(percentage1);
    Serial.println("%");

    Serial.print("Tanki 2: ");
    Serial.print(percentage2);
    Serial.println("%");

    delay(400);
    digitalWrite(LED_BUILTIN, HIGH);
}
