# ACW02 ESPHome Module  

Custom ESPHome component for **Teknopoint** and **Airton** (Airton ref: 409729) AC units using the **Tuya ACW02 Wi-Fi module**.

---

## Weekly build status (auto build with last esphome version)

**ESP32 D1 Mini WROOM 32**  
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/build-d1-mini.json&style=flat-square&label=Build&labelColor=1f2328" height="24"> &nbsp;
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/last_passing-d1-mini.json&style=flat-square&label=Last%20passing&labelColor=1f2328" height="24"> &nbsp;
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/problem-d1-mini.json&style=flat-square&label=Problem%20version&labelColor=1f2328" height="24">

**Lolin ESP32 C3 mini**  
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/build-lolin-c3.json&style=flat-square&label=Build&labelColor=1f2328" height="24"> &nbsp;
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/last_passing-lolin-c3.json&style=flat-square&label=Last%20passing&labelColor=1f2328" height="24"> &nbsp;
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/problem-lolin-c3.json&style=flat-square&label=Problem%20version&labelColor=1f2328" height="24">

**XIAO ESP32 C3**  
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/build-xiao-c3.json&style=flat-square&label=Build&labelColor=1f2328" height="24"> &nbsp;
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/last_passing-xiao-c3.json&style=flat-square&label=Last%20passing&labelColor=1f2328" height="24"> &nbsp;
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/problem-xiao-c3.json&style=flat-square&label=Problem%20version&labelColor=1f2328" height="24">

**XIAO ESP32 C6**  
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/build-xiao-c6.json&style=flat-square&label=Build&labelColor=1f2328" height="24"> &nbsp;
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/last_passing-xiao-c6.json&style=flat-square&label=Last%20passing&labelColor=1f2328" height="24"> &nbsp;
<img src="https://img.shields.io/endpoint?url=https://raw.githubusercontent.com/devildant/acw02_esphome/main/.ci/badges/problem-xiao-c6.json&style=flat-square&label=Problem%20version&labelColor=1f2328" height="24">

---
## 📑 Table of Contents
- [⏪ ESPHome Versions Before 2026.7.0](#-esphome-versions-before-202670)
- [⚠️ Compatibility Warning](#️-compatibility-warning)
- [✅ Community Compatibility List](#-community-compatibility-list)
- [📌 Contribute](#-contribute)
- [📋 Requirements](#-requirements)
- [⚙️ Settings](#️-settings)
  - [📁 Base Configuration](#-base-configuration)
  - [📶 Wi-Fi & Web Server Configuration](#-wi-fi--web-server-configuration)
- [🛠️ Build](#️-build)
  - [📦 Update](#-update)
- [🔍 Interface Details](#-interface-details)
- [🏠 Bonus Jeedom Configuration](#-bonus-jeedom-configuration)
- [📦 Components for Forhorse version](#-components-for-forhorse-version-thanks-to-him)
- [📦 Components for original version](#-components-for-original-version-mine)
  - [⚙️ Choosing the ESP32 Board](#️-choosing-the-esp32-board)
- [🧵 PCB Soldering & Wiring](#-pcb-soldering--wiring)
- [🧱 3D Files](#-3d-files)
  - [🖨️ Printing Material](#️-printing-material)
  - [📂 Available in the `3Dfiles/` directory](#-available-in-the-3dfiles-directory)
  - [🔩 Components](#-components-1)
  - [🧷 Assembly](#-assembly)
- [🧾 Option: QR Code Info](#-option-qr-code-info)
- [❗ Notes](#-notes)
- [⚠️ Disclaimer & Safety Notice](#️-disclaimer--safety-notice)
- [🛡️ License](#️-license)

---

## ⏪ ESPHome Versions Before 2026.7.0

For builds using an ESPHome version earlier than `2026.7.0`, use the following legacy branch:

[legacy/esphome-pre-2026.7](https://github.com/devildant/acw02_esphome/tree/legacy/esphome-pre-2026.7)

The `main` branch is intended for ESPHome `2026.7.0` and later.

---

## ⚠️ Compatibility Warning  

> [!CAUTION]
> **USB connector AC units are not compatible with this firmware.**  
> - AC units using a **USB port as the Wi-Fi module connector** are not compatible with the `7A 7A ...` firmware provided here.  
> - These units are compatible with **Tuya MCU** instead.  
>
> - The PCBs currently provided in this repository are designed to work with the original **12 V ACW02 connector**, while USB-based versions use **5 V**.  
>
> - Some users are currently testing these USB-based units with minor PCB modifications, but this is **not officially supported for now**.  
> - If there are reliable updates, PCB modification instructions may be added here later, if time allows.

> [!WARNING]
> **Important**  
> - Works on most **Teknopoint** units *(unconfirmed but no known issues so far, see the [Community Compatibility List](#-community-compatibility-list))*.  
> - Works on most **Airton** units *(unconfirmed but no known issues so far, see the [Community Compatibility List](#-community-compatibility-list))*.
> - **Exception – Airton**: AC units using the **Wi-Fi module 409945** are **not compatible** with this firmware.  
>   - These units use a **Tuya MCU UART protocol** (`55 AA 03 ...`) instead of the custom `7A 7A ...` protocol implemented here.  
>   - You can still use **all provided hardware parts** (PCB, wiring, and 3D-printed enclosure) from this repository.  
>   - For these models, use this doc **[ACW02 ESPHome Tuya MCU integration](./docs/TUYA_MCU_SETUP.md)** instead — a basic configuration example is provided in that document and can be adapted to your needs.
> - **Exception – Some Airton mainboards**: Certain Airton units using control boards with a reference similar to `SMVHXX-XBXXXXX` may provide insufficient power on the Wi-Fi module connector for an ESP32-based ACW02 replacement.
>   - Symptoms may include random reboots, Wi-Fi instability, connection failures, or ESP32 brownout resets.
>   - If you experience these issues, power the ACW02 module from an external **5 V USB supply** (micro usb, usb c... depends on the ESP32's USB connector) while keeping the UART connection to the AC unit.
>   - In this configuration, the **original 12 V supply from the AC board must be disconnected** (but keep the GND) from the Wi-Fi/ACW02 connector to avoid back-powering or instability. (In this configuration, the polulu module is no longer necessary.)
>   - example:
    https://www.amazon.fr/dp/B0DLK81PNN?ref=cm_sw_r_cso_cp_apin_dp_1YWFK08F028ZE3RDJSZV&social_share=cm_sw_r_cso_cp_apin_dp_1YWFK08F028ZE3RDJSZV&language=en_GB&currency=EUR
    ![conv-220v-5v-usb.png](docs/images/conv-220v-5v-usb.png)
>   - This converter needs to be connected to the main power supply of the indoor unit, which is normally 220v.
>   - This issue appears to be related to the available current on specific board revisions and does not affect all Airton units.
>   - Further community feedback is welcome to identify the affected board revisions more precisely.
>   - All versions have affected my versions and that of Forhorse.


---

## ✅ Community Compatibility List  

A non-exhaustive list of tested models is available here:  
👉 [COMPATIBILITY.md](https://github.com/devildant/acw02_esphome/blob/main/COMPATIBILITY.md)  

---

## 📌 Contribute  

Help improve this project by reporting your unit compatibility.  
If your model is not referenced, please submit a report (**tested units only**):  
👉 [Submit your unit](https://github.com/devildant/acw02_esphome/issues/new?template=unit_report.yml)  
PS: If you encounter a problem, please post an issue before putting your incompatible model.

---

## 📋 Requirements

- Home Assistant with **MQTT integration** enabled and properly configured
- Enable the WIFI with the AC remote


## ⚙️ Settings

### 📁 Base Configuration

Open `esphome-acw02-en.yaml` or `esphome-acw02-fr.yaml` depending on your preferred language.

If you want to compile using the ESPHome Builder add-on in Home Assistant, use the files from the `HA-esphome-builder` folder instead (`ha-esphome-builder-acw02-en.yaml` or `ha-esphome-builder-acw02-fr.yaml`).

Edit the `substitutions` section — you can modify:

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
  lang: "en"
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

---

### 📶 Wi-Fi & Web Server Configuration

Open `secrets.yaml` and fill in your Wi-Fi credentials and desired web server access:

```yaml
wifi_ssid: "testesp32"
wifi_password: "testesp32"
wifi_ssid2: "testesp32"
wifi_password2: "testesp32"
wifi_ssid3: "testesp32"
wifi_password3: "testesp32"
```

---

## 🛠️ Build

- [Build on Windows (native)](docs/install_from_windows_directly.md)  
- [Build on MACOS (native)](docs/install_from_macos_directly.md)
- [Build on Windows with Docker](docs/install_from_docker_win.md)
- Build with ESPHome Builder (2026.7.0 minimum) with Home Assistant add-on:  
  Use the files in the [`HA-esphome-builder`](HA-esphome-builder) folder with the Home Assistant ESPHome add-on:  
  - [`ha-esphome-builder-acw02-fr.yaml`](HA-esphome-builder/ha-esphome-builder-acw02-fr.yaml) – version française  
  - [`ha-esphome-builder-acw02-en.yaml`](HA-esphome-builder/ha-esphome-builder-acw02-en.yaml) – English version  

### 📦 Update

> **🔁 After updating:**  
> I recommend pressing the **"G1: Rebuild MQTT Entities"** button to ensure all MQTT discovery topics are refreshed properly.

---

## 🔍 Interface Details

👉 [See Interface Details](docs/interface_details.md)

---

## 🏠 Bonus Jeedom Configuration

> **Note:** Jeedom is **not officially supported**. This is provided as an **informational guide only**.

- [Enable on Jeedom](docs/enable_on_jeedom.md)  

---
### 📦 Components for Forhorse version (thanks to him)
- **PCB**
  - **[Forhorse PCB](https://github.com/devildant/acw02_esphome/blob/main/docs/version_Forhorse.md)**
  ![Forhorse](PCB/images/pcb/Forhorse_PCB.png)  

- **Components**
  - **[Connector: JST XA 2.5 male/male 4 pins (standard direction) (x1)](https://www.aliexpress.com/item/1005008857984831.html)** 

    ![JST order](PCB/images/components/connector%202.PNG)

- **Configuration board in yaml**:
```yaml
 # LOLIN ESP32 C3 MINI
 board: lolin_c3_mini
 TX: GPIO21
 RX: GPIO20
```

- **(Optional) Additional configuration to add to the yaml file at the end (currently not linked to the firmware)**:
```yaml
## Start of the section for the STA LED
# Off when everything is OK
# slow blinking in case of warning (Wi-Fi connection issue, etc.)
# fast blinking in case of error
status_led:
  pin:
    number: GPIO03
    inverted: true
## End of the section for the STA LED

## Start of the section for the remote temperature probe
# Temperature probe
sensor:
  - platform: ntc
    id: external_temperature_sensor
    sensor: resistance_sensor
    calibration:
      b_constant: 3950
      reference_temperature: 25°C
      reference_resistance: 10kOhm
    name: NTC Temperature

  # Source sensors:
  - platform: resistance
    id: resistance_sensor
    sensor: source_sensor
    configuration: DOWNSTREAM
    resistor: 10kOhm
    name: Resistance Sensor
    internal: True

  - platform: adc
    id: source_sensor
    pin: GPIO01
    attenuation: 12db
    update_interval: never
    internal: True

switch:
  - platform: gpio
    pin: GPIO10
    id: ntc_vcc
    internal: True

interval:
  - interval: 10s #adjust frequency of measurement refresh here
    then:
      - switch.turn_on: ntc_vcc
      - component.update: source_sensor
      - switch.turn_off: ntc_vcc
## End of the section for the remote temperature probe
```

### 📦 Components for original version (mine)

- **[12V → 5V Regulator D24V10F5](https://shop.mchobby.be/fr/regulateurs/554--regul-5v-1a-step-down-d24v10f5-3232100005549-pololu.html)**  
  [Manufacturer](https://www.pololu.com/product/2831)  
  ![D24V10F5](PCB/images/components/D24V10F5.PNG)

- **Channels Logic Level Converter**

  > **2 options available:**  
  > You can either choose a ready-made module based on the CYT1076,  
  > or use a custom board (to be assembled by JLCPCB) designed around two SN74LVC2T45DCUR.  
  > In practice, the second solution is far more optimized for UART communication.  
  > Personally, I recommend this option: a bit more expensive but much more efficient.  
  > (Of course, I might be slightly biased since I designed it myself 🙂)

  - **[4 Channels Logic Level Converter Bi-Directional Shifter (x1): CYT1076](https://amzn.eu/d/2MhG08s)**  
    ![CYT1076](PCB/images/components/4%20Channels%20Logic%20Level%20Converter%20Bi-Directional%20Shifter.PNG)

  - **Channels Logic Level Converter (x1): SN74LVC2T45DCUR**  
    ![SN74LVC2T45DCUR](PCB/logic_level_converter/annexe/image.PNG)  
    - 📥 [Download ZIP (GERBER, BOM, Pick & Place)](https://github.com/devildant/acw02_esphome/raw/main/PCB/logic_level_converter/uart_conv_gerber_BOM_PickAndPlace.zip)
    - 👉 [JLCPCB Assembly Guide](docs/uart_convert_jlcpcb.md)

- **PCB and ESP32 (2 options available)**

  - **[ESP32 D1 MINI WROOM 32](https://github.com/devildant/acw02_esphome/blob/main/docs/version_esp32_D1_mini_wroom32.md)**  
    ![front](PCB/images/pcb/front.PNG)  
    ![front](PCB/images/pcb/frontv1.1.PNG)

  - **[LOLIN ESP32 C3 MINI](https://github.com/devildant/acw02_esphome/blob/main/docs/version_lolin_esp32_C3.md)**  
    ![front](PCB/images/pcb/front-v1.0-C3-C6.PNG)

  - **[XIAO ESP32 C3-C6](https://github.com/devildant/acw02_esphome/blob/main/docs/version_xiao_esp32_C3_C6.md)**  
    ![front](PCB/images/pcb/front-v1.0-C3-C6.PNG)


- **[5 pins male 2.54mm (x1): MaleL7.5-1X5P](https://fr.aliexpress.com/item/1005007128029220.html)**  
  ![MaleL7.5-1X5P](PCB/images/components/5%20pin%20male%202.54.PNG)

- **[6 pins male 2.54mm (x2): MaleL7.5-1X6P](https://fr.aliexpress.com/item/1005007128029220.html)**  
  ![MaleL7.5-1X5P](PCB/images/components/6%20pin%20male%202.54.PNG)

- **[Connector: JST XA 2.5 male 4 pins (Reverse direction) (x1)](https://www.aliexpress.com/item/1005008857984831.html)**  
  ![JST](PCB/images/components/connector.PNG)

📌 Important: Use reverse wiring for the following order:  
![JST order](PCB/images/components/connector%202.PNG)

---

### ⚙️ Choosing the ESP32 Board

Several ESP32 boards can be used with this module. Each has its own pros and cons depending on your needs.

#### ESP32 D1 MINI WROOM 32
- ✅ Used for development, therefore the most tested version  
- ✅ Built-in **reset** button on the case  
- ❌ Many cheap clones with poor 5V → 3.3V regulator (may cause malfunction)  
- ❌ Slightly weaker Wi-Fi signal due to component layout and antenna position  
- ❌ No external antenna support  

#### LOLIN ESP32 C3 MINI (Not recommended)
- ✅ Built-in **reset** button on the case  
- ✅ Better Wi-Fi signal compared to ESP32 D1 MINI WROOM 32  
- ❌ No external antenna support
- ❌ Trouble with modern WIFI router > WIFI 4 (qood singal but WIFI TX/RX very slow, i detect lot of difficulty with wifi 6 in 2.4GHZ)

#### XIAO ESP32 C3
- ✅ External antenna support (Wi-Fi signal ≥ LOLIN ESP32 C3 MINI)  
- ❌ No **reset** button on the case  

#### XIAO ESP32 C6
- ✅ External antenna support  
- ❌ Antenna **not included** (must be purchased separately, internal antenna has very poor reception) 
- ❌ No **reset** button on the case  
- ❌ Average Wi-Fi signal, often weaker than ESP32 D1 MINI WROOM 32 with internal antenna (that's why you need buy the external antenna)

---

### 🧵 PCB Soldering & Wiring

- RED = 12V  
- BLACK = RX  
- YELLOW = TX  
- GREEN = GND

---

## 🧱 3D Files

### 🖨️ Printing Material  

| Material | Compatible | Notes |
|----------|------------|-------|
| PLA      | ❌ No       | Not recommended (low heat resistance) |
| PETG     | ✅ Yes (recommended) | Good balance of strength & printability |
| ABS      | ✅ Yes      | Better heat resistance |
| ASA      | ✅ Yes      | UV resistant, suitable for outdoors |
| PC       | ✅ Yes      | Very strong & heat resistant |

> ⚠️ **Why not use PLA?**  
> - PLA has a glass transition temperature of only **45–50 °C (or even lower)**.  
> - By contrast, **PETG can withstand up to ~80 °C**, making it much more reliable.  
> - For electronic components that generate heat, PLA is unsafe in warm environments — especially if your AC unit also provides **heating**.  

### 📂 Available in the `3Dfiles/` directory:

#### Version 1

- [acw02 case bottom (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%201/acw02%20case%20bot.stl)
- [acw02 case top (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%201/acw02%20case%20top.stl)
- [Alternative: top case with thermo cable hole](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%201/acw02%20case%20top%20cable%20with%20thermo.stl)

#### Version 2

- [acw02 case bot with air flow (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/acw02%20case%20bot%20with%20air%20flow.stl)
- [acw02 case top air flow (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/acw02%20case%20top%20air%20flow.stl)
- [acw02 case top cable with thermo air flow (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/acw02%20case%20top%20cable%20with%20thermo%20air%20flow.stl)
- [acw02 case top cable with thermo plastic card (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/acw02%20case%20top%20cable%20with%20thermo%20plastic%20card.stl)
- [acw02 case top plastic card (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/acw02%20case%20top%20plastic%20card.stl)
- [card (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/card.stl)

#### Version 2 for pcb Forhorse
- [full pcb acw02 case bot with air flow (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/full%20pcb%20by%20Forhorse/full%20pcb%20acw02%20case%20bot%20with%20air%20flow.stl)
- [acw02 case top air flow (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/acw02%20case%20top%20air%20flow.stl)
- [acw02 case top cable with thermo air flow (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/acw02%20case%20top%20cable%20with%20thermo%20air%20flow.stl)
- [acw02 case top cable with thermo plastic card (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/acw02%20case%20top%20cable%20with%20thermo%20plastic%20card.stl)
- [acw02 case top plastic card (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/acw02%20case%20top%20plastic%20card.stl)
- [card (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/Version%202/card.stl)

![airflow1](3Dfiles/images/airflow1.jpg)

![airflow2](3Dfiles/images/airflow2.jpg)

![airflow3](3Dfiles/images/airflow3.jpg)

![airflow4](3Dfiles/images/airflow4.jpg)

![airflow5](3Dfiles/images/airflow5.jpg)

![airflow6](3Dfiles/images/airflow6.jpg) 

#### For ESP32 D1 MINI WROOM 32
- [button.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/button.stl)
- [solder board ESP32 D1 MINI.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/D1-MIMI/solder%20board%20ESP32%20D1%20MINI.stl)

  ![solder1](3Dfiles/images/solder%20board1.jpg)

#### For LOLIN ESP32 C3 MINI
- [button lolin c3.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/LOLIN-C3-XIAO-C3-C6/button%20lolin%20c3.stl)
- [solder board ESP32 C3-C6.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/LOLIN-C3-XIAO-C3-C6/solder%20board%20ESP32%20C3-C6.stl)

  ![solder1](3Dfiles/images/solder%20board1-lolin-c3.jpg)

#### For XIAO ESP32 C3-C6
- [solder board ESP32 C3-C6.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/LOLIN-C3-XIAO-C3-C6/solder%20board%20ESP32%20C3-C6.stl)

  ![solder1](3Dfiles/images/solder%20board1-xiao-c3-c6.jpg)

- support antenna (multiple config possible)
  - support antenna rigid
    - [support antenna rigid part A.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/LOLIN-C3-XIAO-C3-C6/support%20antenna%20rigid%20part%20A.stl)
    - [support antenna rigid part B.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/LOLIN-C3-XIAO-C3-C6/support%20antenna%20rigid%20part%20B.stl)

      ![solder1](3Dfiles/images/antenna-rigid.PNG)
      ![solder1](3Dfiles/images/antenna-rigid-assembly.PNG)

  - [support antenna flex horizontal.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/LOLIN-C3-XIAO-C3-C6/support%20antenna%20flex%20horizontal.stl)

    ![solder1](3Dfiles/images/antenna-flexH.PNG)
    ![solder1](3Dfiles/images/antenna-flexH2.PNG)

  - [support antenna flex vertical.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/LOLIN-C3-XIAO-C3-C6/support%20antenna%20flex%20vertical.stl)

    ![solder1](3Dfiles/images/antenna-flexV.PNG)
    ![solder1](3Dfiles/images/antenna-flexV2.PNG)


  - [support antenna flex free.stl](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/LOLIN-C3-XIAO-C3-C6/support%20antenna%20flex%20free.stl)

    ![solder1](3Dfiles/images/antenna-flexF.PNG)
    ![solder1](3Dfiles/images/antenna-flexF2.PNG)

#### BONUS: for airton support
You don't seem to have a mounting bracket for the module, so I'll provide one for you to stick on your air conditioner. :) (fix with double-sided tape)

- [support airton (.stl)](https://github.com/devildant/acw02_esphome/raw/main/3Dfiles/support%20for%20airton/support%20airton.stl)

---

### 🔩 Components

- [insert M2(OD3.2mm) Length 2.5mm 50pcs (x4)](https://www.aliexpress.com/item/1005003582355741.html)  
  ![insert M2](3Dfiles/images/bottom%20case%20insert.png)

- [insert M3(OD4.2mm) Length 3mm 50pcs (x2)](https://www.aliexpress.com/item/1005003582355741.html)  
  ![insert M3](3Dfiles/images/bottom%20case%20insert.png)

- [Optional: thermo white 4.8 mm x 10 m](https://amzn.eu/d/8Y1PVUU)  
  ![thermo](3Dfiles/images/thermo.PNG)  
  ![thermo in place](3Dfiles/images/alternative%20cable%20with%20thermo.jpg)

- Countersunk head screw M2x6mm (x4)  
- Screw M3x4mm (x2) (+1 for airton support)

---

### 🧷 Assembly

1. ![button](3Dfiles/images/button.PNG)  
2. ![bottom case](3Dfiles/images/bottom%20case%20+%20pcb%20screw.png)  
3. ![case + PCB](3Dfiles/images/case%20with%20pcb.jpg)  
4. ![case + PCB + button](3Dfiles/images/case%20with%20pcb%20+%20button.jpg)  
5. ![top case](3Dfiles/images/top%20case%20screw.png)  
6. ![closed](3Dfiles/images/case%20close.jpg)


#### BONUS : support airton

1. ![support1](3Dfiles/images/support_airton.png)

2. ![support2](3Dfiles/images/support_airton_reco_place.png)

3. ![support3](3Dfiles/images/support1.jpg)

4. ![support4](3Dfiles/images/support2.jpg)

5. ![support5](3Dfiles/images/support3.jpg)

---

## 🧾 Option: QR Code Info

You can generate a QR code to store module information:

1. Open `create_QRcode_info.html` in your browser.  
2. Import your YAML file (`esphome-acw02-en.yaml` or `esphome-acw02-fr.yaml`).  
3. Verify the data and click the QR code to download.  
4. Print and stick it on the module.

---

## ❗ Notes

This component requires **MQTT** to work properly.  
Please ensure you understand how to configure it with Home Assistant.

---

## ⚠️ Disclaimer & Safety Notice

🚨 **IMPORTANT — PLEASE READ BEFORE USING**  

 Working with electrical devices, HVAC systems, and embedded electronics carries inherent risks.  
Proceed **only if you fully understand what you are doing** and have the proper knowledge and equipment.  

- 🔌 Always disconnect power before modifying or connecting hardware.  
- ⚙️ Double-check wiring, pin assignments, and voltages before powering on.  
- 💥 Incorrect wiring or configuration may permanently damage your **AC unit**, **ESP module**, or **other connected components**.  
- 🔥 Electrical work should be done at your own risk and in compliance with local regulations.  

**I take no responsibility** for any direct, indirect, incidental, or consequential damages,  
including (but not limited to) **hardware failure, data loss, personal injury, fire, or electric shock** resulting from the use of this project.  

By using the files, code, or documentation provided in this repository,  
you acknowledge that you do so **entirely at your own risk**.

---

# 🛡️ License

This repository uses multiple licenses depending on content type:

- **Code** (`*.yaml`, `*.cpp`, `*.h`) — [GNU GPLv3](https://www.gnu.org/licenses/gpl-3.0.html)
- **PCB files** (`*.zip`, `*.json`) — [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)
- **3D models** (`*.f3d`, `*.stl`) — [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/)

> Commercial use of PCB and 3D files is **strictly prohibited**.
