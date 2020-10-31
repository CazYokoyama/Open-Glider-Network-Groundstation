As of version 0.2.6, a complete sender (aircraft) beacon message looks as follows:

    FLRDF0A52>APRS,qAS,LSTB:/220132h4658.70N/00707.72Ez090/054/A=001424 !W37! id06DF0A52 +020fpm +0.0rot 55.2dB 0e -6.2kHz gps4x6 s6.01 h03 rDDACC4 +5.0dBm hearD7EA hearDA95

---

The first two groups are standard APRS:

    FLRDF0A52>APRS,qAS,LSTB:/220132h4658.70N/00707.72Ez090/054/A=001424 !W37!

* 'FLRDF0A52>': origin
  * 'FLRDF0A52': sender callsign (may include device ID)
* 'APRS,qAS,LSTB:': source
  * 'APRS': software version (always "APRS")
  * 'qAS': [q construct](http://aprs-is.net/q.aspx) (not suited for beacon identifcation)
  * 'LTSB': receiver callsign
* '/': position with timestamp follows
* '220132h': timestamp in 24h notation UTC (22 hours, 1 minute, 32 seconds)
* '4658.70N/00707.72Ez': position (WSG84) - see position precision enhancement below as well
  * '4658.70N': latitude (46 degrees, 58.70 minutes north)
  * '/': [display symbol](http://wa8lmf.net/aprs/APRS_symbols.htm) - table to use (primary symbol table)
  * '00707.72E': longitude (007 degrees, 07.77 minutes east)
  * 'z': [display symbol](http://wa8lmf.net/aprs/APRS_symbols.htm) - position in table "/" to use (house symbol)
* '090/054': *optional* heading and speed ("000/000" indicates no data)
  * '090': heading (001 thru 360) in degrees ("090" = east)
  * '054': ground speed in knots
* '/A=001424': altitude (1424 feet)
* '!W37!': position precision enhancement
  * '3': latitude minutes third decimal digit (46 degrees, 58.70<b>3</b> minutes north)
  * '7': longitude minutes third decimal digit (007 degrees, 07.72<b>7</b> minutes east)

---

All subsequent groups are OGN specific sender details:

    id06DF0A52 +020fpm +0.0rot FL000.00 55.2dB 0e -6.2kHz gps4x6 s6.01 h03 rDDACC4 +5.0dBm hearD7EA hearDA95

* 'id02DF0A52'
  * 0x'06' = 0b'00000110': sender details (2 digit hex number encoding 8 bits)
      * 0b'0' = 'false': stealth mode boolean (should never be "1")
      * 0b'0' = 'false': no-tracking boolean (must ignore if "1")
      * 0b'0001' = '1':
      * 0b'10' = '2': address type - 01 ICA 02 FLR 03 OGN 04 P3I 05 FNT 0 RANDOM
  * 'DF0A52': sender address
* '+020fpm': climb rate in "feet per minute"
* '+0.0rot': turn rate in "half turn per 2 minutes"
* 'FL000.00': *optional* flight level
* '55.2dB': signal to noise ratio (5db are considered the lower limit for meaningful reception)
* '0e': CRC error rate (>5 is considered unusable)
* '-6.2kHz': frequency offset (should be constant thanks to GSM sync)
* 'gps4x6': *optional* GPS signal quality
 * '4': horizontal accuracy (4 meters)
 * '6': vertical accuracy (6 meters)
* 's6.01': *optional* FLARM software version
* 'h03': *optional* FLARM hardware version (hex)
* 'rDDACC4': *optional* FLARM device ID
* '+5.0dBm': *optional* power ratio :warning: unconfirmed
* 'hearD7EA hearDA95': *optional* other FLARM devices received by this device
  * 'hearD7EA': receiving FLARM device with ID ending "D7EA"
  * 'hearDA95': receiving FLARM device with ID ending "DA95"
