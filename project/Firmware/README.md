# SBC (Bananapi M1+)

* Allwinner A20 Dual-core 1.0GHz CPU
* Mali-400 MP2 with Open GL ES 2.0/1.1.
* 1 GB DDR3 memory.
* Wifi support onboard
* OS: Linux Armbian 2020
* Storage: 8 GB

## Instalasi

* Clang / GCC
  Compiler untuk kode bahasa C atau C++
* WiringPi
  Library C/C++ untuk akses GPIO
* Mosquitto
  MQTT Broker untuk komunikasi data
* NodeJs
  Untuk menjalankan Node-red
* Node-red
  Kebutuhan Sofware berbasis website

[![Instalasi](https://img.shields.io/badge/Setup-SBC-blue)](https://github.com/syukrillah0108/smawa/blob/main/project/Firmware/instalasi.md)

## Kode

* Blink LED
  Kode bahasasa C++ yang memberikan indikator Blink LED sebagai penanda Sistem siap di gunakan
* Monitoring Topic MQTT
  Monitoring beberapa Topic MQTT sebagai indikasi Client MQTT apakah dalam keadaan Online (terhubung) atau Offline (terputus)
* Monitoring Sistem

  [![Kode](https://img.shields.io/badge/kode-program-blue)](https://github.com/syukrillah0108/smawa/blob/main/src/program/monitoring.cpp)

## Konfigurasi

### Set-up IP

Melakukan konfigurasi IP di bagian Acess Point agar SBC mendapatkan IP Statis (IP Tetap)

### Credential

* Membuat akun untuk MQTT Broker
* Setting SSID Acess Point dan Password nya

### Service

Menjalankan apikasi/tools/file secara otomatis ketika Boot

* Blink LED
* MQTT Broker
* Node-red
* Monitoring Topic MQTT

# DEV1

Device wemos untuk monitoring air, kendali isi air dan tekanan air

## Library

```cpp
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
```

## Topic

```cpp
const char* tank_utama = "/air/utama";
const char* tank_nutrisi = "/air/nutrisi";
const char* relay1_topic = "/relay/1";
const char* relay2_topic = "/relay/2";
```

## Variabel

```cpp
const int trigPin1 = D5;
const int echoPin1 = D6;
const int trigPin2 = D7;
const int echoPin2 = D8;
const int relay1Pin = D1; #Relay isi Air
const int relay2Pin = D2; #Relay Tekanan Air
#define LED_BUILTIN D3

#define MAX_HEIGHT_NUTRISI 70.0  // Ketinggian maksimum tanki air nutrisi (cm)
#define MAX_HEIGHT_UTAMA 80.0  // Ketinggian maksimum tanki air utama (cm)
```

## Config

* WiFi
* MQTT Client
* Pin Ultrasonic

  Pin1: Air Utama
  Pin2: Air Nutrisi

```cpp
pinMode(trigPin1, OUTPUT);
pinMode(echoPin1, INPUT);
pinMode(trigPin2, OUTPUT);
pinMode(echoPin2, INPUT);
```

* Pin Relay

```cpp
pinMode(relay1Pin, OUTPUT);
pinMode(relay2Pin, OUTPUT);
```

## Logika

### Relay (Subscribe)

```cpp
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
```

### Publish

```cpp
client.publish(tank_utama, msg1);
client.publish(tank_nutrisi, msg2);
```

# DEV2

Device wemos untuk penyirmaran

## Library

```cpp
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
```

## Topic

```cpp
#define TOPIC_PUBLISH_FLOW "/siram/kecepatan"
#define TOPIC_SUBSCRIBE_SET "/siram/set"
#define TOPIC_SUBSCRIBE_OFF "/siram/off"
#define TOPIC_PUBLISH_TOTAL "/siram/total"
#define TOPIC_PUBLISH_TARGET "/siram/target"
#define TOPIC_PUBLISH_FINISH "/siram/finish"
#define TOPIC_PUBLISH_STATUS "/siram/status"
```

## Variabel

```cpp
#define FLOW_SENSOR_PIN D1
#define RELAY_PIN D2
#define LED_BUILTIN D3
bool NOTIF = false; 

volatile unsigned int pulseCount = 0;
float flowRate = 0.0;
float totalLiters = 0.0;
float targetLiters = 0.0;
```

## Fungsi

```cpp
// Menghitung jumlah Pull Up atau HIGH pada sensor
void ICACHE_RAM_ATTR pulseCounter() {
    pulseCount++;
}
```

## Config

* WiFi
* MQTT Client
* Pin

```cpp
pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
pinMode(RELAY_PIN, OUTPUT);
digitalWrite(RELAY_PIN, LOW);pinMode
```

* Hitung Pull Up

```cpp
attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);
```

## Logika

### Kirim data dari sensor

```cpp
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
```

### Penyiraman

```cpp
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
```
