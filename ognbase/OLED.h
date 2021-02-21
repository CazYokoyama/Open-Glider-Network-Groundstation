/*
 * OLEDHelper.h
 * Copyright (C) 2019-2021 Linar Yusupov
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

#ifndef OLEDHELPER_H
#define OLEDHELPER_H

#include <U8x8lib.h>

#define SSD1306_OLED_I2C_ADDR   0x3C
#define U8X8_OLED_I2C_BUS_TYPE  U8X8_SSD1306_128X64_NONAME_2ND_HW_I2C

byte OLED_setup(void);
void OLED_write(char*, short, short, bool);
void OLED_fini(int);
void OLED_clear(void);
void OLED_bar(uint8_t, uint8_t);
void OLED_info(bool);

extern const char *ISO3166_CC[];
extern const char SoftRF_text[];
extern const char ID_text[];
extern const char PROTOCOL_text[];
extern const char RX_text[];
extern const char TX_text[];

#endif /* OLEDHELPER_H */
