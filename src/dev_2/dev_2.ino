#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi configuration
#define WIFI_SSID "SmartWatering"
#define WIFI_PASSWORD "Smawa@PWD"

// MQTT configuration
#define MQTT_SERVER "192.168.25.2"
#define MQTT_PORT 1883
#define MQTT_USER "user1"
#define MQTT_PASSWORD "1234567890"
#define CLIENT_ID "ESP8266WaterFlow"

#define FLOW_SENSOR_PIN D1
#define RELAY_PIN D2
#define LED_BUILTIN D3
bool NOTIF = false; 

// MQTT topics
#define TOPIC_PUBLISH_FLOW "/siram/kecepatan"
#define TOPIC_SUBSCRIBE_SET "/siram/set"
#define TOPIC_SUBSCRIBE_OFF "/siram/off"
#define TOPIC_PUBLISH_TOTAL "/siram/total"
#define TOPIC_PUBLISH_TARGET "/siram/target"
#define TOPIC_PUBLISH_FINISH "/siram/finish"
#define TOPIC_PUBLISH_STATUS "/siram/status"

WiFiClient espClient;
PubSubClient client(espClient);

volatile unsigned int pulseCount = 0;
float flowRate = 0.0;
float totalLiters = 0.0;
float targetLiters = 0.0;

void ICACHE_RAM_ATTR pulseCounter() {
    pulseCount++;
}

void connectToWiFi() {
    //Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //Serial.print(".");
    }
    //Serial.println("Connected!");
}

void connectToMQTT() {
    while (!client.connected()) {
        //Serial.print("Connecting to MQTT...");
        if (client.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
            //Serial.println("Connected!");
            client.subscribe(TOPIC_SUBSCRIBE_SET);
            client.subscribe(TOPIC_SUBSCRIBE_OFF);
        } else {
            //Serial.print("Failed, rc=");
            //Serial.print(client.state());
            //Serial.println(" Retrying in 5 seconds...");
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    if (String(topic) == TOPIC_SUBSCRIBE_SET) {
        targetLiters = message.toFloat();
        totalLiters = 0;  // Reset total liters
        //Serial.print("Target liters set to: ");
        //Serial.println(targetLiters);
        NOTIF = true;
    }else if(String(topic) == TOPIC_SUBSCRIBE_OFF){
        totalLiters = targetLiters;
        NOTIF = true;
    }
}

void setup() {
    //Serial.begin(115200);
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    pinMode(LED_BUILTIN, OUTPUT);

    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);

    connectToWiFi();

    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);
}

void loop() {
    if (!client.connected()) {
        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(RELAY_PIN, LOW);
        connectToMQTT();
    }
    client.loop();

    // Calculate flow rate
    static unsigned long previousMillis = 0;
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= 1000) { 
        previousMillis = currentMillis;

        flowRate = (pulseCount / 7.5);
        pulseCount = 0;

        float litersThisSecond = flowRate / 60.0;
        totalLiters += litersThisSecond;

        client.publish(TOPIC_PUBLISH_FLOW, String(flowRate).c_str());
        client.publish(TOPIC_PUBLISH_TOTAL, String(totalLiters).c_str());
        client.publish(TOPIC_PUBLISH_TARGET, String(targetLiters).c_str());

    }
    digitalWrite(LED_BUILTIN, HIGH);
    delay(400);
    if (totalLiters >= targetLiters && targetLiters > 0) {
        digitalWrite(RELAY_PIN, LOW);
        client.publish(TOPIC_PUBLISH_STATUS, "1");
        if(NOTIF){
            client.publish(TOPIC_PUBLISH_FINISH, "Penyiraman Selesai");
            NOTIF = false;
        }
    } else if (targetLiters > 0) {
        digitalWrite(RELAY_PIN, HIGH);
        client.publish(TOPIC_PUBLISH_STATUS, "0");
    }
    digitalWrite(LED_BUILTIN, LOW);
    delay(400);
}
