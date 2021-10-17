/*
 * PVALID.h
 * Copyright (C) 2020 Manuel Rösel
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


#ifndef PVALIDHELPER_H
#define PVALIDHELPER_H

bool isPacketValid(uint32_t, double, double, double, time_t);

static int distance(double, double, double, double);
static double deg2rad(double);
static double rad2deg(double);
static void cleanUpPacket(uint8_t);
static void shiftPackets(void);
static bool appendPacket(uint32_t, double, double, time_t, uint8_t cnt);
static int calcMaxDistance(double, time_t);

typedef struct aircrafts
{
    time_t timestamp;
    uint32_t addr;
    float latitude;
    float longitude;
    float altitude;
    float course;
    uint8_t pkt_counter;
} aircrafts_t;

#endif /* PVALIDHELPER_H */
