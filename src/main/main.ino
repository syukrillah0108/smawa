#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "web.html"

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Ultrasonic Sensor
#define TRIG_PIN 5
#define ECHO_PIN 18
long duration;
int waterLevelPercentage;

// Flow Sensor
#define FLOW_SENSOR_PIN 4
volatile int pulseCount;
float flowRate;
float totalLiters = 0;
unsigned long lastFlowTime;

// HTTP Server
WebServer server(80);

TaskHandle_t TaskWaterLevel;
TaskHandle_t TaskFlowSensor;

// RTOS Task untuk Mengukur Ketinggian Air
void WaterLevelTask(void * parameter) {
  while (1) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    duration = pulseIn(ECHO_PIN, HIGH);
    int distance = duration * 0.034 / 2;  // Convert to cm
    waterLevelPercentage = 100 - (distance * 100 / 100);  // Assuming tank height is 100 cm
    
    // Batasan persentase dari 0% sampai 100%
    if (waterLevelPercentage > 100) waterLevelPercentage = 100;
    if (waterLevelPercentage < 0) waterLevelPercentage = 0;
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Delay 2 detik
  }
}

// RTOS Task untuk Sensor Flow
void FlowSensorTask(void * parameter) {
  while (1) {
    if ((millis() - lastFlowTime) > 1000) {
      flowRate = (pulseCount / 7.5);  // Konversi ke L/min (7.5 pulse per liter)
      totalLiters += (flowRate / 60.0);  // Akumulasi jumlah liter
      pulseCount = 0;  // Reset pulse count setiap 1 detik
      lastFlowTime = millis();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Delay 1 detik
  }
}

// ISR untuk Water Flow Sensor
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

// Mengirim Data JSON ke Webserver
String getData() {
  StaticJsonDocument<200> jsonBuffer;
  jsonBuffer["speed"] = int(flowRate);
  jsonBuffer["waterPercentage"] = int(waterLevelPercentage);
  jsonBuffer["totalLiters"] = int(totalLiters);

  String jsonString;
  serializeJson(jsonBuffer, jsonString);
  return jsonString;
}

// Mengatur Mode
void handleMode(String mode) {
  if (mode == "on") {

  } else if (mode == "auto") {

  } else if (mode == "off") {

  }
}

// Siram Manual
void handleSiram(int liters) {
  totalLiters += liters;  // Tambahkan jumlah liter yang disiram
}

void handleRoot(){
 server.send(200, "text/html", webpageCode);
}

// Setup ESP32 dan Server
void setup() {
  // Inisialisasi Serial
  Serial.begin(115200);
  
  // Inisialisasi Pin Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Inisialisasi Pin Flow Sensor
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);
  
  // Set up ESP32 as an Access Point (AP)
  WiFi.softAP("ESP32_Hotspot", "your_password");

  // Set static IP for the ESP32 AP
  WiFi.softAPConfig(local_ip, gateway, subnet);

  // Informasi untuk debugging
  Serial.println("Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // RTOS Task untuk Mengukur Ketinggian Air
  xTaskCreatePinnedToCore(
    WaterLevelTask,   /* Function to implement the task */
    "WaterLevelTask", /* Name of the task */
    10000,            /* Stack size in words */
    NULL,             /* Task input parameter */
    1,                /* Priority of the task */
    &TaskWaterLevel,  /* Task handle. */
    1);               /* Core where the task should run */

  // RTOS Task untuk Flow Sensor
  xTaskCreatePinnedToCore(
    FlowSensorTask,   /* Function to implement the task */
    "FlowSensorTask", /* Name of the task */
    10000,            /* Stack size in words */
    NULL,             /* Task input parameter */
    1,                /* Priority of the task */
    &TaskFlowSensor,  /* Task handle. */
    1);               /* Core where the task should run */

  // Konfigurasi Web Server
  server.on("/", handleRoot);
  server.on("/getdata", HTTP_GET, []() {
    server.send(200, "application/json", getData());
  });

  server.on("/mode", HTTP_POST, []() {
    if (server.hasArg("mode")) {
      String mode = server.arg("mode");
      handleMode(mode);
    }
    server.send(200, "text/plain", "OK");
  });

  server.on("/siram", HTTP_POST, []() {
    if (server.hasArg("liters")) {
      int liters = server.arg("liters").toInt();
      handleSiram(liters);
    }
    server.send(200, "text/plain", "OK");
  });

  // Jalankan server
  server.begin();
}

void loop() {
  // Jalankan web server
  server.handleClient();
}
