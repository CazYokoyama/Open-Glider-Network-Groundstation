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


enum
{
  OLED_PAGE_RADIO,
  OLED_PAGE_OTHER,
  OLED_PAGE_COUNT
};

U8X8_OLED_I2C_BUS_TYPE u8x8_i2c(U8X8_PIN_NONE);

U8X8_OLED_I2C_BUS_TYPE *u8x8 = NULL;

static bool OLED_display_titles = false;
static uint32_t prev_tx_packets_counter = (uint32_t) -1;
static uint32_t prev_rx_packets_counter = (uint32_t) -1;
extern uint32_t tx_packets_counter, rx_packets_counter;

static uint32_t prev_acrfts_counter = (uint32_t) -1;
static uint32_t prev_sats_counter   = (uint32_t) -1;
static uint32_t prev_uptime_minutes = (uint32_t) -1;
static int32_t  prev_voltage        = (uint32_t) -1;
static int8_t   prev_fix            = (uint8_t)  -1;

uint8_t oled_site = 0;

unsigned long OLEDTimeMarker = 0;

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

const char OGN_text[]   = "OGN";
const char ID_text[]       = "ID";
const char PROTOCOL_text[] = "PROTOCOL";
const char RX_text[]       = "RX";
const char TX_text[]       = "TX";
const char ACFTS_text[]    = "ACFTS";
const char SATS_text[]     = "SATS";
const char FIX_text[]      = "FIX";
const char UPTIME_text[]   = "UPTIME";
const char BAT_text[]      = "BAT";

static const uint8_t Dot_Tile[] = { 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00 };

static int OLED_current_page = OLED_PAGE_RADIO;

byte OLED_setup() {

  Wire.begin();
  Wire.beginTransmission(SSD1306_OLED_I2C_ADDR);
  if (Wire.endTransmission() == 0){
    u8x8 = &u8x8_i2c;
  }
  if(u8x8){
    u8x8->begin();
    u8x8->setFont(u8x8_font_chroma48medium8_r);
    u8x8->drawString(0, 1, "Basestation");
    u8x8->drawString(0, 2, "booting...");
    
  }
}

void OLED_write(char* text, short x, short y, bool clear)
{
  if (u8x8) {
    if(clear){
      u8x8->clear();
    }
    u8x8->setFont(u8x8_font_chroma48medium8_r);
    u8x8->drawString(x, y, text);
  }
}

void OLED_clear()
{
  for( int c = 1; c < u8x8->getCols(); c++ ){
    OLED_bar(c, 1);
    OLED_bar(c-1, 0);
  }
  u8x8->clear();
}

void OLED_bar(uint8_t c, uint8_t is_inverse)
{ 
  uint8_t r;
  u8x8->setInverseFont(is_inverse);
  for( r = 0; r < u8x8->getRows(); r++ )
  {
    u8x8->setCursor(c, r);
    u8x8->print(" ");
  }
}

void OLED_fini(int reason)
{

}

void OLED_info(bool ntp)
{
  char buf[16];
  uint32_t disp_value;
  
  OLED_clear();
  
  snprintf (buf, sizeof(buf), "%06X", ThisAircraft.addr);
  if (u8x8) {

    if(!oled_site){
      snprintf (buf, sizeof(buf), "%06X", ThisAircraft.addr);
      u8x8->drawString(0, 0, "ID:");
      u8x8->drawString(4, 0, buf);
      
      snprintf (buf, sizeof(buf), "SSID: %s", ogn_ssid);
      u8x8->drawString(0, 1, buf);
      
      snprintf (buf, sizeof(buf), "CS: %s", ogn_callsign);
      u8x8->drawString(0, 2, buf);
  
      snprintf (buf, sizeof(buf), "Band: %s", ISO3166_CC[settings->band]);
      u8x8->drawString(0, 3, buf); 
      
      //OLED_Protocol_ID[ThisAircraft.protocol]
      snprintf (buf, sizeof(buf), "Prot: %s", OLED_Protocol_ID[ThisAircraft.protocol]);
      u8x8->drawString(0, 4, buf); 
  
      snprintf (buf, sizeof(buf), "UTC: %02d:%02d", hour(now()), minute(now()));
      u8x8->drawString(0, 5, buf);
  
      if(ntp){
        u8x8->drawString(0, 6, "NTP: True");
      }
      else{
        disp_value = gnss.satellites.value();
        itoa(disp_value, buf, 10);
        u8x8->drawString(0, 6, "GNSS:");
        u8x8->drawString(7, 6, buf);
      }
  
       disp_value = settings->range;
       itoa(disp_value, buf, 10);
       u8x8->drawString(0, 7, "Range:");
       u8x8->drawString(7, 7, buf);
       oled_site = 1;
    }
    else{
      snprintf (buf, sizeof(buf), "%06X", ThisAircraft.addr);
      u8x8->drawString(0, 0, "ID:");
      u8x8->drawString(4, 0, buf);
      
      disp_value = RF_last_rssi;
      itoa(disp_value, buf, 10);
      u8x8->drawString(0, 1, "RSSI:");
      u8x8->drawString(7, 1, buf);

      //ogndebug
      disp_value = settings->ogndebug;
      itoa(disp_value, buf, 10);
      u8x8->drawString(0, 2, "DEBUG:");
      u8x8->drawString(7, 2, buf);

      //ogndebugp
      disp_value = settings->ogndebugp;
      itoa(disp_value, buf, 10);
      u8x8->drawString(0, 3, "DEBP:");
      u8x8->drawString(7, 3, buf);

      //bool ignore_stealth;
      disp_value = settings->ignore_stealth;
      itoa(disp_value, buf, 10);
      u8x8->drawString(0, 4, "ISTEALTH:");
      u8x8->drawString(10, 4, buf);

      //bool ignore_no_track;
      disp_value = settings->ignore_no_track;
      itoa(disp_value, buf, 10);
      u8x8->drawString(0, 5, "ITRACK:");
      u8x8->drawString(8, 5, buf);

      //zabbix_en
      disp_value = settings->zabbix_en;
      itoa(disp_value, buf, 10);
      u8x8->drawString(0, 6, "ZABBIX:");
      u8x8->drawString(8, 6, buf);


      oled_site = 0;
    }
  }
}

void OLED_status(){
  
}


void OLED_Next_Page()
{
  if (u8x8) {
    OLED_current_page = (OLED_current_page + 1) % OLED_PAGE_COUNT;
    OLED_display_titles = false;
  }
}
