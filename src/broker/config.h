#include"sMQTTBroker.h"
#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Smart Watering";
const char* password = "Smawa@PWD";

const char* mqtt_server = "localhost";

const char* mqtt_topic_isi_air = "/air/isi";
const char* mqtt_topic_tekanan_air = "/air/tekanan";
const char* mqtt_topic_siram = "/air/siram";

WiFiClient espClient;
PubSubClient client(espClient);

sMQTTBroker broker;
#define mqttPort 1883

void setup_broker(){
    broker.init(mqttPort);
}

void setup_client(){
    client.setServer(mqtt_server, mqttPort);
}

IPAddress local_IP(192, 168, 10, 1);
IPAddress gateway(192, 168, 10, 1);
IPAddress subnet(255, 255, 255, 0);

void StartHost() {
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("Gagal mengatur IP Address!");
    }
    WiFi.softAP(ssid, password);
    Serial.println(WiFi.localIP());
}


TaskHandle_t MQTTBrokerTask;
void brokerUpdateTask(void *parameter) {
    while (true) {
        broker.update();
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}