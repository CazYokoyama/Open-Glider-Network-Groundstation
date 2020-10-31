As of version 0.2.6, a complete receiver status message looks as follows:

    LKHS>APRS,TCPIP*,qAC,GLIDERN2:>211635h v0.2.6.ARM CPU:0.2 RAM:777.7/972.2MB NTP:3.1ms/-3.8ppm 4.902V 0.583A +33.6C 14/16Acfts[1h] RF:+62-0.8ppm/+33.66dB/+19.4dB@10km[112619]/+25.0dB@10km[8/15]

---

The first group is standard APRS:

    LKHS>APRS,TCPIP*,qAC,GLIDERN2:>211635h

* 'LKHS>': origin
  * 'LKHS': receiver callsign
* 'APRS,TCPIP*,qAC,GLIDERN2:': source
  * 'APRS': software version (always "APRS")
  * 'TCPIP*': tunneling (always "TCPIP*")
  * 'qAC': [q construct](http://aprs-is.net/q.aspx) (not suited for beacon identifcation)
  * 'GLIDERN2': receiver callsign (APRS server)
* '/': either "/" (timestamp and position follow) or ">" (timestamp follows)
* '211635h': timestamp in 24h notation UTC (21 hours, 16 minutes, 35 seconds)

---

All subsequent groups are OGN specific receiver status details:

    v0.2.6.ARM CPU:0.2 RAM:777.7/972.2MB NTP:3.1ms/-3.8ppm 4.902V 0.583A +33.6C 14/16Acfts[1h] RF:+62-0.8ppm/+33.66dB/+19.4dB@10km[112619]/+25.0dB@10km[8/15]

* 'v0.2.6.ARM': *optional* software and hardware
  * '0.2.6': software version
  * 'ARM': *optional* hardware platform
* 'CPU:0.2': CPU
  * '0.2': load average
* 'RAM:777.7/972.2MB': memory in MB
  * '777.7': free
  * '972.2': total
* 'NTP:3.1ms/-3.8ppm': real-time clock
  * '3.1ms': offset from reference time
  * '-3.8ppm': correction (parts-per-million)
* '4.902V': *optional* board voltage
* '0.583A': *optional* board amperage
* '+33.6C': *optional* CPU temperature
* '14/16Acfts[1h]': *optional* senders (aircraft) received within the last hour
  * '14': visible senders received
  * '16': total senders received (the difference of 2 are invisible senders by setting either "no-track" on the device or "invisible" in the database)
* '+62-0.8ppm/+33.66dB': *optional* RTLSDR information
  * '+62': *optional* manual correction
  * '-0.8ppm': *optional* automatic correction based on GSM
  * '+33.66db': signal-to-noise ratio
* '+19.4dB@10km[112619]': *optional* quality of all senders (aircraft)
  * '+19.4dB@10km': average signal-to-noise ratio normalized to 10km distance
  * '112619': number of analyzed messages
* '+25.0dB@10km[8/15]': *optional* quality of good senders (aircraft) within the last 24 hours
  * '+25.0dB@10km': average signal-to-noise ratio normalized to 10km distance
  * '8': number of senders which are considered good and properly transmitting
  * '15': total number of senders (the difference of 7 are considered bad and not properly transmitting)
