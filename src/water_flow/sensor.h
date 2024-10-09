volatile int pulseCount = 0;
float flowRate;
unsigned long duration;
float totalLiters;

void IRAM_ATTR countPulse() {
    pulseCount++;
}

void setup_sensor() {
    pinMode(D2, INPUT);
    attachInterrupt(
        digitalPinToInterrupt(D2),
        countPulse, RISING
    );
}

void get_data(){
    pulseCount = 0;
    duration = 10000;
    delay(duration);

    flowRate = (pulseCount / 7.5);
    totalLiters += (flowRate * (duration / 60000.0));
}