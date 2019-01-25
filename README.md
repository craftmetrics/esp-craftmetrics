# esp-craftmetrics

This is an example sketch for sending data to Craft Metrics from an ESP8266 or ESP32 microcontroller using the Arduino environment.

## Requirements

* [ESP8266 Arduino](https://github.com/esp8266/Arduino) or [ESP32 Arduino](https://github.com/espressif/arduino-esp32)
* The [OneWire](https://github.com/PaulStoffregen/OneWire) library is used in this example

## Setup

* Connect a OneWire DS18B20 temperature sensor to GPIO 2
* Include a 4.7k pullup resistor
* Edit the top of the sketch to include your wifi credentials, and Craft Metrics account details.

## Usage

The device will send data to Craft Metrics every 10 seconds. Log into your dashboard and create a new graph, selecting the device name from the list of measurements.

## Support

support@craftmetrics.ca