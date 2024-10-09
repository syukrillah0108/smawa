//Library
    #include <ESP8266WiFi.h>
    #include <PubSubClient.h>

String
    ssid, password, mqtt_server,
    mqtt_topic_flow_rate,
    mqtt_topic_flow_total,
    mqtt_username, mqtt_password;

int pulseCount;
float 
    flowRate, duration,
    totalLiters;

WiFiClient 
    espClient;
PubSubClient
    client(espClient);

void IRAM_ATTR
    countPulse() {
    pulseCount++;
}

void reconnect() {
    // Koneksi ulang
}

void get_data(){
    pulseCount = 0;
    duration = 10000;
    delay(duration);

    flowRate = (
        pulseCount / 7.5);
    totalLiters += (
        flowRate * (
        duration / 60000.0)
    );
}

void setup() {
  Serial.begin(9600);
  setup_wifi(ssid, password);
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  get_data();
  client.publish(
        mqtt_topic_flow_rate,
        String(flowRate).c_str());
  client.publish(
        mqtt_topic_flow_total,
        String(totalLiters).c_str());
}