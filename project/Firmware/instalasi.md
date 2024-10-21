# OS

* unduh OS di https://www.armbian.com/banana-pi-plus/
* unduh Flasher OS https://etcher.balena.io/
  ![balena](image/balena.png)
* Pilih file OS dan SD Card
  ![pick](image/pick_setup.png)
* Flash
  ![pick](image/install.png)
* hidupkan dan tunggu proses boot
  ![pick](image/boot.png)
* buat password untuk Root
  ![pick](image/pwdroot.png)
* buat user baru
  ![pick](image/buatuser.png)
* cek wlan
  ![pick](image/cekwlan.png)
* Ganti Versi
  https://drive.google.com/drive/mobile/folders/1VpvVkYMqgmSnmfKXQSrEY2B6wRa-cggL?usp=share_link&sort=13&direction=a
* Hidupkan wlan0 jika mati
  ```bash
  sudo ifconfig wlan0 up
  ```
  scan jaringan tersedia
  ```bash
  sudo iwlist wlan0 scan
  ```
  Hubungkan dengan jaringan
  ```bash
  sudo nmcli dev wifi connect "nama jaringan" password "kata sandi"

  ```

# MQTT Broker

1. install mosquitto

   ```bash
   sudo apt update
   sudo apt install mosquitto mosquitto-clients
   ```
2. Atur agar berjalan otomatis ketika boot
    ```bash
    sudo systemctl enable mosquitto
   ```
   jalankan
   ```bash
   sudo systemctl start mosquitto
   ```
3. Buat user baru
   ```bash
   sudo mosquitto_passwd -c /etc/mosquitto/passwd nama_user
   ```
   konfigurasi pengguna
   ```bash
    sudo nano /etc/mosquitto/mosquitto.conf
   ```
   Tambahakan baris berikut
   ```bash
    allow_anonymous false
    password_file /etc/mosquitto/passwd
   ```
4. Restart mosquitto
   ```bash
   sudo systemctl restart mosquitto
   ```

# Node-Red
1. Install
   ```bash
   bash <(curl -sL https://raw.githubusercontent.com/node-red/linux-installers/master/deb/update-nodejs-and-nodered)
   ```
   ![install](image/installnode.png)
2. Setup agara berjalan pas Boot
   ```bash
   sudo systemctl enable nodered.service
   ```
3. Test
   ![install](image/testnod.png)

# Python3.13
1. Tambah Reposiroty
   ```bash
    sudo add-apt-repository ppa:deadsnakes/ppa
    sudo apt update
   ```
2. Install python3.13
   ```bash
   sudo apt install python3.13 python3.13-dev
   ```
3. Install pip
   ```bash
   curl -sS https://bootstrap.pypa.io/get-pip.py | python3.13
   ```
4. Install pyA20
    ```bash
   pip3.13 install pyA20
   ```
5. Test Python
   ```python
   from time import sleep
   from pyA20.gpio import gpio
   from pyA20.gpio import port

   gpio.init()

   gpio.setcfg(port.PB13, gpio.OUTPUT)

   try:
      while True:
         gpio.output(port.PB13, gpio.HIGH)
         sleep(1)

         gpio.output(port.PB13, gpio.LOW)
         sleep(1)

   except KeyboardInterrupt:
      gpio.output(port.PB13, gpio.LOW)
   ```
   Jalankan program di Boot
   - Buat file service
      ```bash
      sudo nano /etc/systemd/system/blink.service
      ```
   - Konfigurasi
      ```bash
      [Unit]
      Description=Run Blink Script

      [Service]
      ExecStart=/usr/bin/python3.13 /program/blink.py
      WorkingDirectory=/program
      StandardOutput=inherit
      StandardError=inherit
      Restart=always
      User=root

      [Install]
      WantedBy=multi-user.target
      ```
   - Aktifkan Service
      ```bash
      sudo systemctl enable blink.service
      ```
   - Test
      ```bash
      sudo systemctl start blink.service
      ```

# Acess Point
