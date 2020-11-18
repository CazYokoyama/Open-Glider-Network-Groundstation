# FANET Protocol

* 868.2Mhz
* Syncword 0xF1
* 250kHz Bandwidth
* Spreading Factor 7
* ExplicitHeader: Coding Rate CR 5-8/8 (depending on #neighbors), CRC for Payload

## Header:
### [Byte 0]

* 7bit 		Extended Header
* 6bit 		Forward
* 5-0bit 		Type

## Source Address:
#### [Byte 1-3]

* 1byte		Manufacturer
* 2byte		Unique ID (Little Endian)

## Extended Header:
#### [Byte 4 (if Extended Header bit is set)]

* 7-6bit 		ACK:
 - 0: none
 - 1: requested
 - 2: requested (via forward, if received via forward (received forward bit = 0). must be used if forward is set)
 - 3: reserved

* 5bit		Cast:
 - 0: Broadcast (default)
 - 1: Unicast (adds destination address (8+16bit)) (shall only be forwarded if dest addr in cache and no 'better' retransmission received)
 - 4bit 		Signature (if 1, add 4byte)
 - 3bit		Geo-based Forwarded	(prevent any further geo-based forwarding, can be ignored by any none-forwarding instances)
 - 2-0bit 		Reserved	(ideas: indicate multicast interest add 16bit addr, emergency)

# Types:

* ACK (Type = 0)
  - No Payload, must be unicast

## Tracking (Type = 1)
[recommended intervall: floor((#neighbors/10 + 1) * 5s) ]
Note: Done by app layer of the fanet module

#### [Byte 0-2]

Position	(Little Endian, 2-Complement)

* bit 0-23	Latitude 	(Absolute, see below)

#### [Byte 3-5]

Position	(Little Endian, 2-Complement)

* bit 0-23	Longitude 	(Absolute, see below)

#### [Byte 6-7]

Type		(Little Endian)

* bit 15 		Online Tracking
* bit 12-14	Aircraft Type
 - 0: Other
 - 1: Paraglider
 - 2: Hangglider
 - 3: Balloon
 - 4: Glider
 - 5: Powered Aircraft
 - 6: Helicopter
 - 7: UAV
* bit 11		Altitude Scaling 1->4x, 0->1x
* bit 0-10	Altitude in m

#### [Byte 8]	Speed		(max 317.5km/h)
* bit 7		Scaling 	1->5x, 0->1x
* bit 0-6		Value		in 0.5km/h		

#### [Byte 9]

Climb		(max +/- 31.5m/s, 2-Complement)

* bit 7 Scaling 	1->5x, 0->1x
* bit 0-6 Value		in 0.1m/s

#### [Byte 10]	Heading
* bit 0-7 Value in 360/256 deg

#### [Byte 11] - [optional]

Turn rate 	(max +/- 64deg/s, positive is clock wise, 2-Complement)

* bit 7 Scaling 	1->4x, 0->1x
* bit 0-6 Value 		in 0.25deg/s

#### [optional, if used byte 11 is mandatory as well]
* [Byte 12]	QNE offset 	(=QNE-GPS altitude, max +/- 254m, 2-Complement)
* bit 7		Scaling 	1->4x, 0->1x
* bit 0-6	Value in m

## Name (Type = 2)
[recommended intervall: every 4min]

* 8bit String (of arbitrary length, \0 termination not required)

## Message (Type = 3)

* [Byte 0]	Header
 - bit 0-7 	Subheader, Subtype (TBD)
 - 0: Normal Message

* 8bit String (of arbitrary length)


## Service (Type = 4)
[recommended intervall: 40sec]

#### [Byte 0]

Header	(additional payload will be added in order 6 to 1, followed by Extended Header payload 7 to 0 once defined)

* bit 7 Internet Gateway (no additional payload required, other than a position)
* bit 6	Temperature (+1byte in 0.5 degree, 2-Complement)
* bit 5	Wind (+3byte: 1byte Heading in 360/256 degree, 1byte speed and 1byte gusts in 0.2km/h (each: bit 7 scale 5x or 1x, bit 0-6))
* bit 4	Humidity (+1byte: in 0.4% (%rh*10/4))
* bit 3	Barometric pressure normailized (+2byte: in 10Pa, offset by 430hPa, unsigned little endian (hPa-430)\*10)
* bit 2	Support for Remote Configuration (Advertisement)
* bit 1	State of Charge  (+1byte lower 4 bits: 0x00 = 0%, 0x01 = 6.666%, .. 0x0F = 100%)
* bit 0	Extended Header (+1byte directly after byte 0)
The following is only mandatory if no additional data will be added. Broadcasting only the gateway/remote-cfg flag doesn't require pos information.

#### [Byte 1-3 or Byte 2-4]	Position	(Little Endian, 2-Complement)		

* bit 0-23	Latitude	(Absolute, see below)

#### [Byte 4-6 or Byte 5-7]	Position	(Little Endian, 2-Complement)

* bit 0-23	Longitude   (Absolute, see below)
+ additional data according to the sub header order (bit 6 down to 1)

## Landmarks (Type = 5)

Note: Landmarks are completely independent. Thus the first coordinate in each packet has to be an absolute one. All others are compressed in relation to the one before.

Note2: Identification/detection shall be done by hashing the whole payload, excluding bytes 0, 1 and, 2 (optional). That way one quietly can change the layer to 'Don't care' and quickly
destroy the landmark w/o having to wait for it's relative live span to be exceeded.

Note3: In case a text has the same postion as the first position of any other landmark then the text is considered to be the label of that landmark.

#### [Byte 0]

 * bit 4-7		Time to live +1 in 10min (bit 7 scale 6x or 1x, bit 4-6) (0>10min, 1->20min, ..., F->8h)
 * bit 0-3		Subtype:
	 - 0:     Text
	 - 1:     Line
	 - 2:     Arrow
	 - 3:     Area
	 - 4:     Area Filled
	 - 5:     Circle
	 - 6:     Circle Filled
	 - 7:     3D Line		suitable for cables							
	 - 8:     3D Area		suitable for airspaces (filled if starts from GND=0)
	 - 9:     3D Cylinder	suitable for airspaces (filled if starts from GND=0)
	 - 10-15: TBD

#### [Byte 1]

  * bit 7-5		Reserved
  * bit 4		Internal wind dependency (+1byte wind sector)
  * bit 3-0		Layer:
	 - 0:     Info
	 - 1:     Warning
	 - 2:     Keep out
	 - 3:     Touch down
	 - 4:     No airspace warn zone		(not yet implemented)
	 - 5-14:  TBD
	 - 15:    Don't care

#### [Byte 2]

 only if internal wind bit is set] Wind sectors +/-22.5degree (only display landmark if internal wind is within one of the advertised sectors.

If byte 2 is present but is zero, landmark gets only displayed in case of no wind)

 * bit 7 NW
 * bit 6	W
 * bit 5	SW
 * bit 4 S
 * bit 3 SE
 * bit 2 E
 * bit 1 NE
 * bit 0 N
