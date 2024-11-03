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
#define QOS         2
#define TIMEOUT     10000L

#define TRIG_PIN_1 27
#define ECHO_PIN_1 28
#define TRIG_PIN_2 25
#define ECHO_PIN_2 24

#define TANGKI_TINGGI_CM 100.0
float persentase;

double measureDistance(int trigPin, int echoPin) {
    long startTime, travelTime;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    while (digitalRead(echoPin) == LOW);
    startTime = micros();
    while (digitalRead(echoPin) == HIGH);

    travelTime = micros() - startTime;

    double distance = (travelTime * 0.0343) / 2;
    return distance;
}

double calculateWaterPercentage(double distance) {
    if (distance > TANGKI_TINGGI_CM)
        distance = TANGKI_TINGGI_CM;
    double heightOfWater = TANGKI_TINGGI_CM - distance;
    return (heightOfWater / TANGKI_TINGGI_CM) * 100;
}

int main(void) {
    if (wiringPiSetup() == -1) {
        printf("Gagal menginisialisasi WiringPi\n");
        return 1;
    }

    pinMode(TRIG_PIN_1, OUTPUT);
    pinMode(ECHO_PIN_1, INPUT);

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

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
        double rawDistance = measureDistance(TRIG_PIN_1, ECHO_PIN_1);
        printf("Nilai Mentah Jarak: %.2f\n", rawDistance);
        persentase = (TANGKI_TINGGI_CM-rawDistance);
        persentase /= TANGKI_TINGGI_CM;
        persentase *= 100;

        char payload[50];
        snprintf(payload, sizeof(payload), "%.2f", persentase);

        pubmsg.payload = payload;
        pubmsg.payloadlen = strlen(payload);
        pubmsg.qos = QOS;
        pubmsg.retained = 0;
        MQTTClient_publishMessage(client, TOPIC1, &pubmsg, &token);
        MQTTClient_waitForCompletion(client, token, TIMEOUT);

        printf("Data jarak mentah dikirim ke MQTT: %s\n", payload);

        delay(500);
    }
    
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return 0;
}
