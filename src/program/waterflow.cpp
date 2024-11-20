#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "MQTTAsync.h"

#define ADDRESS        "tcp://localhost:1883"
#define CLIENTID       "WaterFlowClient"
#define USERNAME       "user1"
#define PASSWORD       "1234567890"
#define TOPIC_PUBLISH_FLOW "/air/kecepatan"
#define TOPIC_PUBLISH_TOTAL "/air/total"
#define TOPIC_PUBLISH_TARGET "/air/target"
#define TOPIC_SUBSCRIBE "/set/siram"
#define QOS            2
#define TIMEOUT        10000L

#define FLOW_SENSOR_PIN 10 // Sensor water flow
#define RELAY_PIN       14 // Relay untuk penyiraman
#define CALIBRATION_FACTOR 7.5 // Faktor kalibrasi YF-S201

volatile int pulseCount = 0;
float flowRate = 0;
float totalLiters = 0;
float targetLiters = 0;
int wateringActive = 0;

MQTTAsync client;

// Fungsi interrupt untuk menghitung pulse dari sensor water flow
void pulseCounter() {
    pulseCount++;
}

void calculateFlowRate() {
    flowRate = (pulseCount / CALIBRATION_FACTOR); // L/min
    pulseCount = 0;
    totalLiters += (flowRate / 60); // Tambah liter per detik
}

void publishData(const char* topic, float value) {
    char payload[50];
    snprintf(payload, sizeof(payload), "%.2f", value);

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTAsync_sendMessage(client, topic, &pubmsg, NULL);
}

void publishAllData() {
    publishData(TOPIC_PUBLISH_FLOW, flowRate);
    publishData(TOPIC_PUBLISH_TOTAL, totalLiters);
    publishData(TOPIC_PUBLISH_TARGET, targetLiters);
}

// Fungsi untuk menangani pesan yang diterima
int msgarrvd(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
    char payload[message->payloadlen + 1];
    memcpy(payload, message->payload, message->payloadlen);
    payload[message->payloadlen] = '\0';

    printf("Pesan diterima dari topik %s: %s\n", topicName, payload);

    if (strcmp(topicName, TOPIC_SUBSCRIBE) == 0) {
        float value = atof(payload);
        if (value > 0) {
            targetLiters = value;
            printf("Target volume penyiraman diterima: %.2f liter\n", targetLiters);
            totalLiters = 0;
            wateringActive = 1;
        } else {
            printf("Data tidak valid: %s\n", payload);
        }
    }

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onConnect(void* context, MQTTAsync_successData* response) {
    printf("Berhasil terhubung ke broker MQTT\n");

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = NULL;
    opts.onFailure = NULL;
    opts.context = client;

    if (MQTTAsync_subscribe(client, TOPIC_SUBSCRIBE, QOS, &opts) != MQTTASYNC_SUCCESS) {
        printf("Gagal subscribe ke topik %s\n", TOPIC_SUBSCRIBE);
    }
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("Gagal terhubung ke broker MQTT, kode: %d\n", response->code);
}

void connlost(void* context, char* cause) {
    printf("\nKoneksi terputus: %s\n", cause);
    printf("Mencoba menghubungkan kembali...\n");

    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = USERNAME;
    conn_opts.password = PASSWORD;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;

    MQTTAsync_connect(client, &conn_opts);
}

void setupMQTT() {
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_setCallbacks(client, client, connlost, msgarrvd, NULL);

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = USERNAME;
    conn_opts.password = PASSWORD;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;

    if (MQTTAsync_connect(client, &conn_opts) != MQTTASYNC_SUCCESS) {
        printf("Gagal terhubung ke broker MQTT\n");
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    if (wiringPiSetup() == -1) {
        printf("Gagal menginisialisasi WiringPi\n");
        return 1;
    }

    pinMode(FLOW_SENSOR_PIN, INPUT);
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    wiringPiISR(FLOW_SENSOR_PIN, INT_EDGE_FALLING, &pulseCounter);

    setupMQTT();

    while (1) {
        calculateFlowRate();
        publishAllData();

        printf("Kecepatan Aliran: %.2f L/min, Total: %.2f L, Target: %.2f L\n", flowRate, totalLiters, targetLiters);

        if (wateringActive) {
            digitalWrite(RELAY_PIN, HIGH);
            if (totalLiters >= targetLiters) {
                digitalWrite(RELAY_PIN, LOW);
                printf("Target penyiraman tercapai: %.2f liter\n", totalLiters);
                wateringActive = 0;
            }
        }
        usleep(1000000); // Delay 1 detik
    }

    MQTTAsync_disconnect(client, NULL);
    MQTTAsync_destroy(&client);

    return 0;
}