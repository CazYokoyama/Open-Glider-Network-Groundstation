/*
 * BaroHelper.h
 * Copyright (C) 2018-2020 Linar Yusupov
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

#ifndef BAROHELPER_H
#define BAROHELPER_H

#define BMP280_ADDRESS_ALT  0x76 /* GY-91, SA0 is NC */

enum
{
  BARO_MODULE_NONE,
  BARO_MODULE_BMP180,
  BARO_MODULE_BMP280,
  BARO_MODULE_MPL3115A2
};

typedef struct barochip_ops_struct {
  byte type;
  const char name[10];
  bool (*probe)();
  void (*setup)();
  float (*altitude)(float);
} barochip_ops_t;

extern barochip_ops_t *baro_chip;

bool Baro_probe(void);
byte Baro_setup(void);
void Baro_loop(void);

#endif /* BAROHELPER_H */
