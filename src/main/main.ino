#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "web.html"

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

#define TRIG_PIN 5
#define ECHO_PIN 18
long duration;
int waterLevelPercentage;

#define FLOW_SENSOR_PIN 4
volatile int pulseCount;
float flowRate;
float totalLiters = 0;
unsigned long lastFlowTime;

WebServer server(80);

TaskHandle_t TaskWaterLevel;
TaskHandle_t TaskFlowSensor;

void WaterLevelTask(void * parameter) {
  while (1) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    duration = pulseIn(ECHO_PIN, HIGH);
    int distance = duration * 0.034 / 2; 
    waterLevelPercentage = 100 - (distance * 100 / 100);  
    
    if (waterLevelPercentage > 100) waterLevelPercentage = 100;
    if (waterLevelPercentage < 0) waterLevelPercentage = 0;
    
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
}

void FlowSensorTask(void * parameter) {
  while (1) {
    if ((millis() - lastFlowTime) > 1000) {
      flowRate = (pulseCount / 7.5);
      totalLiters += (flowRate / 60.0);
      pulseCount = 0; 
      lastFlowTime = millis();
    }
    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
}

void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

String getData() {
  StaticJsonDocument<200> jsonBuffer;
  jsonBuffer["speed"] = int(flowRate);
  jsonBuffer["waterPercentage"] = int(waterLevelPercentage);
  jsonBuffer["totalLiters"] = int(totalLiters);

  String jsonString;
  serializeJson(jsonBuffer, jsonString);
  return jsonString;
}

void handleMode(String mode) {
  if (mode == "on") {

  } else if (mode == "auto") {

  } else if (mode == "off") {

  }
}

void handleSiram(int liters) {
  totalLiters += liters;
}

void handleRoot(){
 server.send(200, "text/html", webpageCode);
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);

  WiFi.softAP("ESP32_Hotspot", "your_password");

  WiFi.softAPConfig(local_ip, gateway, subnet);

  Serial.println("Access Point started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  xTaskCreatePinnedToCore(
    WaterLevelTask,
    "WaterLevelTask", 
    10000,
    NULL,             
    1,                
    &TaskWaterLevel,  
    1);               

  xTaskCreatePinnedToCore(
    FlowSensorTask,   
    "FlowSensorTask", 
    10000,            
    NULL,             
    1,                
    &TaskFlowSensor,  
    1);               

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

  server.begin();
}

void loop() {
  server.handleClient();
}
