#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <MQTTClient.h>
#include <string.h>

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "BananaPiClient"
#define USERNAME    "user1"
#define PASSWORD    "1234567890"
#define TOPIC1      "/air/utama"
#define TOPIC2      "/air/nutrisi"
#define QOS         2
#define TIMEOUT     10000L

// Sensor 1
#define TRIG_PIN_1  27
#define ECHO_PIN_1  28

// Sensor 2
#define TRIG_PIN_2  25
#define ECHO_PIN_2  24

#define TANGKI_TINGGI_CM 100.0
#define ECHO_TIMEOUT 30000 // timeout 30ms

float persentase1, persentase2;

double measureDistance(int trigPin, int echoPin) {
    long startTime, travelTime;

    // Kirim sinyal trigger
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Tunggu sinyal ECHO HIGH dengan timeout
    long timeoutStart = micros();
    while (digitalRead(echoPin) == LOW) {
        if (micros() - timeoutStart > ECHO_TIMEOUT) {
            return -1; // timeout, tidak ada respons dari sensor
        }
    }

    startTime = micros();
    while (digitalRead(echoPin) == HIGH) {
        if (micros() - startTime > ECHO_TIMEOUT) {
            return -1; // timeout, ECHO terlalu lama
        }
    }

    travelTime = micros() - startTime;

    // Hitung jarak dalam cm
    double distance = (travelTime * 0.0343) / 2;
    return distance;
}

void publishToMQTT(MQTTClient client, const char* topic, const char* payload) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    pubmsg.payload = (void*)payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);
}

int main(void) {
    if (wiringPiSetup() == -1) {
        printf("Gagal menginisialisasi WiringPi\n");
        return 1;
    }

    pinMode(TRIG_PIN_1, OUTPUT);
    pinMode(ECHO_PIN_1, INPUT);
    pinMode(TRIG_PIN_2, OUTPUT);
    pinMode(ECHO_PIN_2, INPUT);

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = USERNAME;
    conn_opts.password = PASSWORD;

    if (MQTTClient_connect(client, &conn_opts) != MQTTCLIENT_SUCCESS) {
        printf("Gagal terhubung ke broker MQTT\n");
        return 1;
    } else {
        printf("Berhasil terhubung ke MQTT\n");
    }

    while (1) {
        // Sensor 1
        double rawDistance1 = measureDistance(TRIG_PIN_1, ECHO_PIN_1);
        char payload1[50];

        if (rawDistance1 == -1) {
            snprintf(payload1, sizeof(payload1), "err");
            printf("Sensor 1 Error: Tidak ada respons dari sensor\n");
        } else {
            persentase1 = (TANGKI_TINGGI_CM - rawDistance1) / TANGKI_TINGGI_CM * 100;
            if (persentase1 <= 0) {
                snprintf(payload1, sizeof(payload1), "err");
            } else {
                snprintf(payload1, sizeof(payload1), "%.2f", persentase1);
            }
        }

        // Kirim data Sensor 1 ke MQTT
        publishToMQTT(client, TOPIC1, payload1);
        printf("Data Sensor 1 dikirim ke MQTT: %s\n", payload1);

        // Sensor 2
        double rawDistance2 = measureDistance(TRIG_PIN_2, ECHO_PIN_2);
        char payload2[50];

        if (rawDistance2 == -1) {
            snprintf(payload2, sizeof(payload2), "err");
            printf("Sensor 2 Error: Tidak ada respons dari sensor\n");
        } else {
            persentase2 = (TANGKI_TINGGI_CM - rawDistance2) / TANGKI_TINGGI_CM * 100;
            if (persentase2 <= 0) {
                snprintf(payload2, sizeof(payload2), "err");
            } else {
                snprintf(payload2, sizeof(payload2), "%.2f", persentase2);
            }
        }

        // Kirim data Sensor 2 ke MQTT
        publishToMQTT(client, TOPIC2, payload2);
        printf("Data Sensor 2 dikirim ke MQTT: %s\n", payload2);

        delay(500); // jeda 500 ms sebelum pembacaan berikutnya
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return 0;
}
