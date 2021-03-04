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

SSD1306Wire display(SSD1306_OLED_I2C_ADDR, SDA, SCL);  // ADDRESS, SDA, SCL
bool display_init = false;

int rssi = 0;
int oled_site = 0;

enum
{
  OLED_PAGE_RADIO,
  OLED_PAGE_OTHER,
  OLED_PAGE_COUNT
};

const char *OLED_Protocol_ID[] = {
  [RF_PROTOCOL_LEGACY]    = "L",
  [RF_PROTOCOL_OGNTP]     = "O",
  [RF_PROTOCOL_P3I]       = "P",
  [RF_PROTOCOL_ADSB_1090] = "A",
  [RF_PROTOCOL_ADSB_UAT]  = "U",
  [RF_PROTOCOL_FANET]     = "F"
};

const char *ISO3166_CC[] = {
  [RF_BAND_AUTO] = "--",
  [RF_BAND_EU]   = "EU",
  [RF_BAND_US]   = "US",
  [RF_BAND_AU]   = "AU",
  [RF_BAND_NZ]   = "NZ",
  [RF_BAND_RU]   = "RU",
  [RF_BAND_CN]   = "CN",
  [RF_BAND_UK]   = "UK",
  [RF_BAND_IN]   = "IN",
  //[RF_BAND_IL]   = "IL",
  //[RF_BAND_KR]   = "KR"
};

byte OLED_setup() {
  display_init = display.init();
}

void OLED_write(char* text, short x, short y, bool clear)
{
  if(display_init){
    display.displayOn();
    if(clear){
      display.clear();
    }
    display.drawString(x, y, text);
    display.display();
  }

}

void OLED_clear()
{
  if(display_init){
   display.clear(); 
  }
}

void OLED_info(bool ntp)
{  
  char buf[20];
  uint32_t disp_value;
  
  
  if (display_init) {

    display.displayOff();  
    display.clear();

    display.setFont(ArialMT_Plain_24);
    display.drawString(85, 0, "RX");
    display.setFont(ArialMT_Plain_16);
    snprintf (buf, sizeof(buf), "%d", rx_packets_counter);
    display.drawString(85, 30, buf);
    display.setFont(ArialMT_Plain_10);

    if(!oled_site){
      snprintf (buf, sizeof(buf), "ID: %06X", ThisAircraft.addr);
      display.drawString(0, 0, buf);
      
      snprintf (buf, sizeof(buf), "SSID: %s", ogn_ssid);
      display.drawString(0, 9, buf);
      
      snprintf (buf, sizeof(buf), "CS: %s", ogn_callsign);
      display.drawString(0, 18, buf);
  
      snprintf (buf, sizeof(buf), "Band: %s", ISO3166_CC[settings->band]);
      display.drawString(0, 27, buf); 
      
      //OLED_Protocol_ID[ThisAircraft.protocol]
      snprintf (buf, sizeof(buf), "Prot: %s", OLED_Protocol_ID[ThisAircraft.protocol]);
      display.drawString(0, 36, buf); 
  
      snprintf (buf, sizeof(buf), "UTC: %02d:%02d", hour(now()), minute(now()));
      display.drawString(0, 45, buf);
  
      if(ntp){
        display.drawString(0, 54, "NTP: True");
      }
      else{
        disp_value = gnss.satellites.value();
        itoa(disp_value, buf, 10);
        snprintf (buf, sizeof(buf), "GNSS: %s", buf);
        display.drawString(0, 54, buf);
      }
  
       disp_value = settings->range;
       itoa(disp_value, buf, 10);
       snprintf (buf, sizeof(buf), "Range: %s", buf);
       display.drawString(0, 63, buf);
       oled_site = 1;
       display.display();
       display.displayOn(); 
       return;
    }
    if(oled_site == 1){
     
      disp_value = RF_last_rssi;
      snprintf (buf, sizeof(buf), "RSSI: %d", disp_value);
      display.drawString(0, 0, buf);

      //ogndebug
      disp_value = settings->ogndebug;
      snprintf (buf, sizeof(buf), "Debug: %d", disp_value);
      display.drawString(0, 9, buf);

      //ogndebugp
      disp_value = settings->ogndebugp;
      snprintf (buf, sizeof(buf), "DebugP: %d", disp_value);
      display.drawString(0, 18, buf);

      //bool ignore_stealth;
      disp_value = settings->ignore_stealth;
      snprintf (buf, sizeof(buf), "IStealth: %d", disp_value);
      display.drawString(0, 27, buf);


      //bool ignore_no_track;
      disp_value = settings->ignore_no_track;
      snprintf (buf, sizeof(buf), "ITrack: %d", disp_value);
      display.drawString(0, 36, buf);


      //zabbix_en
      disp_value = settings->zabbix_en;
      snprintf (buf, sizeof(buf), "Zabbix: %d", disp_value);
      display.drawString(0, 45, buf);

      //version
      snprintf (buf, sizeof(buf), "Version: %s", _VERSION);
      display.drawString(0, 54, buf);

      
      display.display();
      oled_site = 0;
      display.displayOn(); 
      return;
    }
  }
}

void OLED_status(){
  
}
