/*
   OGN.cpp
   Copyright (C) 2020 Manuel Rösel

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "APRS.h"
#include "PVALID.h"
#include "SoftRF.h"
#include "Battery.h"
#include "Traffic.h"
#include "RF.h"
#include "EEPROM.h"
#include "global.h"
#include "Log.h"
#include "OLED.h"
#include "global.h"


#include <TimeLib.h>

#define kKey 0x73e2

int MIN_SPEED = 0;

enum aprs_reg_state aprs_registred = APRS_NOT_REGISTERED;
bool aprs_connected   = false;
int  last_packet_time = 0; // seconds
int  ap_uptime        = 0;

#define seconds() (millis() / 1000)

static String zeroPadding(String data, int len)
{
    if (data.charAt(2) == '.')
        data.remove(len, data.length());

    if (data.charAt(1) == '.')
        data.remove(len - 1, data.length());
    String tmp = "";
    for (int i = data.length(); i < len; i++)
        tmp += "0";
    tmp += data;
    return tmp;
}

static String getWW(String data)
{
    String tmp = data;
    int    len = tmp.length();
    tmp.remove(0, len - 1);
    return tmp;
}

static float SnrCalc(float rssi)
{
    float noise = -108.0;
    return rssi - (noise);
}

static short AprsPasscode(const char* theCall)
{
    char  rootCall[10];
    char* p1 = rootCall;

    while ((*theCall != '-') && (*theCall != 0) && (p1 < rootCall + 9)) {
        *p1++ = toupper(*theCall++);
    }

    *p1 = 0;

    short hash = kKey;
    short i    = 0;
    short len  = strlen(rootCall);
    char* ptr  = rootCall;

    while (i < len) {
        hash ^= (*ptr++) << 8;
        hash ^= (*ptr++);
        i    += 2;
    }

    return hash & 0x7fff;
}

static bool OGN_APRS_Connect()
{
    if (SoC->WiFi_connect_TCP(ogn_server.c_str(), ogn_port))
        return true;
    else
        return false;
}

static bool OGN_APRS_DisConnect()
{
    if (!SoC->WiFi_disconnect_TCP())
        return true;
    return false;
}

static int OGN_APRS_check_reg(String* msg) // 0 = unverified // 1 = verified // -1 = wrong message
{
    int len = msg->length();

    if (msg->indexOf("verified") > -1)
        return 1;
    if (msg->indexOf("unverified") > -1)
        return 0;
    if (msg->indexOf("invalid") > -1)
        return 2;

    return -1;
}

bool OGN_APRS_check_Wifi()
{
    if (WiFi.status() != WL_CONNECTED && WiFi.getMode() == WIFI_STA)
    {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        //WiFi.begin(ogn_ssid_1.c_str(), ogn_wpass_1.c_str());
        delay(100);
    }
    if (WiFi.status() == WL_CONNECTED)
        return true;
    else
    {
        Serial.println("more then 10 minutes in AP mode reset");
        SoC->reset();
    }
    return false;
}

enum aprs_reg_state
OGN_APRS_check_messages()
{
    char*  RXbuffer  = (char *) malloc(512);
    int    recStatus = SoC->WiFi_receive_TCP(RXbuffer, 512);
    String msg;

    for (int i = 0; i < recStatus; i++)
        msg += RXbuffer[i];


    if (recStatus > 0)
    {
        if (OGN_APRS_check_reg(&msg) == 0)
            msg = "login unsuccessful";

        if (OGN_APRS_check_reg(&msg) == 1)
            msg = "login successful";

        last_packet_time = seconds();
    }

    Logger_send_udp(&msg);

    if (seconds() - last_packet_time > 60)
    {
        msg = "no packet since > 60 seconds...reconnecting";
        Logger_send_udp(&msg);
        SoC->WiFi_disconnect_TCP();
        aprs_registred = APRS_NOT_REGISTERED;
    }


    free(RXbuffer);
    return aprs_registred;
}

void OGN_APRS_Export()
{
    struct aprs_airc_packet APRS_AIRC;
    time_t                  this_moment = now();

    String symbol_table[16] = {"/", "/", "\\", "/", "\\", "\\", "/", "/", "\\", "J", "/", "/", "M", "/", "\\", "\\"}; // 0x79 -> aircraft type 1110 dec 14 & 0x51 -> aircraft type 4
    String symbol[16]       = {"z", "^", "^", "X", "", "^", "g", "g", "^", "^", "^", "O", "^", "\'", "", "n", };


    for (int i = 0; i < MAX_TRACKING_OBJECTS; i++)
        if (Container[i].addr && (this_moment - Container[i].timestamp) <= EXPORT_EXPIRATION_TIME && Container[i].distance < ogn_range * 1000)
        {

            if(!isPacketValid(Container[i].addr, Container[i].latitude, Container[i].longitude, Container[i].speed, Container[i].timestamp)){
              break;
            }
            
            if (Container[i].distance / 1000 > largest_range)
                largest_range = Container[i].distance / 1000;

            float LAT = fabs(Container[i].latitude);
            float LON = fabs(Container[i].longitude);


            APRS_AIRC.callsign = zeroPadding(String(Container[i].addr, HEX), 6);
            APRS_AIRC.callsign.toUpperCase();
            APRS_AIRC.rec_callsign = ogn_callsign;


            // TBD need to use Container[i].timestamp instead of hour(), minute(), second()
            // actually, need to make sure Container[i].timestamp is based on SlotTime not current time due slot-2 time extension
            //converting Container[i].timestamp to hour minutes seconds

            time_t receive_time = Container[i].timestamp;
            APRS_AIRC.timestamp = zeroPadding(String(hour(receive_time)), 2) + zeroPadding(String(minute(receive_time)), 2) + zeroPadding(String(second(receive_time)), 2) + "h";

            /*Latitude*/
            APRS_AIRC.lat_deg = String(int(LAT));
            APRS_AIRC.lat_min = zeroPadding(String((LAT - int(LAT)) * 60, 3), 5);

            /*Longitude*/
            APRS_AIRC.lon_deg = zeroPadding(String(int(LON)), 3);
            APRS_AIRC.lon_min = zeroPadding(String((LON - int(LON)) * 60, 3), 5);

            /*Altitude*/
            APRS_AIRC.alt = zeroPadding(String(int(Container[i].altitude * 3.28084)), 6);

            APRS_AIRC.heading      = zeroPadding(String(int(Container[i].course)), 3);
            APRS_AIRC.ground_speed = zeroPadding(String(int(Container[i].speed)), 3);


            APRS_AIRC.sender_details = zeroPadding(String(Container[i].aircraft_type << 2 | (Container[i].stealth << 7) | (Container[i].no_track << 6) | Container[i].addr_type, HEX), 2);

            APRS_AIRC.symbol_table = symbol_table[Container[i].aircraft_type];
            APRS_AIRC.symbol       = symbol[Container[i].aircraft_type];

            APRS_AIRC.snr = String(SnrCalc(Container[i].rssi), 1);


            String W_lat = String((LAT - int(LAT)) * 60, 3);
            String W_lon = String((LON - int(LON)) * 60, 3);


            APRS_AIRC.pos_precision = getWW(W_lat) + getWW(W_lon);

            if (Container[i].vs >= 0)
                APRS_AIRC.climbrate = "+" + zeroPadding(String(int(Container[i].vs)), 3);
            else
                APRS_AIRC.climbrate = zeroPadding(String(int(Container[i].vs)), 3);

            String AircraftPacket = "";

            if (Container[i].addr_type == 1)
                AircraftPacket = "ICA";
            if (Container[i].addr_type == 2)
                AircraftPacket += "FLR";
            if (Container[i].addr_type == 3)
                AircraftPacket += "OGN";
            if (Container[i].addr_type == 4)
                AircraftPacket += "P3I";
            if (Container[i].addr_type == 5)
                AircraftPacket += "FNT";
            if (Container[i].addr_type == 0 || Container[i].addr_type > 5)
                AircraftPacket += "RANDOM";

            AircraftPacket += APRS_AIRC.callsign;
            APRS_AIRC.sender_details.toUpperCase();

            AircraftPacket += ">APRS,qAS,";
            AircraftPacket += APRS_AIRC.rec_callsign;
            AircraftPacket += ":/";
            AircraftPacket += APRS_AIRC.timestamp;
            AircraftPacket += APRS_AIRC.lat_deg;
            AircraftPacket += APRS_AIRC.lat_min;
            if (Container[i].latitude < 0)
                AircraftPacket += "S";
            else
                AircraftPacket += "N";
            AircraftPacket += APRS_AIRC.symbol_table;
            AircraftPacket += APRS_AIRC.lon_deg;
            AircraftPacket += APRS_AIRC.lon_min;
            if (Container[i].longitude < 0)
                AircraftPacket += "W";
            else
                AircraftPacket += "E";
            AircraftPacket += APRS_AIRC.symbol;
            AircraftPacket += APRS_AIRC.heading;
            AircraftPacket += "/";
            AircraftPacket += APRS_AIRC.ground_speed;
            AircraftPacket += "/A=";
            AircraftPacket += APRS_AIRC.alt;
            AircraftPacket += " !W";
            AircraftPacket += APRS_AIRC.pos_precision;
            AircraftPacket += "! id";
            AircraftPacket += APRS_AIRC.sender_details;
            AircraftPacket += APRS_AIRC.callsign;
            AircraftPacket += " ";
            AircraftPacket += APRS_AIRC.climbrate;
            AircraftPacket += "fpm +0.0rot ";
            AircraftPacket += APRS_AIRC.snr;
            AircraftPacket += "dB 0e -0.0kHz";
            AircraftPacket += "\r\n";

            Logger_send_udp(&AircraftPacket);
            if (!Container[i].stealth && !Container[i].no_track || ogn_itrackbit && ogn_istealthbit)
                SoC->WiFi_transmit_TCP(AircraftPacket);
        }

    for (int i = 0; i < MAX_TRACKING_OBJECTS; i++) // cleaning up containers
        Container[i] = EmptyFO;
}

enum aprs_reg_state
OGN_APRS_Register(ufo_t* this_aircraft)
{
    if (OGN_APRS_Connect())
    {
        struct aprs_login_packet APRS_LOGIN;

        APRS_LOGIN.user    = String(this_aircraft->addr, HEX);
        APRS_LOGIN.pass    = String(AprsPasscode(APRS_LOGIN.user.c_str()));
        APRS_LOGIN.appname = "ESP32";
        APRS_LOGIN.version = SOFTRF_FIRMWARE_VERSION;

        String LoginPacket = "user ";
        LoginPacket += APRS_LOGIN.user;
        LoginPacket += " pass ";
        LoginPacket += APRS_LOGIN.pass;
        LoginPacket += " vers ";
        LoginPacket += APRS_LOGIN.appname;
        LoginPacket += " ";
        LoginPacket += APRS_LOGIN.version;
        LoginPacket += " ";
        LoginPacket += "m/";
        LoginPacket += String(ogn_range);
        LoginPacket += "\n";

        Logger_send_udp(&LoginPacket);
        SoC->WiFi_transmit_TCP(LoginPacket);

        aprs_registred = APRS_REGISTERED;
    }

    else
    {
        Serial.println("OGN connection failed");
        aprs_registred = APRS_NOT_REGISTERED;
        return APRS_FAILED;
    }

    if (aprs_registred == APRS_REGISTERED) {
        /* RUSSIA>APRS,TCPIP*,qAC,248280:/220757h626.56NI09353.92E&/A=000446 */

        struct  aprs_reg_packet APRS_REG;
        float                   LAT = fabs(this_aircraft->latitude);
        float                   LON = fabs(this_aircraft->longitude);

        APRS_REG.origin   = ogn_callsign;
        APRS_REG.callsign = String(this_aircraft->addr, HEX);
        APRS_REG.callsign.toUpperCase();
        APRS_REG.alt       = zeroPadding(String(int(this_aircraft->altitude * 3.28084)), 6);
        APRS_REG.timestamp = zeroPadding(String(hour()), 2) + zeroPadding(String(minute()), 2) + zeroPadding(String(second()), 2) + "h";

        APRS_REG.lat_deg = zeroPadding(String(int(LAT)), 2);
        APRS_REG.lat_min = zeroPadding(String((LAT - int(LAT)) * 60, 3), 5);

        APRS_REG.lon_deg = zeroPadding(String(int(LON)), 3);
        APRS_REG.lon_min = zeroPadding(String((LON - int(LON)) * 60, 3), 5);

        String RegisterPacket = "";
        RegisterPacket += APRS_REG.origin;
        RegisterPacket += ">APRS,TCPIP*,qAC,";
        RegisterPacket += APRS_REG.callsign;
        RegisterPacket += ":/";
        RegisterPacket += APRS_REG.timestamp;
        RegisterPacket += APRS_REG.lat_deg + APRS_REG.lat_min;

        if (this_aircraft->latitude < 0)
            RegisterPacket += "S";
        else
            RegisterPacket += "N";
        RegisterPacket += "I";
        RegisterPacket += APRS_REG.lon_deg + APRS_REG.lon_min;
        if (this_aircraft->longitude < 0)
            RegisterPacket += "W";
        else
            RegisterPacket += "E";
        RegisterPacket += "&/A=";
        RegisterPacket += APRS_REG.alt;
        RegisterPacket += "\r\n";

        SoC->WiFi_transmit_TCP(RegisterPacket);
        Logger_send_udp(&RegisterPacket);
    }
    return aprs_registred;
}

void OGN_APRS_KeepAlive()
{
    String KeepAlivePacket = "#keepalive\n";
    Logger_send_udp(&KeepAlivePacket);
    SoC->WiFi_transmit_TCP(KeepAlivePacket);
}

// LKHS>APRS,TCPIP*,qAC,GLIDERN2:>211635h v0.2.6.ARM CPU:0.2 RAM:777.7/972.2MB NTP:3.1ms/-3.8ppm 4.902V 0.583A +33.6C

void OGN_APRS_Status(ufo_t* this_aircraft)
{
    struct aprs_stat_packet APRS_STAT;
    APRS_STAT.origin   = ogn_callsign;
    APRS_STAT.callsign = String(this_aircraft->addr, HEX);
    APRS_STAT.callsign.toUpperCase();
    APRS_STAT.timestamp      = zeroPadding(String(hour()), 2) + zeroPadding(String(minute()), 2) + zeroPadding(String(second()), 2) + "h";

    /*issue17*/ /*v0.1.0.20-ESP32*/
    APRS_STAT.platform      = "v";
    APRS_STAT.platform      += SOFTRF_FIRMWARE_VERSION;
    APRS_STAT.platform      += "-ESP32";

    APRS_STAT.ram_max       = "320";
    APRS_STAT.ram_usage     = String(APRS_STAT.ram_max.toInt() - SoC->getFreeHeap()/1024);
    
    APRS_STAT.realtime_clock = String(0.0);
    APRS_STAT.board_voltage  = String(Battery_voltage()) + "V";

    // 14/16Acfts[1h]

    String StatusPacket = "";
    StatusPacket += APRS_STAT.origin;
    StatusPacket += ">APRS,TCPIP*,qAC,";
    StatusPacket += APRS_STAT.callsign;
    StatusPacket += ":>";
    StatusPacket += APRS_STAT.timestamp;
    StatusPacket += " ";
    StatusPacket += APRS_STAT.platform;
    StatusPacket += " RAM:";
    StatusPacket += APRS_STAT.ram_usage;
    StatusPacket += "/";
    StatusPacket += APRS_STAT.ram_max;
    StatusPacket += "KB";
    StatusPacket += " ";
    StatusPacket += APRS_STAT.board_voltage;
    //StatusPacket += " ";
    //StatusPacket += ThisAircraft.timestamp;
    StatusPacket += "\r\n";
    SoC->WiFi_transmit_TCP(StatusPacket);
    Logger_send_udp(&StatusPacket);
    return;
}
