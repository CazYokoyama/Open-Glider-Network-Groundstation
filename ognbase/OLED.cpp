/*
 * OLED.cpp
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

#include "SoC.h"

#include <Wire.h>

#include "OLED.h"
#include "EEPROM.h"
#include "RF.h"
#include "GNSS.h"
#include "Battery.h"
#include "Traffic.h"
#include "global.h"
#include "Web.h"
#include "version.h"
#include "logos.h"

SSD1306Wire display(SSD1306_OLED_I2C_ADDR, SDA, SCL, GEOMETRY_128_64, I2C_TWO, 400000);  // ADDRESS, SDA, SCL

bool display_init = false;
bool display_enabled = true;

int rssi      = 0;
int oled_site = 0;

enum
{
    OLED_PAGE_RADIO,
    OLED_PAGE_OTHER,
    OLED_PAGE_COUNT
};

const char* OLED_Protocol_ID[] = {
    [RF_PROTOCOL_LEGACY]    = "L",
    [RF_PROTOCOL_OGNTP]     = "O",
    [RF_PROTOCOL_P3I]       = "P",
    [RF_PROTOCOL_ADSB_1090] = "A",
    [RF_PROTOCOL_ADSB_UAT]  = "U",
    [RF_PROTOCOL_FANET]     = "F"
};

const char* ISO3166_CC[] = {
    [RF_BAND_AUTO] = "--",
    [RF_BAND_EU]   = "EU",
    [RF_BAND_US]   = "US",
    [RF_BAND_AU]   = "AU",
    [RF_BAND_NZ]   = "NZ",
    [RF_BAND_RU]   = "RU",
    [RF_BAND_CN]   = "CN",
    [RF_BAND_UK]   = "UK",
    [RF_BAND_IN]   = "IN"
};

byte OLED_setup()
{
    display_init = display.init();
    display.flipScreenVertically();
}

void OLED_write(char* text, short x, short y, bool clear)
{
    if (!display_enabled){return;}
      
    display.displayOn();
    if (clear)
        display.clear();
    display.drawString(x, y, text);
    display.display();
}

void OLED_disable()
{
  if (!display_enabled){return;}
  
  display.clear();
  display.displayOff();
  display_enabled = false;
}

void OLED_enable()
{
  display_enabled = true;
}

void OLED_draw_Bitmap(int16_t x, int16_t y, uint8_t bm, bool clear)
{
    if (!display_enabled){return;}
    
    display.displayOn();
    if (clear)
        display.clear();
    if (bm == 1)
        display.drawXbm(x, y, 50, 28, wifi_50_28);
    if (bm == 2)
        // fanet_export_75_75
        display.drawXbm(x, y, 100, 63, fanet_export_100_63);
    if (bm == 3) //network network_50_44
        display.drawXbm(x, y, 50, 44, network_50_44);
    if (bm == 100)
        // supporter_50_64
        display.drawXbm(x, y, 50, 64, supporter_50_64);
    display.display();
}

void OLED_info(bool ntp)
{
    if (!display_enabled){return;}
    
    char     buf[64];
    uint32_t disp_value;

    int     bars = 0;
    int32_t RSSI;

    RSSI = WiFi.RSSI();

    if (RSSI > -55)
        bars = 5;
    else if (RSSI < -55 & RSSI > -65)
        bars = 4;
    else if (RSSI < -65 & RSSI > -70)
        bars = 3;
    else if (RSSI < -70 & RSSI > -78)
        bars = 2;
    else if (RSSI < -78 & RSSI > -82)
        bars = 1;
    else
        bars = 0;


    if (display_init)
    {
        display.displayOff();
        display.clear();


        for (int b=0; b <= bars; b++)
            display.fillRect(100 + (b * 5), 65 - (b * 3), 3, b * 3);

        display.setFont(ArialMT_Plain_24);
        display.drawString(95, 0, "RX");
        display.setFont(ArialMT_Plain_16);
        snprintf(buf, sizeof(buf), "%d", rx_packets_counter);
        display.drawString(100, 28, buf);
        display.setFont(ArialMT_Plain_10);


        if (!oled_site)
        {
            snprintf(buf, sizeof(buf), "ID: %06X", ThisAircraft.addr);
            display.drawString(0, 0, buf);

            snprintf(buf, sizeof(buf), "SSID: %s", WiFi.SSID().c_str());
            display.drawString(0, 9, buf);

            snprintf(buf, sizeof(buf), "CS: %s", ogn_callsign);
            display.drawString(0, 18, buf);

            snprintf(buf, sizeof(buf), "Band: %s", ISO3166_CC[ogn_band]);
            display.drawString(0, 27, buf);

            //OLED_Protocol_ID[ThisAircraft.protocol]
            snprintf(buf, sizeof(buf), "Prot: %s", OLED_Protocol_ID[ogn_protocol_1]);
            display.drawString(0, 36, buf);

            snprintf(buf, sizeof(buf), "UTC: %02d:%02d", hour(now()), minute(now()));
            display.drawString(0, 45, buf);

            if (ntp)
                display.drawString(0, 54, "NTP: True");
            else
            {
                disp_value = gnss.satellites.value();
                itoa(disp_value, buf, 10);
                snprintf(buf, sizeof(buf), "GNSS: %s", buf);
                display.drawString(0, 54, buf);
            }
            /*
               disp_value = settings->range;
               itoa(disp_value, buf, 10);
               snprintf (buf, sizeof(buf), "Range: %s", buf);
               display.drawString(0, 63, buf);
             */
            oled_site = 1;
            display.display();
            display.displayOn();
            return;
        }
        if (oled_site == 1)
        {
            disp_value = RF_last_rssi;
            snprintf(buf, sizeof(buf), "RSSI: %d", disp_value);
            display.drawString(0, 0, buf);

            //ogndebug
            disp_value = ogn_debug;
            snprintf(buf, sizeof(buf), "Debug: %d", disp_value);
            display.drawString(0, 9, buf);

            //ogndebugp
            disp_value = ogn_debug;
            snprintf(buf, sizeof(buf), "DebugP: %d", disp_value);
            display.drawString(0, 18, buf);

            //bool ignore_stealth;
            disp_value = ogn_istealthbit;
            snprintf(buf, sizeof(buf), "IStealth: %d", disp_value);
            display.drawString(0, 27, buf);


            //bool ignore_no_track;
            disp_value = ogn_itrackbit;
            snprintf(buf, sizeof(buf), "ITrack: %d", disp_value);
            display.drawString(0, 36, buf);


            //zabbix_en
            disp_value = zabbix_enable;
            snprintf(buf, sizeof(buf), "Zabbix: %d", disp_value);
            display.drawString(0, 45, buf);

            //version
            snprintf(buf, sizeof(buf), "Version: %s", _VERSION);
            display.drawString(0, 54, buf);


            display.display();
            oled_site = 2;
            display.displayOn();
            return;
        }

        if (oled_site == 2)
        {
            display.clear();
            snprintf(buf, sizeof(buf), "SSID List");
            display.drawString(0, 0, buf);

            for (int i=0; i < 5; i++)
                if (ogn_ssid[i] != "")
                {
                    snprintf(buf, sizeof(buf), "%s", ogn_ssid[i].c_str());
                    display.drawString(0, (i + 1) * 9, buf);
                }

            for (int b=0; b <= bars; b++)
                display.fillRect(100 + (b * 5), 40 - (b * 6), 3, b * 6);

            snprintf(buf, sizeof(buf), "connected to %s", WiFi.SSID().c_str());
            display.drawString(0, 54, buf);

            display.display();
            display.displayOn();
            oled_site = 3;
            return;
        }

        if (oled_site == 3)
        {
            display.clear();
            snprintf(buf, sizeof(buf), "POSITION DATA");
            display.drawString(0, 0, buf);

            snprintf(buf, sizeof(buf), "LAT: %.4f", ThisAircraft.latitude);
            display.drawString(0, 9, buf);

            snprintf(buf, sizeof(buf), "LON:: %.4f", ThisAircraft.longitude);
            display.drawString(0, 18, buf);

            snprintf(buf, sizeof(buf), "ALT: %.2f", ThisAircraft.altitude);
            display.drawString(0, 27, buf);

            snprintf(buf, sizeof(buf), "GEOID: %.2f", ThisAircraft.geoid_separation);
            display.drawString(0, 36, buf);

            snprintf(buf, sizeof(buf), "GPS-FIX: %s", isValidFix() ? "true" : "false");
            display.drawString(0, 45, buf);                                                                

            display.display();
            if (beers_show)
                oled_site = 4;
            else
                oled_site = 0;
            display.displayOn();
            return;
        }  

        if (oled_site == 4)
        {
            String beer_supporters[] = {"guy", "jozef", "camille", "caz", "uwe"};
            display.clear();
            snprintf(buf, sizeof(buf), "Bier supporters");
            display.drawString(0, 0, buf);
            for (int i=0; i < sizeof(beer_supporters); i++) {
                snprintf(buf, sizeof(buf), "%s", beer_supporters[i]);
                display.drawString(0, (i + 1) * 9, buf);
            }

            snprintf(buf, sizeof(buf), "many thanks !!");
            display.drawString(0, 52, buf);

            OLED_draw_Bitmap(75, 0, 100, false);
            display.display();
            oled_site = 0;
            display.displayOn();
            return;
        }
    }
}
