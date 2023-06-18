/*
   ognbase(.ino) firmware
   Copyright (C) 2020 Manuel Roesel

   Author: Manuel Roesel, manuel.roesel@ros-it.ch

   Web: https://github.com/roema/Open-Glider-Network-Groundstation

   Credits:
     Arduino core for ESP8266 is developed/supported by ESP8266 Community (support-esp8266@esp8266.com)
     AVR/Arduino nRF905 Library/Driver is developed by Zak Kemble, contact@zakkemble.co.uk
     flarm_decode is developed by Stanislaw Pusep, http://github.com/creaktive
     Arduino Time Library is developed by Paul Stoffregen, http://github.com/PaulStoffregen
     "Aircraft" and MAVLink Libraries are developed by Andy Little
     TinyGPS++ and PString Libraries are developed by Mikal Hart
     Adafruit NeoPixel Library is developed by Phil Burgess, Michael Miller and others
     TrueRandom Library is developed by Peter Knight
     IBM LMIC and Semtech Basic MAC frameworks for Arduino are maintained by Matthijs Kooijman
     ESP8266FtpServer is developed by David Paiva
     Lib_crc is developed by Lammert Bies
     OGN library is developed by Pawel Jalocha
     NMEA library is developed by Timur Sinitsyn, Tobias Simon, Ferry Huberts
     ADS-B encoder C++ library is developed by yangbinbin (yangbinbin_ytu@163.com)
     Arduino Core for ESP32 is developed by Hristo Gochkov
     ESP32 BT SPP library is developed by Evandro Copercini
     Adafruit BMP085 library is developed by Limor Fried and Ladyada
     Adafruit BMP280 library is developed by Kevin Townsend
     Adafruit MPL3115A2 library is developed by Limor Fried and Kevin Townsend
     U8g2 monochrome LCD, OLED and eInk library is developed by Oliver Kraus
     NeoPixelBus library is developed by Michael Miller
     jQuery library is developed by JS Foundation
     EGM96 data is developed by XCSoar team
     BCM2835 C library is developed by Mike McCauley
     SimpleNetwork library is developed by Dario Longobardi
     ArduinoJson library is developed by Benoit Blanchon
     Flashrom library is part of the flashrom.org project
     Arduino Core for TI CC13X0 and CC13X2 is developed by Robert Wessels
     EasyLink library is developed by Robert Wessels and Tony Cave
     Dump978 library is developed by Oliver Jowett
     FEC library is developed by Phil Karn
     AXP202X library is developed by Lewis He
     Arduino Core for STM32 is developed by Frederic Pillon
     TFT library is developed by Bodmer
     STM32duino Low Power and RTC libraries are developed by Wi6Labs
     Basic MAC library is developed by Michael Kuyper
     LowPowerLab SPIFlash library is maintained by Felix Rusu
     Arduino core for ASR650x is developed by Aaron Lee (HelTec Automation)

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

#include "OTA.h"
#include "Time.h"

#include "GNSS.h"
#include "RF.h"

#include "EEPROM.h"
#include "Battery.h"
//#include "NMEA.h"
#include "SoC.h"
#include "WiFi.h"
#include "Web.h"

#include "TTN.h"
#include "Traffic.h"

#include "APRS.h"
#include "RSM.h"
#include "PNET.h"
#include "MONIT.h"
#include "OLED.h"
#include "Log.h"
#include "global.h"
#include "version.h"
#include "config.h"

#include <rom/rtc.h>

#include <TimeLib.h>

#define TBEAM
//#define TTGO

#if defined(ENABLE_AHRS)
#include "AHRS.h"
#endif /* ENABLE_AHRS */

#if LOGGER_IS_ENABLED
#include "Log.h"
#endif /* LOGGER_IS_ENABLED */

#define DEBUG 0
#define DEBUG_TIMING 0

#define seconds() (millis()/1000)

#define isTimeToExport() (millis() - ExportTimeMarker > 1000)

#define APRS_EXPORT_AIRCRAFT 5
#define TimeToExportOGN() (seconds() - ExportTimeOGN >= APRS_EXPORT_AIRCRAFT)

#define APRS_REGISTER_REC 300
#define TimeToRegisterOGN() (now() - ExportTimeRegisterOGN >= APRS_REGISTER_REC)

#define APRS_KEEPALIVE_TIME 240
#define TimeToKeepAliveOGN() (seconds() - ExportTimeKeepAliveOGN >= APRS_KEEPALIVE_TIME)

#define APRS_CHECK_KEEPALIVE_TIME APRS_REGISTER_REC
#define TimeToCheckKeepAliveOGN() (now() - ExportTimeCheckKeepAliveOGN >= APRS_CHECK_KEEPALIVE_TIME)

#define APRS_CHECK_WIFI_TIME 600
#define TimeToCheckWifi() (seconds() - ExportTimeCheckWifi >= APRS_CHECK_WIFI_TIME)

#define APRS_STATUS_REC (5 * 60) /* tramsit status packet every 5 min */
#define TimeToStatusOGN() (now() - ExportTimeStatusOGN >= APRS_STATUS_REC)

#define APRS_PROTO_SWITCH 2
#define TimeToswitchProto() (seconds() - ExportTimeSwitch >= APRS_PROTO_SWITCH)

#define TIME_TO_REFRESH_WEB 30
#define TimeToRefreshWeb() (seconds() - ExportTimeWebRefresh >= TIME_TO_REFRESH_WEB)

//testing
#define TimeToSleep() (seconds() - ExportTimeSleep >= ogn_rxidle)

//testing
#define TIME_TO_DIS_WIFI  60
#define TimeToDisWifi() (seconds() - ExportTimeDisWifi >= TIME_TO_DIS_WIFI)

//testing
#define TimeToDisableOled() (seconds() - ExportTimeOledDisable >= oled_disable)

//time reregister if failed
#define TIME_TO_REREG 30
#define TimeToReRegisterOGN() (seconds() - ExportTimeReRegister >= TIME_TO_REREG)

/*Testing FANET service messages*/
#define TIME_TO_EXPORT_FANET_SERVICE 40 /*every 40 sec 10 for testing*/
#define TimeToExportFanetService() (seconds() - ExportTimeFanetService >= TIME_TO_EXPORT_FANET_SERVICE)

#define BUTTON 38

/*
   TODO: make them configurable
*/
#define LOCALTIME_DIFF (-7)
#define NIGHT_START 21
#define NIGHT_END 6
#define isNight(hour) (hour < NIGHT_END || NIGHT_START <= hour)

inline short
convert_localtime(short hour, short time_diff) {
  hour += time_diff;
  if (hour < 0)
    hour += 24;
  else if (24 <= hour)
    hour -= 24;
  return hour;
}

ufo_t ThisAircraft;
bool groundstation = false;
RTC_DATA_ATTR enum aprs_reg_state ground_registred = APRS_NOT_REGISTERED;
bool fanet_transmitter = false;
bool time_synced = false;
int proto_in_use = 0;

hardware_info_t hw_info = {
  .model    = DEFAULT_SOFTRF_MODEL,
  .revision = 0,
  .soc      = SOC_NONE,
  .rf       = RF_IC_NONE,
  .gnss     = GNSS_MODULE_NONE,
};

unsigned long LEDTimeMarker = 0;
unsigned long ExportTimeMarker = 0;

unsigned long ExportTimeOGN = 0;
RTC_DATA_ATTR unsigned long ExportTimeRegisterOGN = 0;
unsigned long ExportTimeKeepAliveOGN = 0;
RTC_DATA_ATTR unsigned long ExportTimeStatusOGN = 0;
unsigned long ExportTimeSwitch = 0;
unsigned long ExportTimeSleep = 0;
unsigned long ExportTimeDisWifi = 0;
unsigned long ExportTimeWebRefresh = 0;
unsigned long ExportTimeFanetService = 0;
RTC_DATA_ATTR unsigned long ExportTimeCheckKeepAliveOGN = 0;
unsigned long ExportTimeCheckWifi = 0;
unsigned long ExportTimeOledDisable = 0;
unsigned long ExportTimeReRegister = 0;

/*set ground position only once*/
bool position_is_set = false;
/*
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}
*/

void setup()
{

  rst_info *resetInfo;

  hw_info.soc = SoC_setup(); // Has to be very first procedure in the execution order

  resetInfo = (rst_info *) SoC->getResetInfoPtr();

  Serial.begin(SERIAL_OUT_BR, SERIAL_OUT_BITS);

  Serial.println();
  Serial.print(F(SOFTRF_IDENT));
  Serial.print(SoC->name);
  Serial.print(F(" FW.REV: " SOFTRF_FIRMWARE_VERSION " DEV.ID: "));
  Serial.println(String(SoC->getChipId(), HEX));
  Serial.println(F("Copyright (C) 2020 Manuel Roesel. All rights reserved."));
  Serial.flush();

  if (resetInfo) {
    Serial.println(""); Serial.print(F("Reset reason: ")); Serial.println(resetInfo->reason);
    }
  Serial.println(SoC->getResetReason());
  Serial.print(F("Free heap size: ")); Serial.println(SoC->getFreeHeap());
  Serial.println(SoC->getResetInfo()); Serial.println("");

  EEPROM_setup();
  OLED_setup();

  WiFi_setup();

  SoC->Button_setup();

  ThisAircraft.addr = SoC->getChipId() & 0x00FFFFFF;

  hw_info.rf = RF_setup();

  if (hw_info.model    == SOFTRF_MODEL_PRIME_MK2 &&
      hw_info.revision == 2                      &&
      RF_SX12XX_RST_is_connected)
  {
    hw_info.revision = 5;
  }

  delay(100);

#if defined(TBEAM)
  hw_info.gnss = GNSS_setup();
#endif
  
  ThisAircraft.aircraft_type = settings->aircraft_type;
  Battery_setup();
  Traffic_setup();

  SoC->swSer_enableRx(false);

  OTA_setup();
  delay(2000);

  Web_setup(&ThisAircraft);
  Time_setup();
  SoC->WDT_setup();

  if(private_network || remotelogs_enable){
    aes_init();
  }

#if defined(TBEAM)
  pinMode(BUTTON, INPUT);
#endif  
}

void loop()
{
  // Do common RF stuff first
  RF_loop();

  ground();
  
  // Handle DNS
  WiFi_loop();

  // Handle Web
  /*MANU add timer to refresh values*/
  if(TimeToRefreshWeb()){
    Web_loop();
    ExportTimeWebRefresh = seconds();
  }

  // Handle OTA update.
  OTA_loop();

  SoC->loop();

  Battery_loop();

  SoC->Button_loop();

  yield();
}

void shutdown(const char *msg)
{
  SoC->WDT_fini();

  SoC->swSer_enableRx(false);


  Web_fini();

  WiFi_fini();
  
#if defined(TBEAM)
  GNSS_fini();
#endif

  RF_Shutdown();

  SoC->Button_fini();

  SoC_fini();
}

void
ogn_goto_sleep(int how_long, bool gnss_sleep)
{
    String msg;

    msg = "entering sleep mode for ";
    msg += String(how_long);
    msg += " seconds - good night";
    Logger_send_udp(&msg);

    esp_sleep_enable_timer_wakeup(how_long*1000000LL);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,1);
    OLED_disable();
#if defined(TBEAM)
    if (gnss_sleep)
      GNSS_sleep();
#endif

    if (!ognrelay_enable)
        SoC->WiFi_disconnect_TCP();
    esp_deep_sleep_start();
}

void ground()
{

   char *disp;
   String msg;
   char buf[32];
   short sys_hour = -1;
   int how_long;

   if (!groundstation) {
    groundstation = true;

    msg = "good morning, startup esp32 groundstation ";
    msg += "version ";
    msg += String(_VERSION);
    msg += " after ";
    msg += String(SoC->getResetInfo());
    Logger_send_udp(&msg);

    msg = "current time ";
    msg += now();
    Logger_send_udp(&msg);
  }

  if((WiFi.getMode() == WIFI_AP) && !ognrelay_enable){
    OLED_write("Setup mode..", 0, 9, true);
    snprintf (buf, sizeof(buf), "SSID: %s", host_name.c_str());
    OLED_write(buf, 0, 18, false);
    snprintf (buf, sizeof(buf), "ip: %s", "192.168.1.1");
    OLED_write(buf, 0, 27, false);
    snprintf (buf, sizeof(buf), "reboot in %d seconds", 300 - seconds());
    OLED_write(buf, 0, 36, false);
    snprintf (buf, sizeof(buf), "Version: %s ", _VERSION);
    OLED_write(buf, 0, 45, false);    
    delay(1000);
    if(300 < seconds()){
      SoC->reset();
      }
    }

  sys_hour = hour(); /* in UTC */
  sys_hour = convert_localtime(sys_hour, LOCALTIME_DIFF);
  if (ogn_sleepmode != OSM_DISABLED && isNight(sys_hour)) {
    how_long = NIGHT_END - sys_hour;
    if (how_long < 0)
        how_long += 24;
    else if (how_long >= 24)
        how_long -= 24;
    ogn_goto_sleep(how_long * 60 * 60, true);
  }

  if (RF_Receive()) {
    ExportTimeSleep = seconds();
    if (isValidFix() || position_is_set)
      ParseData();
  }

  if (ogn_lat != 0 && ogn_lon != 0 && !position_is_set) {
    ThisAircraft.latitude = ogn_lat;
    ThisAircraft.longitude = ogn_lon;
    ThisAircraft.altitude = ogn_alt;
    ThisAircraft.course = 0;
    ThisAircraft.speed = 0;
    ThisAircraft.hdop = 0;
    ThisAircraft.geoid_separation = ogn_geoid_separation;

#if defined(TBEAM)
    GNSS_sleep();
#endif

    msg = "found position data LAT: ";
    msg += ogn_lat;
    msg += " LON: ";
    msg += ogn_lon;
    msg += " ALT: ";
    msg += ogn_alt;
    Logger_send_udp(&msg);

   position_is_set = true;
  }

  if (isValidFix() && ogn_lat == 0 && ogn_lon == 0 && !position_is_set) {
    
    ThisAircraft.latitude = gnss.location.lat();
    ThisAircraft.longitude = gnss.location.lng();
    ThisAircraft.altitude = gnss.altitude.meters();
    ThisAircraft.course = gnss.course.deg();
    ThisAircraft.speed = gnss.speed.knots();
    ThisAircraft.hdop = (uint16_t) gnss.hdop.value();
    ThisAircraft.geoid_separation = gnss.separation.meters();

    ogn_lat = gnss.location.lat();
    ogn_lon = gnss.location.lng();
    ogn_alt = gnss.altitude.meters();
    ogn_geoid_separation = gnss.separation.meters();

    msg = "GPS fix LAT: ";
    msg += gnss.location.lat();
    msg += " LON: ";
    msg += gnss.location.lng();
    msg += " ALT: ";
    msg += gnss.altitude.meters();
    Logger_send_udp(&msg);    

    position_is_set = true;    

  }

  if(!position_is_set){
    OLED_write("no position data found", 0, 18, true);
    delay(1000);
    OLED_write("waiting for GPS fix", 0, 18, true);
    delay(1000);
  }

#if defined(TBEAM)
  GNSS_loop();
#endif

  ThisAircraft.timestamp = now();

  //only as basestation


  if (ognrelay_enable) {
    if (TimeToCheckWifi()) {
      OLED_draw_Bitmap(39, 5, 3 , true);
      OLED_write("check connections..", 15, 45, false);
      if(OGN_APRS_check_Wifi())
        OLED_write("success", 35, 54, false);
      else
        OLED_write("error", 35, 54, false);
      ExportTimeCheckWifi = seconds();
    }
    if (TimeToExportOGN()) {
      OLED_info(position_is_set);
      ExportTimeOGN = seconds();
    }
  } else {
    if (position_is_set && WiFi.getMode() != WIFI_AP &&
        (TimeToRegisterOGN() || ground_registred == APRS_NOT_REGISTERED)) {
      ground_registred = OGN_APRS_Register(&ThisAircraft);
      ExportTimeRegisterOGN = now();
    }
  
    if (ground_registred == APRS_FAILED) {
      OLED_write("server registration failed!", 0, 18, true);
      OLED_write("please check json file!", 0, 27, false);
      snprintf (buf, sizeof(buf), "%s : %d", ogn_server.c_str(), ogn_port);
      OLED_write(buf, 0, 36, false);
      ground_registred = APRS_WILL_REGISTER;
    }
  
    if (ground_registred == APRS_WILL_REGISTER) {
      OGN_APRS_check_Wifi();
      ExportTimeReRegister = seconds();
      while(TimeToReRegisterOGN()){
        os_runstep();
      }
      ground_registred = APRS_NOT_REGISTERED;
    }
  
    if (ground_registred == APRS_REGISTERED) {
      if (TimeToExportOGN()) {
        if (new_protocol_enable && testmode_enable)
          RSM_ExportAircraftPosition();
        OGN_APRS_Export();
        OLED_info(position_is_set);
        ExportTimeOGN = seconds();
      }
  
      if (TimeToKeepAliveOGN()) {
        disp = "keepalive OGN...";
        OLED_write(disp, 0, 24, true);
      
        OGN_APRS_KeepAlive();
        ExportTimeKeepAliveOGN = seconds();
      }
  
      if (TimeToStatusOGN() && position_is_set) {
        disp = "status OGN...";
        OLED_write(disp, 0, 24, true);
      
        OGN_APRS_Status(&ThisAircraft);
  
        msg = "Version: ";
        msg += String(_VERSION);
        msg += " Power: ";
        msg += String(SoC->Battery_voltage());
        msg += String(" Uptime: ");
        msg += String(millis() / 3600000);
        msg += String(" GNSS: ");
        msg += String(gnss.satellites.value());
        Logger_send_udp(&msg);
        ExportTimeStatusOGN = now();
      }
  
      if (TimeToCheckKeepAliveOGN()) {
        ground_registred = OGN_APRS_check_messages();
        ExportTimeCheckKeepAliveOGN = now();
        MONIT_send_trap();
      }
    } /* ground_registred == APRS_REGISTERED */
  } /* ognrelay_enable */
  
  if (ogn_sleepmode != OSM_DISABLED && TimeToSleep())
    ogn_goto_sleep(ogn_wakeuptimer, ogn_sleepmode == OSM_FULL);

  if (ground_registred == APRS_REGISTERED && TimeToExportFanetService()) {

    
    OLED_draw_Bitmap(14, 0, 2 , true);
      
    if( fanet_transmitter ){
      RSM_receiver();
    }
    else{
      fanet_transmitter = RSM_Setup(ogn_debugIP+1);
    }
    ExportTimeFanetService = seconds();
  }

  // Handle Air Connect
#if defined(TBEAM) 
  ClearExpired();
#endif   


  if( TimeToDisWifi() && ognrelay_enable ){
    WiFi.mode(WIFI_OFF);
    ExportTimeDisWifi = seconds();
    //Serial.print("disabling Wifi");
  }

  if( TimeToDisableOled() ){
    if (oled_disable > 0){
     OLED_disable(); 
    }
  }
  
#if defined(TBEAM) 
  if (!digitalRead(BUTTON)){
    while(!digitalRead(BUTTON)){delay(100);}
    OLED_enable();
    ExportTimeOledDisable = seconds();
  }
#endif 
}
