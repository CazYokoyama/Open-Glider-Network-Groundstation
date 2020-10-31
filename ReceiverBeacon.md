As of version 0.2.6, a complete receiver beacon message looks as follows:

    LKHS>APRS,TCPIP*,qAC,GLIDERN2:/211635h4902.45NI01429.51E&000/000/A=001689

This message is standard APRS:

* 'LKHS>': origin
  * 'LKHS': receiver callsign
* 'APRS,TCPIP*,qAC,GLIDERN2:': source
  * 'APRS': software version (always "APRS")
  * 'TCPIP*': tunneling (always "TCPIP*")
  * 'qAC': [q construct](http://aprs-is.net/q.aspx) (not suited for beacon identifcation)
  * 'GLIDERN2': receiver callsign (APRS server)
* '/': either "/" (timestamp and position follow) or ">" (timestamp follows)
* '211635h': timestamp in 24h notation UTC (21 hours, 16 minutes, 35 seconds)
* '4902.45NI01429.51E&': <i>optional</i> position (WSG84)
  * '4902.45N': latitude (49 degrees, 2.45 minutes north)
  * 'I': [display symbol](http://wa8lmf.net/aprs/APRS_symbols.htm) - table to use (alternate symbol table "I")
  * '01429.51E': longitude (14 degrees, 29.51 minutes east)
  * '&': [display symbol](http://wa8lmf.net/aprs/APRS_symbols.htm) - position in table "I" to use (diamond symbol with overlay)
* '000/000': *optional* heading and speed ("000/000" indicates no data)
  * '000': heading (001 thru 360) in degrees ("000" = no data)
  * '000': ground speed in knots (heading "000" = no data)
* '/A=001689': *optional* altitude (1689 feet)
