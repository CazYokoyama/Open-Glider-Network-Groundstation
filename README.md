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

Features:
* Power consumption around 20 mA in sleep mode - 120mA on normal mode ;
* Auto wakeup on RX packages ;
* Wakeup Timer
* No additional computer necessary - only wireless ;
* Ideal for solar and battery supply
* low cost hardware

## Installation

* Install arduino ide
* Install ESP32 Filesystem Uploader
* Compile and upload Sketch and files (.css and .html)
* Connect to OGN Wifi AP and configure
* You can update you installation http://you-ogn-ground-ip/update

**Feel free to contact me - manuel.roesel@ros-it.ch**

![alt text](https://ros-it.ch/wp-content/uploads/2020/10/ogn_base.png)
