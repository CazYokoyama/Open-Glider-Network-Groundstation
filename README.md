# OGN-Basestation

Groundstation for Open Glider Network with ESP32 

**I want to highlight that I am a non-professional programmer.**

The aim of this project is to create a simple base station for the OGN network. The SoftRF project was used as the base. Thanks to Linar Yusupov for this great work!

A TTGO T-Beam (SoftRF Prime Edition MkII) is used as hardware, which has WLAN, Bluetooth, U-Blox GPS and a LoraWan chip.

[TTGO T-Beam Aliexpress](https://s.click.aliexpress.com/e/_dTnxWSv)

The TT-Beam does not need another computer (Raspberry) and sends the APRS messages directly into the Open Glider network.
Only WiFi and callsign have to be configured, the position is determined via gps. 

Since the T-Beam is very energy-saving, it can be operated very easily on a battery with solar panels.

There are also a few drawbacks to the traditional OGN receivers. Several protocols cannot be decoded at the same time (at the moment).

Features:
* Power consumption around 20 mA in sleep mode - 120mA on normal mode ;
* auto wakeup on RX packages ;
* No additional computer necessary - only wireless ;
* Ideal for solar and battery supply
* low cost hardware
