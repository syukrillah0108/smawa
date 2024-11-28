#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <MQTTClient.h>

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "BananaPiSystemMonitor"
#define USERNAME    "user1"
#define PASSWORD    "1234567890"
#define TOPIC       "/system/monitoring"
#define QOS         2
#define TIMEOUT     10000L

using namespace std;

// Fungsi untuk mendapatkan informasi dari file
string getFileContent(const string& path) {
    ifstream file(path);
    string content;
    if (file.is_open()) {
        getline(file, content);
        file.close();
    }
    return content;
}

// Fungsi untuk mendapatkan penggunaan CPU dalam format integer (persentase)
int getCPUUsage() {
    string line, cpu;
    long user, nice, system, idle;
    
    ifstream file("/proc/stat");
    if (file.is_open()) {
        getline(file, line);
        istringstream iss(line);
        iss >> cpu >> user >> nice >> system >> idle;
        file.close();
        
        long total = user + nice + system + idle;
        long used = user + nice + system;
        return static_cast<int>((used * 100) / total);
    }
    return -1;
}

// Fungsi untuk mendapatkan penggunaan RAM dalam format integer (dalam MB)
int getRAMUsage() {
    string line;
    long total, available;
    
    ifstream file("/proc/meminfo");
    if (file.is_open()) {
        while (getline(file, line)) {
            if (line.find("MemTotal:") == 0) {
                sscanf(line.c_str(), "MemTotal: %ld kB", &total);
            } else if (line.find("MemAvailable:") == 0) {
                sscanf(line.c_str(), "MemAvailable: %ld kB", &available);
                break;
            }
        }
        file.close();
        
        long used = total - available;
        return static_cast<int>(used / 1024); // Konversi ke MB
    }
    return -1;
}

// Fungsi untuk mendapatkan penggunaan Disk dalam format integer (persentase)
int getDiskUsage() {
    string result;
    FILE* pipe = popen("df / --output=pcent | tail -1 | tr -dc '0-9'", "r");
    if (pipe) {
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
    }
    return result.empty() ? -1 : stoi(result);
}

// Fungsi untuk mendapatkan suhu CPU dalam format integer (derajat Celsius)
int getCPUTemperature() {
    string tempStr = getFileContent("/sys/class/thermal/thermal_zone0/temp");
    if (!tempStr.empty()) {
        return stoi(tempStr) / 1000; // Konversi dari m°C ke °C
    }
    return -1;
}

// Fungsi untuk mengirim data ke broker MQTT
void sendToMQTT(int cpuUsage, int ramUsage, int diskUsage, int cpuTemp) {
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
        return;
    }

    char payload[100];
    snprintf(
        payload, sizeof(payload), 
        "{\"cpu\": %d, \"ram\": %d, \"disk\": %d, \"temp\": %d}", 
        cpuUsage, ramUsage, diskUsage, cpuTemp
    );

    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    MQTTClient_waitForCompletion(client, token, TIMEOUT);

    printf("Data dikirim ke MQTT: %s\n", payload);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}

int main() {
    while (true) {
        // Monitoring sistem Banana Pi
        int cpuUsage = getCPUUsage();
        int ramUsage = getRAMUsage();
        int diskUsage = getDiskUsage();
        int cpuTemp = getCPUTemperature();

        // Kirim data ke MQTT
        sendToMQTT(cpuUsage, ramUsage, diskUsage, cpuTemp);

        // Tunggu 5 detik sebelum update berikutnya
        sleep(5);
    }
    return 0;
}
