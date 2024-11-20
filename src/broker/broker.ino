#include "config.h"
#include "sensor.h"
#include "TeleBot.h"

#define btnPin 5

void check_alarm() {
    unsigned long currentMillis = millis(); // Mendapatkan waktu saat ini

    if ((Temp1 >= 70 || Temp3 >= 70)) {
        digitalWrite(RELAY1, HIGH);
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis; // Mengupdate waktu sebelumnya
            bot.sendMessage(CHATID, "❗PERINGATAN \n Suhu Nori: " + String(int(Temp3)) + " ℃");
            Serial.println("Pesan dikirim setiap 10 detik sekali!"); // Mengirim pesan ke Serial Monitor
        }
    } else{
        digitalWrite(RELAY1, LOW);
    }

}

void setup(){
    Serial.begin(115200);
    dht1.begin();
    dht3.begin();unsigned long bot_lasttime;

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(RELAY1, OUTPUT);
    pinMode(btnPin, INPUT);
    pinMode(btn, INPUT);

    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(RELAY1, LOW);

    WiFi.begin(ssid, password);
    delay(2000);

    setup_broker();
    setup_client();
    client_bot.setInsecure();

    xTaskCreatePinnedToCore(
        brokerUpdateTask,   // Task function
        "MQTTBrokerTask",   // Task name
        4096,               // Stack size (4 KB)
        NULL,               // Parameter (not used here)
        1,                  // Priority
        &MQTTBrokerTask,    // Task handle
        0                   // Core 0
    );


}
void loop(){
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    //read();

    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    /*
    client.publish(mqtt_topic_temp1, String(Temp1).c_str());
    client.publish(mqtt_topic_temp3, String(Temp3).c_str());
    client.publish(mqtt_topic_hum1, String(Hum1).c_str());
    client.publish(mqtt_topic_hum3, String(Hum3).c_str());
    */

    client.publish(mqtt_topic_temp1, "63");
    client.publish(mqtt_topic_temp3, "71");
    client.publish(mqtt_topic_hum1, "23");
    client.publish(mqtt_topic_hum3, "20");

    check_alarm();
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);

    if (WiFi.status() == WL_CONNECTED) {
        // Memeriksa update bot secara berkala
        if (millis() > lastTimeBotRan + botInterval) {
            int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
            while (numNewMessages) {
                handleNewMessages(numNewMessages);
                numNewMessages = bot.getUpdates(bot.last_message_received + 1);
            }
            lastTimeBotRan = millis();
        }
    }else{
        WiFi.begin(ssid, password);
    }
}