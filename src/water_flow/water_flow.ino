#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "sensor.h"

const char* ssid = "realme C25s";
const char* password = "code787898";

const char* mqtt_server = "192.168.142.181";
const char* mqtt_topic_flow_rate = "flow/rate";
const char* mqtt_topic_flow_total = "flow/total";

const char* mqtt_username = "user1";
const char* mqtt_password = "1234567890";

WiFiClient espClient;
PubSubClient client(espClient);

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
  Serial.println(WiFi.localIP());
  Serial.println("WiFi connected");
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("WaterFlowSensor", mqtt_username, mqtt_password)) {
        Serial.println("connected");
    } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.print(" - ");
        Serial.println(mqtt_server);
        delay(5000);
    }

  }
}

void setup() {
  Serial.begin(9600);
  setup_sensor();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  get_data();
  client.publish(mqtt_topic_flow_rate, String(flowRate).c_str());
  client.publish(mqtt_topic_flow_total, String(totalLiters).c_str());
  Serial.print("Published: ");
}
