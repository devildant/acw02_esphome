# 🔧 Build from Docker (Windows)

## 🐳 Install Docker Desktop on Windows

You can install it from the Microsoft Store or directly from:  
[Docker Windows Install Docs](https://docs.docker.com/desktop/setup/install/windows-install/)

---

## 🐧 Install WSL

Run PowerShell as administrator and execute:  
```powershell
wsl --install
wsl --install -d Ubuntu-22.04
```

![usbipd list](../docker/images/docker%20desktop.PNG)

---

## 🔌 Install usbipd

- [Windows docs](https://learn.microsoft.com/fr-fr/windows/wsl/connect-usb)  
- [GitHub repo for download](https://github.com/dorssel/usbipd-win)

---

## 📦 Share USB to container

### Manually (important for understand script):

1. Connect your ESP via USB  
2. Run PowerShell as administrator  
3. List USB devices:
   ```powershell
   usbipd list
   ```
   ![usbipd list](../docker/images/list%20usbipd.PNG)

4. Find your ESP (e.g. for ESP32: *Silicon Labs CP210x USB to UART Bridge (COMX)*)  
5. Get the associated BUSID and bind it (replace `3-3` with your BUSID):
   ```powershell
   usbipd bind --busid 3-3
   ```

6. Run the list command again:
   ```powershell
   usbipd list
   ```
   ![usbipd list 2](../docker/images/list%20usbipd2.PNG)

7. Attach USB to WSL:
   ```powershell
   usbipd attach --wsl --busid 3-3
   ```
   ![usbipd attach](../docker/images/list%20usbipd3.PNG)

   
#### 🔁 Reverse commands

```powershell
usbipd detach --busid 3-3
usbipd unbind --busid 3-3
```

---

### With script:

1. Connect your ESP via USB  
2. Run PowerShell as administrator  
3. run cmd bellow for attach usb, this script ask you the BUSID

   ```powershell
   attach-usb.bat
   ```
   ![usbipd attach](../docker/images/usbipd%20attach.PNG)


#### 🔁 Reverse commands
1. run cmd bellow for attach usb, this script ask you the BUSID

   ```powershell
   detach-usb.bat
   ```
   ![usbipd attach](../docker/images/usbipd%20detach.PNG)

---

## 🛠️ Build component

### ⚙️ Install

1. Open PowerShell in the `docker` directory (inside the `acw02_esphome` project)  
2. Run commands one by one:
   ```powershell
   docker compose build
   docker compose up -d esphome-usb
   docker compose up -d esphome-usb-c3c6
   docker compose up -d esphome-ota
   ```
   or use script 
   ```powershell
   build-docker.bat
   start-docker.bat
   ```

3. Build and push firmware to USB (params ota | usb | usb-c3c6):
   ```powershell
   build-fr.bat
   ```
   or (depending on language) (params ota | usb | usb-c3c6):
   ```powershell
   build-en.bat
   ```

### 🧪 Optional commands

- `build-docker.bat` : build docker container
- `build-compile-only-fr.bat` : build only (FR), no flash  
- `build-compile-only-en.bat` : build only (EN), no flash  
- `open-container.bat` : enter Docker container (params ota | usb)
- `start-docker.bat` : only start docker (ota only)
- `stop-docker.bat` : only stop docker (ota + usb + usb-c3c6)
- `attach-usb.bat` : script for attach usb to docker
- `detach-usb.bat` : script for detach usb to docker
- `start-docker-with-attach-USB.bat` : run attach USB to docker and start docker (ota + usb + usb-c3c6)
- `stop-docker-with-attach-USB.bat` : run stop docker and detach USB to docker (ota + usb + usb-c3c6)
- `logs-fr.bat` : show logs OTA (FR)
- `logs-en.bat` : show logs OTA (EN)
- `clear-memory.bat` : reset memorised config  (usb | usb-c3c6)

---

## 📁 Bin files

You can find the generated `.bin` files in:  
```
docker/bin_generated
```
