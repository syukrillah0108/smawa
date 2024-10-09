volatile int pulseCount = 0; // Menghitung pulsa dari sensor
float flowRate; // Dalam L/min
unsigned long duration; // Durasi dalam milidetik
float totalLiters; // Total volume air yang mengalir

void IRAM_ATTR countPulse() {
    pulseCount++; // Tambah hitungan pulsa setiap kali interrupt terjadi
}

void setup() {
    Serial.begin(9600);
    pinMode(D2, INPUT); // Pin sensor sebagai input
    attachInterrupt(digitalPinToInterrupt(D2), countPulse, RISING); // Interrupt untuk menghitung pulsa
}

void loop() {
    pulseCount = 0; // Reset hitungan pulsa
    duration = 500; // Hitung selama 1 detik
    delay(duration); // Tunggu selama durasi

    // Hitung flow rate
    flowRate = (pulseCount / 7.5); // Ganti 7.5 sesuai dengan spesifikasi sensor (pulses per liter)
    totalLiters += (flowRate * (duration / 60000.0)); // Total volume dalam liter

    Serial.print("Flow Rate: ");
    Serial.print(flowRate);
    Serial.println(" L/min");
    
    Serial.print("Total Volume: ");
    Serial.print(totalLiters);
    Serial.println(" L");
}
