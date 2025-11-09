# 🚀 Installation on macOS

## 🖥️ Tested Environment
- Mac Mini 2018
- 3 GHz Intel Core i5 (6 cores)
- 8 GB 2667 MHz DDR4
- macOS 15.7 (24G222)

## 📋 Prerequisites
1. Clone the repository:
   ```bash
   git clone https://github.com/devildant/acw02_esphome.git
   ```
2. Copy the YAML file for your preferred language, for example:
   ```bash
   cp esphome-acw02-fr.yaml my-esphome-acw02.yaml
   ```
   > ℹ️ The file can be `esphome-acw02-fr.yaml` or `esphome-acw02-en.yaml` depending on the language you want.
3. Configure `my-esphome-acw02.yaml` and `secrets.yaml`.

---

# ⚡ ESPHome: Installation, Build, and Flash

## 1️⃣ ESPHome Installation
Install a specific ESPHome version via Homebrew:

```bash
brew install esphome@2025.10.4
esphome version  # should return 2025.10.4
```

---

## 2️⃣ Build and Run
```bash
esphome run my-esphome-acw02.yaml
```

---

## 3️⃣ Choose Upload Method
After compilation, ESPHome will show multiple upload options:

```text
INFO Package configuration completed successfully
INFO Package configuration completed successfully
INFO Successfully compiled program.
Found multiple options for uploading, please choose one:
  [1] /dev/cu.usbserial-0001 (CP2102 USB to UART Bridge Controller - CP2102 USB to UART Bridge Controller)
  [2] Over The Air (acw02-entree.local)
  [3] Over The Air (MQTT IP lookup)
(number): 1
```

- Select the number that fits your situation.

---

## 4️⃣ Startup and Wi-Fi Connection
Be patient during the first connection:

```text
esptool v5.0.2
Connected to ESP32 on /dev/cu.usbserial-0001:
Chip type:          ESP32-D0WD-V3 (revision v3.1)
Features:           Wi-Fi, BT, Dual Core + LP Core, 240MHz, Vref calibration in eFuse, Coding Scheme None
Crystal frequency:  40MHz
MAC:                5c:01:3b:4c:52:50

Stub flasher running.
Changing baud rate to 460800...
Changed.

Configuring flash size...

[...]

[22:50:22.430][W][component:296]: wifi set Warning flag: scanning for networks
[22:50:22.435][W][acw02:2996]: TX: [7A.7A.21.D5.0C.00.00.AB.0A.0A.FC.F9 (12)]
[22:50:22.861][W][wifi_esp32:1001]: esp_wifi_sta_get_ap_info failed: ESP_ERR_WIFI_NOT_CONNECT
[22:50:22.869][W][wifi_esp32:991]: esp_wifi_sta_get_ap_info failed: ESP_ERR_WIFI_NOT_CONNECT
[22:50:22.876][W][wifi_esp32:1012]: esp_wifi_sta_get_ap_info failed: ESP_ERR_WIFI_NOT_CONNECT
[22:50:24.294][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:50:25.023][W][wifi:612]: No matching network found
[22:50:28.297][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:50:32.300][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:50:32.634][W][wifi:612]: No matching network found
[22:50:36.303][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:50:40.245][W][wifi:612]: No matching network found
[22:50:40.307][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:50:44.311][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:50:47.856][W][wifi:612]: No matching network found
[22:50:48.313][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:50:52.317][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:50:55.467][W][wifi:612]: No matching network found
[22:50:56.319][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:51:00.323][W][acw02:872]: Wi-Fi not connected; retrying in 5 s
[22:51:00.480][I][wifi:329]: Connecting to 'Fra...'
[22:51:04.260][I][wifi:675]: Connected
[22:51:04.264][W][wifi:678]: Network 'Fra...' should be marked as hidden
[22:51:04.282][I][app:120]: setup() finished successfully!
[22:51:04.288][W][component:326]: wifi cleared Warning flag
[22:51:04.293][I][app:185]: ESPHome version 2025.9.3 compiled on Oct  2 2025, 22:47:45
[22:51:04.330][W][acw02:863]: Wi-Fi OK; attempting MQTT connection…
[22:51:04.335][W][component:287]: mqtt set Warning flag: unspecified
[22:51:04.336][I][mqtt:268]: Connecting
[22:51:04.382][W][acw02:881]: MQTT connected → publishing discovery and state
[22:51:04.388][W][component:326]: mqtt cleared Warning flag
[22:51:04.389][I][mqtt:309]: Connected
```


