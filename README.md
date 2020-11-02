# OGN-Basestation

Groundstation for Open Glider Network with ESP32

**I want to highlight that I am a non-professional programmer.**
**This software is at alpha status.**

The aim of this project is to create a simple base station for the OGN network. The SoftRF project was used as the base. Thanks to Linar Yusupov for this great work!

A TTGO T-Beam (SoftRF Prime Edition MkII) is used as hardware, which has Wifi, Bluetooth, U-Blox GPS and a LoraWan chip.

[TTGO T-Beam Aliexpress](https://s.click.aliexpress.com/e/_dTnxWSv)

The TT-Beam does not need another computer (Raspberry) and sends the APRS messages directly into the Open Glider network.
Only WiFi and callsign have to be configured, the position is determined via gps.

Since the T-Beam is very energy-saving, it can be operated very easily on a battery with solar panels.

There are also a few drawbacks to the traditional OGN receivers. Several protocols cannot be decoded at the same time (at the moment - second protocol has no function).

## Features:
* Power consumption around 20 mA in sleep mode - 120mA on normal mode ;
* Auto wakeup on RX packages ;
* Wakeup Timer
* No additional computer necessary - only Wifi ;
* Ideal for solar and battery supply
* low cost hardware

## Planed Features
* Protocol hopping - aprs messages are sent every 5 seconds
	- there should be enough time to decode a second protocol
* ADS-B decoder
	- we still need some hardware
* send APRS messages over LoraWan
* send Fanet weather data

## Known bugs / Missing features
* SNR calculation not correct
* No RAM and CPU data in APRS status messages
* no GPS signal quality data in APRS messages
* no turn rate in APRS messages (always zero)
* disable sleep mode if everyone is connected to webserver
* and a lot more...

## Installation

* Install arduino ide
* Install ESP32 Filesystem Uploader
* Compile and upload Sketch and files (.css and .html)
* Connect to OGN Wifi AP and configure
* You can update you installation http://you-ogn-ground-ip/update
* You can upload new .html .css files http://you-ogn-ground-ip/upload

## Easy Installation

* Download [binary image](https://github.com/roema/Open-Glider-Network-Groundstation/releases) and install it with web-updater
* Connect to Wifi OGN-XXXXXX with password 987654321
* Type 192.168.1.1 in you browser and you will see a file-upload page (only on first startup)
* Upload index.html, update.html and style.css
* You can also upload a file called <ogn_conf.txt> with Wifi and Callsign configuration
* Reset and connect again

### Installation Problems
* Bootloop - erase flash with
	- **esptool.py  erase_flash**
	- **ESP32 Download Tool - ERASE**
	- use always actual .html .css files

## Update / File Uploader

Firmware updater can be reached at http://you-ogn-ground-ip/update  
File uploader can be reached at http://you-ogn-ground-ip/upload  

During the first start-up you will be automatically directed to the file upload page.  
After the html and css files have been uploaded, the T-Beam can be restarted.   After that, the surface should be visible like the picture.  

### ogn_conf.txt example

| Line    | Example    |
| ------- |:----------:|
| ssid    | myNetwork  |
| pass    | myPassword |
| origin  | LSZX       |
| Lat			| 46.12345	 |
| Lon     | 7.123456	 |
| Alt     | 550        |
| geoid   | 45         |

```
myNetwork
myPassword
LSZX
46.123456
7.123456
550
45
```


## UDP Debugging

A simple Python script is sufficient to debug the APRS messages. APRS messages are not output via the serial interface.

When the T-Beam is connected to the wifi it will send the debug messages to the ip address ending with 200.

As an an example

* 10.0.0.200
* 192.168.1.200
* 172.21.1.200

The destination port can be freely selected in the web interface. 12000 is used as the default port.

```python
#!usr/bin/python3

import socket
import datetime

sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)

udp_host = "xxx.xxx.xxx.xxx"		# Host IP
udp_port = 12000		    # specified port to connect

sock.bind((udp_host,udp_port))

while True:
	data,addr = sock.recvfrom(1024)
	now = datetime.datetime.now()
	print("%s Received Messages: %s"%(now,data))
```

## Configuration example

* MULHBtest
	- origin
* Lat / Lon / Alt
	- if zero, GPS position is used
* GeoID
	- if Lat / Lon / Alt is zero - GPS is used
* APRS Debug
	- sending UDP messages to xxx.xxx.xxx.200 with port 12000
	- take a look to udp_client.py
* Ignore Track / Stealth Bit
	- if False, Aircrafts with Stealth or No-Track bit set will not be forwarded to OGN
* Sleep Mode
	- Full - ESP32 and GPS will go sleep after RX ilde seconds (min. 1800 seconds) ~ 20mA
		- only SX12xx stay awake
	- Without GPS ~80mA
	- Disabled ~120mA
* Wake up Timer
	- ESP will wake up after 3600 sec even if no package was received
		- ***Attention, the T-Beam cannot be brought out of sleep mode remotely!***
	- to disable enter zero




![alt text](https://ros-it.ch/wp-content/uploads/2020/11/OGN_Base.png.png)

I am always open to wishes and suggestions, but would also like to emphasize that I do this project in my spare time. I am not responsible for any damage caused by this software.  

**Feel free to contact me - manuel.roesel@ros-it.ch**

## Thanks to my testers
* Peter A.
* Nick B.

## Thanks to..

Ivan Grokhotkov - Arduino core for ESP8266  
Zak Kemble - nRF905 library  
Stanislaw Pusep - flarm_decode  
Paul Stoffregen - Arduino Time Library  
Mikal Hart - TinyGPS++ and PString Libraries  
Phil Burgess - Adafruit NeoPixel Library  
Andy Little - Aircraft and MAVLink Libraries  
Peter Knight - TrueRandom Library  
Matthijs Kooijman - IBM LMIC and Semtech Basic MAC frameworks for Arduino  
David Paiva - ESP8266FtpServer  
Lammert Bies - Lib_crc  
Pawel Jalocha - OGN library  
Timur Sinitsyn, Tobias Simon, Ferry Huberts - NMEA library  
yangbinbin (yangbinbin_ytu@163.com) - ADS-B encoder C++ library  
Hristo Gochkov - Arduino core for ESP32  
Evandro Copercini - ESP32 BT SPP library  
Limor Fried and Ladyada - Adafruit BMP085 library  
Kevin Townsend - Adafruit BMP280 library  
Limor Fried and Kevin Townsend - Adafruit MPL3115A2 library  
Oliver Kraus - U8g2 LCD, OLED and eInk library  
Michael Miller - NeoPixelBus library  
Shenzhen Xin Yuan (LilyGO) ET company - TTGO T-Beam and T-Watch  
JS Foundation - jQuery library  
XCSoar team - EGM96 data  
Mike McCauley - BCM2835 C library  
Dario Longobardi - SimpleNetwork library  
Benoit Blanchon - ArduinoJson library  
flashrom.org project - Flashrom library  
Robert Wessels and Tony Cave - EasyLink library  
Oliver Jowett - Dump978 library  
Phil Karn - FEC library  
Lewis He - AXP20X library  
Bodmer - TFT library  
Michael Kuyper - Basic MAC library  
