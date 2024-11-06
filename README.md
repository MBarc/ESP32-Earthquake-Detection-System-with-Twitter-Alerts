# ESP32 Earthquake Detection System with Twitter Alerts

This project is an **Earthquake Detection and Alert System** built using an **ESP32 microcontroller**, **MPU6050 accelerometer and gyroscope**, **TP4056 charging module**, and a **rechargeable USB-C battery**. When the device detects a seismic event, it sends a **Twitter alert** to notify users.

The project is designed for portability and reusability, making it a versatile addition to IoT and environmental monitoring solutions.

## Table of Contents
- [Features](#features)
- [Hardware](#hardware)
- [Software](#software)
- [Setup and Installation](#setup-and-installation)
- [Usage](#usage)
- [License](#license)

---

## Features

- **Real-time earthquake detection**: Continuously monitors seismic activity based on acceleration data.
- **Threshold-based alert**: Sends a Twitter alert when earthquake conditions are met.
- **Portable and rechargeable**: Powered by a USB-C rechargeable battery module (TP4056).
- **Secure Twitter integration**: OAuth 1.0a authentication for secure Twitter communication.
- **Customizable detection parameters**: Adjustable sensitivity and duration thresholds for earthquake detection.

## Hardware

1. **ESP32** - Main microcontroller with Wi-Fi capability for Twitter alerts.
2. **MPU6050 Accelerometer and Gyroscope** - Measures acceleration to detect potential earthquakes.
3. **TP4056 Charging Module** - Manages battery charging via USB-C.
4. **Lithium-Ion Battery** - Provides portable power for the system.

## Software

The project software is written in C++ and uses the **Arduino framework** with the following key libraries:

- **Wire.h** - Handles I2C communication with MPU6050.
- **I2Cdev.h** and **MPU6050.h** - Interfaces with the MPU6050 sensor.
- **WiFi.h** - Connects to Wi-Fi for internet access.
- **HTTPClient.h** - Sends HTTP requests to the Twitter API.
- **mbedtls/md.h** - For SHA-1 and HMAC computation needed in OAuth signing.
- **NTPClient.h** - Syncs the device with Network Time Protocol for precise timestamping.

## Setup and Installation

### Hardware Setup
![alt text](https://github.com/MBarc/ESP32-Earthquake-Detection-System-with-Twitter-Alerts/blob/main/hardwareSetup.png)

### Software Setup

1. **Clone this repository**:
   ```bash
   git clone https://github.com/MBarc/ESP32-Earthquake-Detection-System-with-Twitter-Alerts.git
   cd ESP32-Earthquake-Detection-System-with-Twitter-Alerts
2. Open the project in the Arduino IDE or VSCode with the PlatformIO extension.
3. **Install required libraries**:
   - In the Arduino IDE, go to **Tools > Manage Libraries** and install the libraries listed above.
   - For PlatformIO, add dependencies to the *platformio.ini* file or use the Library Manager.
4. **Configure Twitter OAuth Credentials**
   - Obtain **API Key**, **API Secret Key**, **Access Token**, and **Access Token Secret** from your Twitter Developer account.
   - Add your Twitter credentials in the code as follows:
     ```cpp
     const char* apiKey = "YOUR_API_KEY";
     const char* apiSecretKey = "YOUR_API_SECRET";
     const char* accessToken = "YOUR_ACCESS_TOKEN";
     const char* accessTokenSecret = "YOUR_ACCESS_SECRET";
5. Connect the ESP32 to your computer and upload the code using the Arduino IDE or PlatformIO.

### Usage

1. Place device somewhere that is not prone to strong vibrations.
2. Turn on the device by flipping the switch.
3. Wait for an earthquake to happen.
4. Check Twitter to see the alert notification.

If you decide to monitor the serial output, you should see an output like this when an earthquake occurs:
  ```plaintext
    Accel (X, Y, Z): 2004 11808 -14252  Magnitude: 18616
    accelMagnitude is greater than threshold!
    Potential earthquake detected! Monitoring...
    ...
    Earthquake confirmed!
    Sending Twitter alert...

### License

This project is licensed under the MIT License - see the LICENSE file for details.
