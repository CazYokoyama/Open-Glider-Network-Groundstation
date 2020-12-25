/*
 * OGN.cpp
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

#include "MONIT.h"
#include "SoftRF.h"
#include "Battery.h"
#include "RF.h"
#include "EEPROM.h"
#include "zabbixSender.h"
#include "global.h"

#define hours() (millis() / 3600000)


WiFiClient zclient;

String zabbix_server;
int    zabbix_port;
String zabbix_host;
bool   config_status;

static bool loadConfig()
{
    int line = 0;

    if (!SPIFFS.begin(true))
        return false;

    File configFile = SPIFFS.open("/zabbix.txt", "r");
    if (!configFile)
        return false;

    while (configFile.available() && line < 7)
    {
        if (line == 0)
            zabbix_server = configFile.readStringUntil('\n').c_str();
        if (line == 1)
            zabbix_port = configFile.readStringUntil('\n').toInt();
        if (line == 2)
            zabbix_host = configFile.readStringUntil('\n').c_str();
        line++;
    }

    configFile.close();

    if (line < 2)
        return false;

    zabbix_server.trim();
    return true;
}

static bool MONIT_setup()
{
    if (!config_status)
        config_status = loadConfig();
    return config_status;
}

void MONIT_send_trap()
{
    if (MONIT_setup())
    {
        ZabbixSender zs;
        String       jsonPayload;

        jsonPayload = zs.createPayload(zabbix_host.c_str(), Battery_voltage(), RF_last_rssi, int(hours()), gnss.satellites.value(), ThisAircraft.timestamp, largest_range);

        String msg = zs.createMessage(jsonPayload);

        if (zclient.connect(zabbix_server.c_str(), zabbix_port))
            zclient.print(msg);
        zclient.stop();
    }
}
