/*
 * EEPROM.h
 * Copyright (C) 2019-2020 Linar Yusupov
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

#ifndef EEPROMHELPER_H
#define EEPROMHELPER_H

#include "SoC.h"

#if !defined(EXCLUDE_EEPROM)
#include <EEPROM.h>
#endif /* EXCLUDE_EEPROM */

#define SOFTRF_EEPROM_MAGIC 0xBABADEDA
#define SOFTRF_EEPROM_VERSION 0x0000005E

typedef struct Settings
{
    uint8_t mode;
    uint8_t rf_protocol;
    uint8_t band;
    uint8_t aircraft_type;
    uint8_t txpower;
    uint8_t volume;
    uint8_t led_num;
    uint8_t pointer;

    bool nmea_g : 1;
    bool nmea_p : 1;
    bool nmea_l : 1;
    bool nmea_s : 1;
    bool resvd1 : 1;
    uint8_t nmea_out : 3;

    uint8_t bluetooth : 3; /* ESP32 built-in Bluetooth */
    uint8_t alarm : 3;
    bool stealth : 1;
    bool no_track : 1;

    uint8_t gdl90 : 3;
    uint8_t d1090 : 3;
    uint8_t json : 2;

    uint8_t power_save;
    int8_t freq_corr;   /* +/-, kHz */
    int16_t range;      /*OGN Basestation max Range*/
    bool sxlna;         /*SX1276 agcref settings*/
    bool ogndebug;      /*debug on*/
    uint16_t ogndebugp; /*debug port*/
    bool ignore_stealth;
    bool ignore_no_track;
    uint8_t rf_protocol2;
    uint8_t sleep_mode;
    uint16_t sleep_after_rx_idle;
    uint16_t wake_up_timer;
    bool zabbix_en;
} settings_t;

typedef struct EEPROM_S
{
    uint32_t magic;
    uint32_t version;
    settings_t settings;
} eeprom_struct_t;

typedef union EEPROM_U
{
    eeprom_struct_t field;
    uint8_t raw[sizeof(eeprom_struct_t)];
} eeprom_t;

void EEPROM_setup(void);

void EEPROM_defaults(void);

void EEPROM_store(void);

extern settings_t* settings;

#endif /* EEPROMHELPER_H */
