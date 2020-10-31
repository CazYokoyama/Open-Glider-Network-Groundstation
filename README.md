# OGN-Basestation

Groundstation for Open Glider Network with ESP32 

**I want to highlight that I am a non-professional programmer.**
**This software is at alpha status.**

The aim of this project is to create a simple base station for the OGN network. The SoftRF project was used as the base. Thanks to Linar Yusupov for this great work!

A TTGO T-Beam (SoftRF Prime Edition MkII) is used as hardware, which has WLAN, Bluetooth, U-Blox GPS and a LoraWan chip.

[TTGO T-Beam Aliexpress](https://s.click.aliexpress.com/e/_dTnxWSv)

The TT-Beam does not need another computer (Raspberry) and sends the APRS messages directly into the Open Glider network.
Only WiFi and callsign have to be configured, the position is determined via gps. 

Since the T-Beam is very energy-saving, it can be operated very easily on a battery with solar panels.

There are also a few drawbacks to the traditional OGN receivers. Several protocols cannot be decoded at the same time (at the moment - second protocol has no function).

## Features:
* Power consumption around 20 mA in sleep mode - 120mA on normal mode ;
* Auto wakeup on RX packages ;
* Wakeup Timer
* No additional computer necessary - only wireless ;
* Ideal for solar and battery supply
* low cost hardware

## Planed Features
* Protocol hopping - aprs messages are sent every 5 seconds - there should be enough time to decode a second protocol
* Send APRS messages over LoraWan

## Installation

* Install arduino ide
* Install ESP32 Filesystem Uploader
* Compile and upload Sketch and files (.css and .html)
* Connect to OGN Wifi AP and configure
* You can update you installation http://you-ogn-ground-ip/update

**Feel free to contact me - manuel.roesel@ros-it.ch**

## Configuration example

* MULHBtest - origin
* Lat / Lon / Alt - if zero, GPS position is used
* GeoID - if Lat / Lon / Alt is zero - GPS is used
* APRS Debug - UDP - sending UDP messages to xxx.xxx.xxx.200 with port 12000
* Ignore Track / Stealth Bit - if False - Aircrafts with Stealth or No-Track bit set will not be forwarded to OGN
* Sleep Mode - Full - ESP32 and GPS will go sleep after RX ilde seconds - minimum 1800 seconds
* Wake up Timer - ESP will wake up after 3600 sec even if no package was received - to disable enter zero

![alt text](https://ros-it.ch/wp-content/uploads/2020/10/OGN_Base-1.png)
