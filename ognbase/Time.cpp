/*
 * Time.cpp
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

#include "SoC.h"
#include "Log.h"

#if defined(EXCLUDE_WIFI)
void Time_setup()
{}
#else

const char *ntpServer = "pool.ntp.org";
/* TODO: make them configurable */
const long gmtOffset_sec = (-8 * 60 * 60);
const int  daylightOffset_sec = (1 * 60 * 60);

void Time_setup()
{
  struct tm timeinfo;
  String msg;

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

#endif /* EXCLUDE_WIFI */
