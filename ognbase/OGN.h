/*
 * OGN.h
 * Copyright (C) 2020 Manuel Roesel
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "SoC.h"



#ifndef OGNHELPER_H
#define OGNHELPER_H

//LKHS>APRS,TCPIP*,qAC,GLIDERN2:/211635h4902.45NI01429.51E&000/000/A=001689

struct aprs_reg_packet{
  String origin;
  String callsign;
  String timestamp;
  String lat_deg;
  String lat_min;
  String lon_deg;
  String lon_min;
  String alt;
};

struct aprs_login_packet{ // user OGN123456 pass 12345 vers DemoApp 0.0.0 filter m/10
  String user;
  String pass;
  String appname;
  String version;
};

// LKHS>APRS,TCPIP*,qAC,GLIDERN2:>211635h v0.2.6.ARM CPU:0.2 RAM:777.7/972.2MB NTP:3.1ms/-3.8ppm 4.902V 0.583A +33.6C
struct aprs_stat_packet{
  String origin;
  String callsign;
  String timestamp;
  String platform;
  String cpu_usage;
  String ram_max;
  String ram_usage;
  String realtime_clock;
  String board_voltage;
  String board_amperage;
  String cpu_temperature;
};

// FLRDF0A52>APRS,qAS,LSTB:/220132h4658.70N/00707.72Ez090/054/A=001424 !W37! id06DF0A52 +020fpm +0.0rot 55.2dB 0e -6.2kHz gps4x6 s6.01 h03 rDDACC4 +5.0dBm hearD7EA hearDA95
struct aprs_airc_packet{
  String callsign;
  String rec_callsign;
  String timestamp;
  String lat_deg;
  String lat_min;
  String lon_deg;
  String lon_min;
  String alt;
  String heading;
  String ground_speed;
  String pos_precision;
  String symbol_table;
  String symbol;
  String climbrate;
  String sender_details;
  String snr;
};

enum
{
  OGN_OFF,
  OGN_ON,
};

static bool OGN_APRS_Connect();
static bool OGN_APRS_DisConnect();


void OGN_APRS_Export();
bool OGN_APRS_Register(ufo_t *this_aircraft);
void OGN_APRS_KeepAlive();
void OGN_APRS_Status(ufo_t *this_aircraft);

extern String ogn_callsign;
#endif /* OGNHELPER_H */
