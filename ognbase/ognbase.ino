
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
#include "MAVLink.h"
#include "GDL90.h"
#include "NMEA.h"
#include "D1090.h"
#include "SoC.h"
#include "WiFi.h"
#include "Web.h"

#include "TTN.h"
#include "Traffic.h"

#include "APRS.h"
#include "RSM.h"
#include "MONIT.h"
#include "OLED.h"
#include "Log.h"
#include "global.h"
#include "version.h"
#include "config.h"

#include <rom/rtc.h>

#include <TimeLib.h>

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
#define TimeToRegisterOGN() (seconds() - ExportTimeRegisterOGN >= APRS_REGISTER_REC)

#define APRS_KEEPALIVE_TIME 240
#define TimeToKeepAliveOGN() (seconds() - ExportTimeKeepAliveOGN >= APRS_KEEPALIVE_TIME)

#define APRS_CHECK_KEEPALIVE_TIME 20
#define TimeToCheckKeepAliveOGN() (seconds() - ExportTimeCheckKeepAliveOGN >= APRS_CHECK_KEEPALIVE_TIME)

#define APRS_CHECK_WIFI_TIME 600
#define TimeToCheckWifi() (seconds() - ExportTimeCheckWifi >= APRS_CHECK_WIFI_TIME)

#define APRS_STATUS_REC 1800
#define TimeToStatusOGN() (seconds() - ExportTimeStatusOGN >= APRS_STATUS_REC)

#define APRS_PROTO_SWITCH 2
#define TimeToswitchProto() (seconds() - ExportTimeSwitch >= APRS_PROTO_SWITCH)

#define TIME_TO_REFRESH_WEB 30
#define TimeToRefreshWeb() (seconds() - ExportTimeWebRefresh >= TIME_TO_REFRESH_WEB)

//testing
#define TIME_TO_SLEEP  60
#define TimeToSleep() (seconds() - ExportTimeSleep >= ogn_rxidle)

//testing
#define TimeToDisableOled() (seconds() - ExportTimeOledDisable >= oled_disable)

/*Testing FANET service messages*/
#define TIME_TO_EXPORT_FANET_SERVICE 40 /*every 40 sec 10 for testing*/
#define TimeToExportFanetService() (seconds() - ExportTimeFanetService >= TIME_TO_EXPORT_FANET_SERVICE)

#define BUTTON 38


ufo_t ThisAircraft;
bool groundstation = false;
int ground_registred = 0;
bool fanet_transmitter = false;
bool time_synced = false;
bool ntp_in_use = false;
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
unsigned long ExportTimeRegisterOGN = 0;
unsigned long ExportTimeKeepAliveOGN = 0;
unsigned long ExportTimeStatusOGN = 0;
unsigned long ExportTimeSwitch = 0;
unsigned long ExportTimeSleep = 0;
unsigned long ExportTimeWebRefresh = 0;
unsigned long ExportTimeFanetService = 0;
unsigned long ExportTimeCheckKeepAliveOGN = 0;
unsigned long ExportTimeCheckWifi = 0;
unsigned long ExportTimeOledDisable = 0;

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

  hw_info.gnss = GNSS_setup();
  ThisAircraft.aircraft_type = settings->aircraft_type;
  Battery_setup();
  Traffic_setup();

  SoC->swSer_enableRx(false);

  OTA_setup();
  NMEA_setup();

  delay(2000);

  Web_setup(&ThisAircraft);
  Time_setup();
  SoC->WDT_setup();

  /*T-BEAM only*/
  //pinMode(BUTTON, INPUT);
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

  NMEA_fini();

  Web_fini();

  WiFi_fini();

  if (settings->mode != SOFTRF_MODE_UAV) {
    GNSS_fini();
  }


  RF_Shutdown();

  SoC->Button_fini();

  SoC_fini();
}

void ground()
{

   char *disp;
   bool success;
   String msg;
   char buf[32];

   if (!groundstation) {

   
    RF_Transmit(RF_Encode(&ThisAircraft), true);
    groundstation = true;

 
    msg = "good morning, startup esp32 groundstation ";
    msg += "version ";
    msg += String(_VERSION);
    msg += " after ";
    msg += String(SoC->getResetInfo());
    Logger_send_udp(&msg);
  }

  if((WiFi.getMode() == WIFI_AP)){
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

  
  GNSS_loop(groundstation);

  success = RF_Receive();
  if (success && isValidFix() || success && ntp_in_use){
    
    msg += String("receiving data..");
    Logger_send_udp(&msg);
    
    ParseData();
    ExportTimeSleep = seconds();
  }

  if (isValidFix() && ogn_lat == 0 && ogn_lon == 0 && ogn_alt == 0) {
    
    ThisAircraft.latitude = gnss.location.lat();
    ThisAircraft.longitude = gnss.location.lng();
    ThisAircraft.altitude = gnss.altitude.meters();
    ThisAircraft.course = gnss.course.deg();
    ThisAircraft.speed = gnss.speed.knots();
    ThisAircraft.hdop = (uint16_t) gnss.hdop.value();
    ThisAircraft.geoid_separation = gnss.separation.meters();

  }
  if (ogn_lat != 0 && ogn_lon != 0 && ogn_alt != 0) {
    ThisAircraft.latitude = ogn_lat;
    ThisAircraft.longitude = ogn_lon;
    ThisAircraft.altitude = ogn_alt;
    ThisAircraft.course = 0;
    ThisAircraft.speed = 0;
    ThisAircraft.hdop = 0;
    ThisAircraft.geoid_separation = ogn_geoid_separation;
    
    if(!ntp_in_use){
      //GNSS_sleep(); error in function, do not activate
      }
    ntp_in_use = true;
  }

  ThisAircraft.timestamp = now();

  if ((TimeToRegisterOGN() && (isValidFix() || ntp_in_use)) && WiFi.getMode() != WIFI_AP || (ground_registred == 0 ) && (isValidFix() || ntp_in_use) && WiFi.getMode() != WIFI_AP)
  {  
    ground_registred = OGN_APRS_Register(&ThisAircraft);
    ExportTimeRegisterOGN = seconds();
  }

  if(ground_registred ==  -1){
    OLED_write("server registration failed!", 0, 18, true);
    OLED_write("please check json file!", 0, 27, false);
    snprintf (buf, sizeof(buf), "%s : %d", ogn_server.c_str(), ogn_port);
    OLED_write(buf, 0, 36, false);
    ground_registred = -2; 
  }

  if(ground_registred == -2){
    //
  }

  if (TimeToExportOGN() && ground_registred == 1)
  {
    OGN_APRS_Export();
    ClearExpired();
    OLED_info(ntp_in_use);
    ExportTimeOGN = seconds();
  }

  if (TimeToKeepAliveOGN() && ground_registred == 1)
  {
    disp = "keepalive OGN...";
    OLED_write(disp, 0, 24, true);
    
    OGN_APRS_KeepAlive();
    ExportTimeKeepAliveOGN = seconds();
  }

  if (TimeToStatusOGN() && ground_registred == 1 && (isValidFix() || ntp_in_use))
  {

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
    ExportTimeStatusOGN = seconds();
  }

  
  if ( TimeToSleep() && ogn_sleepmode )
  {
    msg = "entering sleep mode for ";
    msg += String(ogn_wakeuptimer); 
    msg += " seconds - good night";
    Logger_send_udp(&msg);
    
    esp_sleep_enable_timer_wakeup(ogn_wakeuptimer*1000000LL);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,1);
    if (ogn_sleepmode == 1){
      GNSS_sleep(); 
    }
    ground_registred = 0;
    SoC->WiFi_disconnect_TCP();
    esp_deep_sleep_start();
  }

  if(ground_registred == 1 && TimeToExportFanetService()){

    
    OLED_draw_Bitmap(14, 0, 2 , true);
      
    if( fanet_transmitter ){
      RSM_receiver();
    }
    else{
      fanet_transmitter = RSM_Setup(ogn_debugport+1);
    }
    ExportTimeFanetService = seconds();
    msg = "current system time  ";
    msg += String(now());
    Logger_send_udp(&msg);
  }

  if(TimeToCheckKeepAliveOGN() && ground_registred == 1){
    ground_registred = OGN_APRS_check_messages();
    ExportTimeCheckKeepAliveOGN = seconds();
    MONIT_send_trap();
  }
  
  if( TimeToCheckWifi() ){
    OLED_draw_Bitmap(39, 5, 3 , true);
    OLED_write("check connections..", 15, 45, false);
    if(OGN_APRS_check_Wifi()){
      OLED_write("success", 35, 54, false);
    }
    else{
      OLED_write("error", 35, 54, false);
    }
    ExportTimeCheckWifi = seconds();
  }

  // Handle Air Connect
  NMEA_loop();
  ClearExpired();

  if( TimeToDisableOled() ){
    if (oled_disable > 0){
     OLED_disable(); 
    }
  }
  /*
  if (!digitalRead(BUTTON)){
    while(!digitalRead(BUTTON)){delay(100);}
    OLED_enable();
    ExportTimeOledDisable = seconds();
  }
  */
}
