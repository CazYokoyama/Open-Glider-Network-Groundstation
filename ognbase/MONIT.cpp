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
#include "GNSS.h"
#include "Log.h"

#define hours() (millis() / 3600000)


bool config_status;

void MONIT_send_trap()
{
    int timeout = 5;
    if (zabbix_enable)
    {
        ZabbixSender zs;
        String       jsonPayload;
        String       msg;
    
        msg = "sending zabbix trap to ";
        msg += zabbix_server.c_str();
        msg += ":";
        msg += zabbix_port;
        Logger_send_udp(&msg);        

        jsonPayload = zs.createPayload(zabbix_key.c_str(), Battery_voltage(), RF_last_rssi, int(hours()), gnss.satellites.value(), ThisAircraft.timestamp, largest_range);

        String zb_msg = zs.createMessage(jsonPayload);

        Logger_send_udp(&jsonPayload);

        //byte buffer[zb_msg.length() + 1];
        //zb_msg.getBytes(buffer, zb_msg.length() + 1);

        SoC->WiFi_connect_TCP2(zabbix_server.c_str(), zabbix_port);
        SoC->WiFi_transmit_TCP2(zb_msg);
        SoC->WiFi_disconnect_TCP2();
        
        
        //SoC->WiFi_transmit_UDP(zabbix_server.c_str(), zabbix_port, buffer, zb_msg.length() + 1);
        
    }
}
