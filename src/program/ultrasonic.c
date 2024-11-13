#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <MQTTClient.h>
#include <string.h>

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "WaterFlowClient"
#define USERNAME    "user1"
#define PASSWORD    "1234567890"
#define TOPIC_PUBLISH "/air/kecepatan"
#define TOPIC_SUBSCRIBE "/set/siram"
#define QOS         2
#define TIMEOUT     10000L

#define FLOW_SENSOR_PIN  7 // Gunakan pin wiringBP
#define RELAY_PIN        2 // Gunakan pin wiringBP

// Konstanta sensor YF-S201
#define CALIBRATION_FACTOR 7.5 // faktor kalibrasi untuk YF-S201

volatile int pulseCount = 0;
float flowRate = 0;
float totalLiters = 0;
float targetLiters = 0;
int wateringActive = 0; // Status aktif/tidaknya penyiraman

MQTTClient client;

void pulseCounter() {
    pulseCount++;
}

void calculateFlowRate() {
    flowRate = (pulseCount / CALIBRATION_FACTOR); // Kecepatan air dalam L/min
    pulseCount = 0; // Reset hitungan pulse
    totalLiters += (flowRate / 60); // Tambah total liter dengan liter per detik
}

void publishFlowRate() {
    char payload[50];
    snprintf(payload, sizeof(payload), "%.2f", flowRate);

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;

    MQTTClient_publishMessage(client, TOPIC_PUBLISH, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);
}

void messageArrivedHandler(char* payload) {
    targetLiters = atof(payload);
    printf("Target volume penyiraman diterima: %.2f liter\n", targetLiters);
    totalLiters = 0; // Reset volume total saat target baru diterima
    wateringActive = 1; // Aktifkan penyiraman
}

int messageArrived(void* context, char* topicName, int topicLen, MQTTClient_message* message) {
    char payload[message->payloadlen + 1];
    memcpy(payload, message->payload, message->payloadlen);
    payload[message->payloadlen] = '\0';

    if (strcmp(topicName, TOPIC_SUBSCRIBE) == 0) {
        messageArrivedHandler(payload);
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

int main(void) {
    if (wiringPiSetup() == -1) {
        printf("Gagal menginisialisasi WiringPi\n");
        return 1;
    }

    pinMode(FLOW_SENSOR_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    // Set up interrupt untuk counter pulse
    wiringPiISR(FLOW_SENSOR_PIN, INT_EDGE_FALLING, &pulseCounter);

    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = USERNAME;
    conn_opts.password = PASSWORD;

    if (MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS) {
        printf("Gagal terhubung ke broker MQTT\n");
        return 1;
    }
    printf("Berhasil terhubung ke MQTT\n");

    MQTTClient_subscribe(client, TOPIC_SUBSCRIBE, QOS);

    while (1) {
        delay(1000); // Perhitungan tiap 1 detik

        calculateFlowRate();
        publishFlowRate();
        printf("Kecepatan Aliran: %.2f L/min, Total: %.2f L\n", flowRate, totalLiters);

        if (wateringActive) {
            if (totalLiters >= targetLiters) {
                digitalWrite(RELAY_PIN, LOW); // Matikan penyiraman
                printf("Target penyiraman tercapai: %.2f liter\n", totalLiters);
                wateringActive = 0;
            } else {
                digitalWrite(RELAY_PIN, HIGH); // Aktifkan penyiraman
            }
        }
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return 0;
}
