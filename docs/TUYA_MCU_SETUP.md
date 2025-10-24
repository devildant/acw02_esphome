# 🌀 ESPHome Configuration for Airton AC with Wi-Fi Module 409945

This guide applies **only to Airton A+++ AC units using the Wi-Fi module 409945**.  
These models use the **Tuya MCU UART protocol** (`55 AA 03 ...`) instead of the custom `7A 7A ...` protocol implemented in this repository.  

## 🧩 Hardware Compatibility

All **hardware parts provided in this repository** — including the **PCB**, **wiring**, and **3D printed enclosure** — remain **fully compatible** with this configuration.  
Only the firmware and communication protocol differ.  

## 📚 More Information

For more details about the Tuya MCU integration in ESPHome, refer to the official documentation:  
👉 [ESPHome Tuya MCU Component](https://esphome.io/components/tuya/)

---

## 💬 Acknowledgments

Special thanks to **[@mrgnsrl](https://github.com/mrgnsrl)** 🙌  
for identifying all the correct **Tuya MCU datapoints (DPIDs)** and for providing the initial working **ESPHome YAML configuration** that made this compatibility possible.  
Your contribution greatly helps other Airton users integrate their AC units with ESPHome! 💡

---

## ⚠️ Known Limitation — Non-Reportable Datapoints

The Tuya MCU used in the Airton 409945 Wi-Fi module does **not** report all datapoints (DPs) back to the ESP module.  
Some DPs are **control-only (write-only)**, meaning they execute a command on the AC but **do not send any state feedback** —  
neither at startup, nor during periodic status updates, nor after remote control changes.

### 🧾 Verified Observation
Based on actual Tuya MCU status dumps, the unit reports the following datapoints:  
`1–5, 8, 20–22, 101–115`  
**DP13 (Display)** and a few others are **absent**, confirming that they are not part of the MCU’s reportable list.

### 🧩 Datapoint Behavior Summary
| Entity | Datapoint | Type | Reported by MCU | Behavior |
|:--|:--:|:--:|:--:|:--|
| **Power** | `1` | switch | ✅ Yes | Fully functional |
| **Target Temp.** | `2` | int | ✅ Yes | Fully functional |
| **Ambient Temp.** | `3` | int | ✅ Yes | Read-only |
| **Mode** | `4` | enum | ✅ Yes | Fully functional |
| **Fan Speed** | `5` | enum | ✅ Yes | Fully functional |
| **Display / Affichage** | `13` | switch | ❌ No | Write-only — never reported |
| **Horizontal Swing** | `106` | enum | ✅ Yes | Reported correctly |
| **Vertical Swing** | `107` | enum | ✅ Yes | Reported correctly |
| **Night / Sleep** | `109` | switch | ✅ Yes | Usually reported, but may lag |
| **Purifier / Health** | `110` | switch | ✅ Yes | Reported correctly |

### 🔍 Technical Explanation
In the Tuya MCU protocol (`55 AA 03 ...`), each datapoint is classified internally as:
- `R` — Readable (reported periodically to Wi-Fi)
- `W` — Writable (can be controlled from Wi-Fi)
- `R/W` — Read & Write
- `Write-only` — Executes an action locally, without feedback

The **Display switch (DP13)** is defined as **write-only**,  
and therefore it never appears in any Tuya MCU status message — confirmed by real UART logs.

### 💡 Recommendations
- Treat these “write-only” datapoints (like Display) as **unidirectional controls**.  
  ESPHome will keep the **last known state**, which might differ from the real device state if changed via remote.
- For such entities, consider using `optimistic: true` or `restore_value: yes` in your YAML configuration.

---

## ❗ Notes

This firmware version does not use **MQTT**, but if you want you can add it yourself in the yaml

---

## 🧠 Example ESPHome Configuration

Edit the `substitutions` section (in yaml bellow) — you can modify:

- `dev_name`
- `dev_friendly`
- `api_encrypted_key`
- `ota_password`
- `ap_password`
- `web_server_username`
- `web_server_password`

```yaml
substitutions:
  dev_name: acw02-salon
  dev_friendly: ACW02 salon
  api_encrypted_key: "D2oldc0VP++fni6src89tCSC0UwBhNPgyc8vgYN8/mA="
  ota_password: "REPLACE_WITH_YOUR_PASSWORD"
  ap_password: "fallbackpassword"
  web_server_username: admin
  web_server_password: admin
  # ESP32 D1 mini WROOM 32
  board: esp32dev
  TX: GPIO17
  RX: GPIO16
  
  ## LOLIN ESP32 C3 MINI
  # board: lolin_c3_mini
  # TX: GPIO21
  # RX: GPIO20

  ## XIAO (seeeds studio) ESP32 C3
  # board: seeed_xiao_esp32c3
  # TX: GPIO21
  # RX: GPIO20

  ## XIAO (seeeds studio) ESP32 C6
  # board: seeed_xiao_esp32c6
  # TX: GPIO16
  # RX: GPIO17
```

⚠️ **The configuration depends on the ESP32 card chosen (Comment/uncomment the blocks)**  
 - **ESP32 D1 mini WROOM 32**
    ```
    # ESP32 D1 mini WROOM 32
    board: esp32dev
    TX: GPIO17
    RX: GPIO16
    ```
 - **LOLIN ESP32 C3 MINI**
    ```
    # LOLIN ESP32 C3 MINI
    board: lolin_c3_mini
    TX: GPIO21
    RX: GPIO20
    ```
 - **XIAO (seeeds studio) ESP32 C3**
    ```
    # XIAO (seeeds studio) ESP32 C3
    board: seeed_xiao_esp32c3
    TX: GPIO21
    RX: GPIO20
    ```
 - **XIAO (seeeds studio) ESP32 C6**
    ```
    # XIAO (seeeds studio) ESP32 C6
    board: seeed_xiao_esp32c6
    TX: GPIO16
    RX: GPIO17
    ```

🔑 I recommend visiting [this page](https://esphome.io/components/api.html) to generate a unique `api_encrypted_key`.

### FR Example Configuration (French version)

Below is a configuration in French adapted for the 409945 Tuya MCU module.  
It includes network fallback, web interface authentication, and hardware flexibility for various ESP32 boards.

```yaml
substitutions:
  dev_name: acw02-salon
  dev_friendly: ACW02 salon
  api_encrypted_key: "D2oldc0VP++fni6src89tCSC0UwBhNPgyc8vgYN8/mA="
  ota_password: "REPLACE_WITH_YOUR_PASSWORD"
  ap_password: "fallbackpassword"
  web_server_username: admin
  web_server_password: admin
  # ESP32 D1 mini WROOM 32
  board: esp32dev
  TX: GPIO17
  RX: GPIO16

  ## LOLIN ESP32 C3 MINI
  # board: lolin_c3_mini
  # TX: GPIO21
  # RX: GPIO20

  ## XIAO (seeeds studio) ESP32 C3
  # board: seeed_xiao_esp32c3
  # TX: GPIO21
  # RX: GPIO20

  ## XIAO (seeeds studio) ESP32 C6
  # board: seeed_xiao_esp32c6
  # TX: GPIO16
  # RX: GPIO17


esphome:
  name: ${dev_name}
  friendly_name: ${dev_friendly}

esp32:
  board: ${board}
  framework:
    type: esp-idf

web_server:
  port: 80
  auth:
    username: ${web_server_username}
    password: ${web_server_password}
  version: 3
  log: true
  local : true
  include_internal: true

logger:
  level: INFO

api:
  encryption:
    key: ${api_encrypted_key}

ota:
  - platform: esphome
    password: ${ota_password}
  - platform: web_server

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  power_save_mode: none
  # networks:
  #   - ssid: !secret wifi_ssid
  #     password: !secret wifi_password
  #   - ssid: !secret wifi_ssid2
  #     password: !secret wifi_password2
  #   - ssid: !secret wifi_ssid3
  #     password: !secret wifi_password3
  ap:
    ssid: ${dev_friendly}
    password: ${ap_password}

captive_portal:
	
uart:
  id: uart_acw02
  tx_pin: ${TX}
  rx_pin: ${RX}
  baud_rate: 9600

tuya:
climate:
  - platform: tuya
    id: acw02
    name: "Climatiseur"
    switch_datapoint: 1
    supports_heat: true
    supports_cool: true
    target_temperature_datapoint: 2
    current_temperature_datapoint: 3
    temperature_multiplier: 0.1
    on_state:
      - lambda: |-
          if (x.target_temperature < 10) {
            id(acw02).target_temperature = x.target_temperature * 10;
          }
    visual:
      min_temperature: 16
      max_temperature: 31
      temperature_step: 1
    active_state:
      datapoint: 4
      heating_value: 3
      cooling_value: 1
      drying_value: 2
      fanonly_value: 4
select:
  - platform: tuya
    name: "Ventilation"
    enum_datapoint: 5
    options:
      0: "Auto"
      1: "20%"
      2: "40%"
      3: "60%"
      4: "80%"
      5: "100%"
      6: "Silencieux"
      7: "Turbo"
  - platform: "tuya"
    name: "Oscillation Vertical"
    enum_datapoint: 107
    options:
      0: "Désactivé"
      1: "Activé"
      2: "Position 1"
      3: "Position 2"
      4: "Position 3"
      5: "Position 4"
      6: "Position 5"  
  - platform: "tuya"
    name: "Unité"
    enum_datapoint: 105
    options:
      0: "°C"
      1: "°F"
  - platform: "tuya"
    name: "Oscillation Horizontal"
    enum_datapoint: 106
    options:
      0: "Désactivé"
      1: "Activé"

switch:
  - platform: "tuya"
    name: "Affichage"
    switch_datapoint: 13
  - platform: "tuya"
    name: "Purificateur"
    switch_datapoint: 110
  - platform: "tuya"
    name: "Nuit"
    switch_datapoint: 109

```

### EN Example Configuration (English version)

Below is a configuration in English adapted for the 409945 Tuya MCU module.  
It includes network fallback, web interface authentication, and hardware flexibility for various ESP32 boards.

```yaml
substitutions:
  dev_name: acw02-salon
  dev_friendly: ACW02 salon
  api_encrypted_key: "D2oldc0VP++fni6src89tCSC0UwBhNPgyc8vgYN8/mA="
  ota_password: "REPLACE_WITH_YOUR_PASSWORD"
  ap_password: "fallbackpassword"
  web_server_username: admin
  web_server_password: admin
  # ESP32 D1 mini WROOM 32
  board: esp32dev
  TX: GPIO17
  RX: GPIO16

  ## LOLIN ESP32 C3 MINI
  # board: lolin_c3_mini
  # TX: GPIO21
  # RX: GPIO20

  ## XIAO (seeeds studio) ESP32 C3
  # board: seeed_xiao_esp32c3
  # TX: GPIO21
  # RX: GPIO20

  ## XIAO (seeeds studio) ESP32 C6
  # board: seeed_xiao_esp32c6
  # TX: GPIO16
  # RX: GPIO17


esphome:
  name: ${dev_name}
  friendly_name: ${dev_friendly}

esp32:
  board: ${board}
  framework:
    type: esp-idf

web_server:
  port: 80
  auth:
    username: ${web_server_username}
    password: ${web_server_password}
  version: 3
  log: true
  local : true
  include_internal: true

logger:
  level: INFO

api:
  encryption:
    key: ${api_encrypted_key}

ota:
  - platform: esphome
    password: ${ota_password}
  - platform: web_server

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  power_save_mode: none
  # networks:
  #   - ssid: !secret wifi_ssid
  #     password: !secret wifi_password
  #   - ssid: !secret wifi_ssid2
  #     password: !secret wifi_password2
  #   - ssid: !secret wifi_ssid3
  #     password: !secret wifi_password3
  ap:
    ssid: ${dev_friendly}
    password: ${ap_password}

captive_portal:
	
uart:
  id: uart_acw02
  tx_pin: ${TX}
  rx_pin: ${RX}
  baud_rate: 9600

tuya:
climate:
  - platform: tuya
    id: acw02
    name: "AC"
    switch_datapoint: 1
    supports_heat: true
    supports_cool: true
    target_temperature_datapoint: 2
    current_temperature_datapoint: 3
    temperature_multiplier: 0.1
    on_state:
      - lambda: |-
          if (x.target_temperature < 10) {
            id(acw02).target_temperature = x.target_temperature * 10;
          }
    visual:
      min_temperature: 16
      max_temperature: 31
      temperature_step: 1
    active_state:
      datapoint: 4
      heating_value: 3
      cooling_value: 1
      drying_value: 2
      fanonly_value: 4
select:
  - platform: tuya
    name: "Fan"
    enum_datapoint: 5
    options:
      0: "Auto"
      1: "20%"
      2: "40%"
      3: "60%"
      4: "80%"
      5: "100%"
      6: "Silent"
      7: "Turbo"
  - platform: "tuya"
    name: "Vertical swing"
    enum_datapoint: 107
    options:
      0: "Off"
      1: "On"
      2: "Position 1"
      3: "Position 2"
      4: "Position 3"
      5: "Position 4"
      6: "Position 5"  
  - platform: "tuya"
    name: "Unit"
    enum_datapoint: 105
    options:
      0: "°C"
      1: "°F"
  - platform: "tuya"
    name: "Horizontal swing"
    enum_datapoint: 106
    options:
      0: "Off"
      1: "On"

switch:
  - platform: "tuya"
    name: "Display"
    switch_datapoint: 13
  - platform: "tuya"
    name: "Purifier"
    switch_datapoint: 110
  - platform: "tuya"
    name: "Night"
    switch_datapoint: 109

```