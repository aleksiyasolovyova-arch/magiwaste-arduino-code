# MagiWaste Arduino ESP32

## Team Information
- **Team name:** MagiCorp
- **Team Members**:
    - MJ
    - Szymon
    - Storm
    - Aleks
    - Pam

## Responsibilities
- **Sensors:** Szymon
- **Code Implementation:** MJ
- **Review:** Storm, Pam, Aleks

## Project Overview
This project involves using an ESP32 microcontroller to monitor bin information such as :
    - Bin fill rate using 2 ultrasonic sensors;
    - Checking temperature using a DHT11 Temperature sensor;
    - A tilt sensor to control when a bin has been tilted;
    - Infrared sensor checking when waste has been thrown in.

## Installation and Configuration

1. Hardware Assembly:
  - Connect sensors to specified GPIO pins on the ESP32
  - Ensure that there is a proper power supply to the ESP32 and sensors
2. Software Configuration:
  - Install the needed Libraries
  - Make sure WiFi credentials are correct
  - Flash the code onto the ESP32
3. Server Setup:
  - Configure the MQTT broker to send data on emqx/ESP32
  - setup HTTP server to accept JSON data @ http://10.134.178.158:8080/data


### Functionality
1. Bin Fill Rate:
  - Two ultrasonic sensors measure the distance between the sensor and the top of the waste in the bin.

2. Temperature Monitoring:
  - A DHT11 sensor collects temperature and humidity data.

3. Tilt Detection:
  -  A tilt sensor monitors whether the bin has been tilted.

4. Waste Detection:
  - An infrared (IR) sensor detects when waste is added.

### Prerequisites

### Setup

## Hardware Specifications
 **Microcontroller:** ESP32

- **Sensors:**
  - **DHT11 Sensor:**
    - Temperature Measurement
    - Data Pin: GPIO 15
  - **Ultrasonic Sensors:**
    - Distance Measurement
    - Sensor 1:
      - Trig Pin: GPIO 5
      - Echo Pin: GPIO 18
    - Sensor 2:
      - Trig Pin: GPIO 19
      - Echo Pin: GPIO 21
  - **Tilt Sensor**
    - Detect bin tilt
    - Tilt Pin: GPIO 22
  - **Infrared Sensor**
    - Detect waste insertion
    - Sensor Pin: GPIO4Ardu

## Libraries Used
 - [Arduino]()
 - [PubSubClient](https://github.com/knolleary/pubsubclient)
 - [WiFi]
 - [HTTPClient](https://github.com/amcewen/HttpClient)
 - [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
 - [WifiManager](https://github.com/tzapu/WiFiManager)
 - [DHTesp](https://github.com/beegee-tokyo/DHTesp)
 - [Ticker](https://github.com/espressif/arduino-esp32/tree/master/libraries/Ticker)


## Troubleshooting

- WiFi Issues:
  - Ensure the KdG-iDev network is active
  - verify WiFi credentials

- Sensor Issues:
  - checking wiring connection and make sure voltage levels are correct
  - check if you haven't switched up the GND and 3V3 pin (I for sure did)
  - for temperature sensor, make sure there is a sufficient delay provided for accurate readings

- Server Issues:
  - confirm if the server is available @ http://10.134.178.158:8080/data

- JSON Issues:
  - Check if the data is sent through the body and not the header
  - make sure that proper spacing and quotation is respected
  - For MQTT, check if the broker is active

- HTTP Issues:
  - verify request handling
  - try a POST/ CURL command with the parameters to see if you can reach it.
  - Use the debug method to see what payload it is sending

## Notes
 - The DHT11 sensor has a slow response time readings may be up to 2 seconds old
 - Ultrasonic sensors might give inaccurate readings in high humidity or noisy environments
 - IR sensors are sensitive to reflective surfaces, which might affect detection accuracy


## Thanks

