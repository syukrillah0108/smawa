# SBC

## Instalasi

* Clang / GCC
  Compiler untuk code bahasa C atau C++
* WiringPi
  Library C/C++ untuk akses GPIO
* MQTT Broker
  Server untuk komunikasi data
* NodeJs
  Untuk menjalankan Node-red
* Node-red
  Kebutuhan Sofware berbasis website

## Code

* Blink LED
  Code bahasasa C++ yang memberikan indikator Blink LED sebagai penanda Sistem siap di gunakan
* Monitoring Topic MQTT
  Monitoring beberapa Topic MQTT sebagai indikasi Client MQTT apakah dalam keadaan Online (terhubung) atau Offline (terputus)

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

### Relay

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

# DEV2
