import time
from pyA20.gpio import gpio
from pyA20.gpio import port
import paho.mqtt.client as mqtt

ECHO_PIN = port.PI20
TRIG_PIN = port.PI21

TANGKI_TINGGI_CM = 100.0
persentase = float

gpio.init()
gpio.setcfg(ECHO_PIN, gpio.INPUT)
gpio.setcfg(TRIG_PIN, gpio.OUTPUT)

def measure_distance():
    gpio.output(TRIG_PIN, gpio.LOW)
    time.sleep(0.02)

    gpio.output(TRIG_PIN, gpio.HIGH)
    time.sleep(0.00001)
    gpio.output(TRIG_PIN, gpio.LOW)
    try:
        while gpio.input(ECHO_PIN) == gpio.LOW:
            start_time = time.time()

        while gpio.input(ECHO_PIN) == gpio.HIGH:
            end_time = time.time()

        duration = end_time - start_time
        distance = (duration * 34300) / 2
        return distance
    except UnboundLocalError:
        print("err pin")
        return TANGKI_TINGGI_CM

def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected to MQTT broker with result code {reason_code}")

mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqtt_client.username_pw_set("user1", "1234567890") 
mqtt_client.on_connect = on_connect
mqtt_client.connect("localhost", 1883, 60)

mqtt_client.loop_start()

try:
    while True:
        distance = measure_distance()
        print("Jarak: {:.2f} cm".format(distance))
        persentase = (TANGKI_TINGGI_CM-distance)
        persentase /= TANGKI_TINGGI_CM
        persentase *= 100
        mqtt_client.publish("/air/utama", persentase)
        time.sleep(0.5)

except KeyboardInterrupt:
    print("Pengukuran dihentikan")
finally:
    gpio.output(TRIG_PIN, gpio.LOW)
    gpio.cleanup()