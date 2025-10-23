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

## ⚠️ Known Limitation — Non-Reportable Switch Datapoints

Some Tuya MCU datapoints used in this configuration behave as **“control-only” (write-only)** commands.  
This means they execute a local action on the AC but **do not send back their state** to the Wi-Fi module —  
neither during startup, nor when changed via the IR remote or front panel.

### 🧩 Affected Switches
| Entity | Datapoint | Behavior |
|:--|:--:|:--|
| **Display / Affichage** | `DP 13` | Write-only — never reported back |
| **Purifier / Santé** | `DP 110` | Usually write-only — may not report |
| **Night / Sleep** | `DP 109` | Often write-only — not always reported |
| **Horizontal Swing / Oscillation horizontale** | `DP 106` | May or may not report depending on MCU |

### 🔍 Technical Explanation
In the Tuya MCU serial protocol (`55 AA 03 ...`), each datapoint is defined internally by the AC firmware as:
- `R` – Readable (reported to the Wi-Fi module)
- `W` – Writable (can be controlled from Wi-Fi)
- `R/W` – Both readable and writable
- `Write-only` – Executes a command locally, no status feedback

The **Display switch (DP13)** and similar control DPs are usually marked as *write-only* by the MCU manufacturer.  
Therefore:
- The Wi-Fi module **can send the command**, and the action is applied.  
- The AC **does not send any confirmation** or status change in return.  
- ESPHome will **show the last known state**, which may differ if changed by remote.

### 💡 Recommendation
For these datapoints, you can:
- Use `optimistic: true` or `restore_value: yes` in ESPHome to remember the last state locally.  
- Treat them as **“unidirectional controls”** rather than stateful switches.

*(Based on observations, Tuya MCU documentation, and community testing — not officially documented by Tuya for all models.)*

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
  - platform: "tuya"
    name: "Oscillation Horizontal"
    switch_datapoint: 106

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
  - platform: "tuya"
    name: "Horizontal swing"
    switch_datapoint: 106

```