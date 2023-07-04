# ESP2Flutter

Simple ESP32 communication with Flutter app via BLE.

## ESP32 App
Smartcane integrated with ESP32, HC-SRO4, and MPU6050 for assistive functions. Designed for visually-impaired individuals.

### Installation
Include Adafruit MPU6050 sensor library and Adafruit Unified Sensor Driver. Simply drag and drop `esp/src/main.cpp` in a new project from either of the two platforms.
1. [PlatformIO]
2. [Arduino]

For [PlatformIO], include also `esp/src/platformio.ini`. 


## Flutter App

Simple [Flutter] app for notifications at an instance of fall in the ESP device.



[PlatformIO]: https://platformio.org/
[Arduino]: https://www.arduino.cc/
[Flutter]: https://flutter.dev/