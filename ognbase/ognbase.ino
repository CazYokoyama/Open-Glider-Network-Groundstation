 

/*
   ognbase(.ino) firmware
   Copyright (C) 2020 Manuel Roesel

   Author: Manuel Roesel, lmanuel.roesel@ros-it.ch

   Web: http://github.com/lyusupov/SoftRF

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
#include "global.h"

#include <TimeLib.h>

#if defined(ENABLE_AHRS)
#include "AHRS.h"
#endif /* ENABLE_AHRS */

#if LOGGER_IS_ENABLED
#include "Log.h"
#endif /* LOGGER_IS_ENABLED */

#define DEBUG 0
#define DEBUG_TIMING 0

#define APRS_KEEPALIVE_TIME 240
#define APRS_REGISTER_REC 300
#define APRS_STATUS_REC 100
#define APRS_EXPORT_AIRCRAFT 5
#define APRS_PROTO_SWITCH 2

#define RESET_TIMER 60
#define TIME_TO_SLEEP  60

#define seconds() (millis()/1000)

#define isTimeToExport() (millis() - ExportTimeMarker > 1000)

#define TimeToExportOGN() (seconds() - ExportTimeOGN > APRS_EXPORT_AIRCRAFT)
#define TimeToRegisterOGN() (seconds() - ExportTimeRegisterOGN > APRS_REGISTER_REC)
#define TimeToKeepAliveOGN() (seconds() - ExportTimeKeepAliveOGN > APRS_KEEPALIVE_TIME)
#define TimeToStatusOGN() (seconds() - ExportTimeStatusOGN > APRS_KEEPALIVE_TIME)
#define TimeToCheckWifi() (seconds() - ExportTimeReset > RESET_TIMER)
#define TimeToswitchProto() (seconds() - ExportTimeSwitch > APRS_PROTO_SWITCH)

//testing
#define TimeToSleep() (seconds() - ExportTimeSleep > settings->sleep_after_rx_idle)


ufo_t ThisAircraft;
bool groundstation = false;
bool ground_registred = false;
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
unsigned long ExportTimeRegisterOGN = 0;
unsigned long ExportTimeKeepAliveOGN = 0;
unsigned long ExportTimeStatusOGN = 0;
unsigned long ExportTimeReset = 0;
unsigned long ExportTimeSwitch = 0;
unsigned long ExportTimeSleep = 0;

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

void setup()
{

   rst_info *resetInfo;

  hw_info.soc = SoC_setup(); // Has to be very first procedure in the execution order

  resetInfo = (rst_info *) SoC->getResetInfoPtr();

  Serial.begin(SERIAL_OUT_BR, SERIAL_OUT_BITS);

#if defined(USBD_USE_CDC) && !defined(DISABLE_GENERIC_SERIALUSB)
  /* Let host's USB and console drivers to warm-up */
  delay(2000);
#endif

#if LOGGER_IS_ENABLED
  Logger_setup();
#endif /* LOGGER_IS_ENABLED */

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

#if defined(ENABLE_AHRS)
  hw_info.ahrs = AHRS_setup();
#endif /* ENABLE_AHRS */

#if !defined(EXCLUDE_MAVLINK)
  if (settings->mode == SOFTRF_MODE_UAV) {
    Serial.begin(57600);
    MAVLink_setup();
    ThisAircraft.aircraft_type = AIRCRAFT_TYPE_UAV;
  }  else
#endif /* EXCLUDE_MAVLINK */
  {
    hw_info.gnss = GNSS_setup();
    ThisAircraft.aircraft_type = settings->aircraft_type;
  }
  ThisAircraft.protocol = settings->rf_protocol;
  ThisAircraft.stealth  = settings->stealth;
  ThisAircraft.no_track = settings->no_track;

  Battery_setup();
  Traffic_setup();

  SoC->swSer_enableRx(false);

  WiFi_setup();

  if (SoC->Bluetooth) {
    SoC->Bluetooth->setup();
  }

  OTA_setup();
  NMEA_setup();

#if defined(ENABLE_TTN)
  TTN_setup();
#endif

  delay(2000);

  Web_setup();
  Time_setup();

  SoC->WDT_setup();
}

void loop()
{
  // Do common RF stuff first
  RF_loop();

  ground();
  
  // Handle DNS
  WiFi_loop();

  // Handle Web
  Web_loop();

  // Handle OTA update.
  OTA_loop();

#if LOGGER_IS_ENABLED
  Logger_loop();
#endif /* LOGGER_IS_ENABLED */

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

  bool success;

  GNSS_loop();

  if (!groundstation) {
    RF_Transmit(RF_Encode(&ThisAircraft), true);
    groundstation = true;
  }


  success = RF_Receive();
  if (success && isValidFix()){
    ParseData();
    ExportTimeSleep = seconds();
  }

  ThisAircraft.timestamp = now();

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
  }

  if ( (TimeToRegisterOGN() && isValidFix()) || (!ground_registred && isValidFix()) )
  {
    ground_registred = OGN_APRS_Register(&ThisAircraft);
    ExportTimeRegisterOGN = seconds();
  }

  if (TimeToExportOGN() && ground_registred)
  {
    OGN_APRS_Export();
    ClearExpired();
    ExportTimeOGN = seconds();
  }

  if (TimeToKeepAliveOGN() && ground_registred)
  {
    OGN_APRS_KeepAlive();
    ExportTimeKeepAliveOGN = seconds();
  }

  if (TimeToStatusOGN() && ground_registred && isValidFix())
  {
    OGN_APRS_Status(&ThisAircraft);
    ExportTimeStatusOGN = seconds();
  }

  //sleep mode #define TimeToSleep() (seconds() - ExportTimeSleep > APRS_SLEPP_AFTER_TIME)

  
  if ( TimeToSleep() && settings->sleep_mode )
  {
    if(settings->wake_up_timer > 0){
      esp_sleep_enable_timer_wakeup(settings->wake_up_timer*1000000);
    }
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,1);
    if (settings->sleep_mode == 1){
      GNSS_sleep(); 
    }
    esp_deep_sleep_start();
  }

  /*if(TimeToCheckWifi()){
    if (!Wifi_connected()){
      Serial.println("Reconnecting to WiFi...");
      //SoC->reset();
    }
    }*/

  // Handle Air Connect
  NMEA_loop();
  ClearExpired();

}

void watchout()
{
  bool success;

  success = RF_Receive();

  if (success) {
    size_t rx_size = RF_Payload_Size(settings->rf_protocol);
    rx_size = rx_size > sizeof(fo.raw) ? sizeof(fo.raw) : rx_size;

    memset(fo.raw, 0, sizeof(fo.raw));
    memcpy(fo.raw, RxBuffer, rx_size);

    if (settings->nmea_p) {
      StdOut.print(F("$PSRFI,"));
      StdOut.print((unsigned long) now());    StdOut.print(F(","));
      StdOut.print(Bin2Hex(fo.raw, rx_size)); StdOut.print(F(","));
      StdOut.println(RF_last_rssi);
    }
  }

}

#if !defined(EXCLUDE_TEST_MODE)

unsigned int pos_ndx = 0;
unsigned long TxPosUpdMarker = 0;

void txrx_test()
{
  bool success = false;
#if DEBUG_TIMING
  unsigned long baro_start_ms, baro_end_ms;
  unsigned long tx_start_ms, tx_end_ms, rx_start_ms, rx_end_ms;
  unsigned long parse_start_ms, parse_end_ms, led_start_ms, led_end_ms;
  unsigned long export_start_ms, export_end_ms;
  unsigned long oled_start_ms, oled_end_ms;
#endif
  ThisAircraft.timestamp = now();

  if (TxPosUpdMarker == 0 || (millis() - TxPosUpdMarker) > 4000 ) {
    ThisAircraft.latitude =  pgm_read_float( &txrx_test_positions[pos_ndx][0]);
    ThisAircraft.longitude =  pgm_read_float( &txrx_test_positions[pos_ndx][1]);
    pos_ndx = (pos_ndx + 1) % TXRX_TEST_NUM_POSITIONS;
    TxPosUpdMarker = millis();
  }
  ThisAircraft.altitude = TXRX_TEST_ALTITUDE;
  ThisAircraft.course = TXRX_TEST_COURSE;
  ThisAircraft.speed = TXRX_TEST_SPEED;
  ThisAircraft.vs = TXRX_TEST_VS;


#if defined(ENABLE_AHRS)
  AHRS_loop();
#endif /* ENABLE_AHRS */

#if DEBUG_TIMING
  tx_start_ms = millis();
#endif
  RF_Transmit(RF_Encode(&ThisAircraft), true);
#if DEBUG_TIMING
  tx_end_ms = millis();
  rx_start_ms = millis();
#endif
  success = RF_Receive();
#if DEBUG_TIMING
  rx_end_ms = millis();
#endif

#if DEBUG_TIMING
  parse_start_ms = millis();
#endif
  if (success) ParseData();
#if DEBUG_TIMING
  parse_end_ms = millis();
#endif

#if defined(ENABLE_TTN)
  TTN_loop();
#endif

  Traffic_loop();


#if DEBUG_TIMING
  export_start_ms = millis();
#endif
  if (isTimeToExport()) {
#if defined(USE_NMEALIB)
    NMEA_Position();
#endif
    NMEA_Export();
    GDL90_Export();
    D1090_Export();
    ExportTimeMarker = millis();
  }
#if DEBUG_TIMING
  export_end_ms = millis();
#endif

#if DEBUG_TIMING
  oled_start_ms = millis();
#endif
  //  SoC->Display_loop();
#if DEBUG_TIMING
  oled_end_ms = millis();
#endif

#if DEBUG_TIMING
  if (baro_start_ms - baro_end_ms) {
    Serial.print(F("Baro start: "));
    Serial.print(baro_start_ms);
    Serial.print(F(" Baro stop: "));
    Serial.println(baro_end_ms);
  }
  if (tx_end_ms - tx_start_ms) {
    Serial.print(F("TX start: "));
    Serial.print(tx_start_ms);
    Serial.print(F(" TX stop: "));
    Serial.println(tx_end_ms);
  }
  if (rx_end_ms - rx_start_ms) {
    Serial.print(F("RX start: "));
    Serial.print(rx_start_ms);
    Serial.print(F(" RX stop: "));
    Serial.println(rx_end_ms);
  }
  if (parse_end_ms - parse_start_ms) {
    Serial.print(F("Parse start: "));
    Serial.print(parse_start_ms);
    Serial.print(F(" Parse stop: "));
    Serial.println(parse_end_ms);
  }
  if (led_end_ms - led_start_ms) {
    Serial.print(F("LED start: "));
    Serial.print(led_start_ms);
    Serial.print(F(" LED stop: "));
    Serial.println(led_end_ms);
  }
  if (export_end_ms - export_start_ms) {
    Serial.print(F("Export start: "));
    Serial.print(export_start_ms);
    Serial.print(F(" Export stop: "));
    Serial.println(export_end_ms);
  }
  if (oled_end_ms - oled_start_ms) {
    Serial.print(F("OLED start: "));
    Serial.print(oled_start_ms);
    Serial.print(F(" OLED stop: "));
    Serial.println(oled_end_ms);
  }
#endif

  // Handle Air Connect
  NMEA_loop();

  ClearExpired();
}

#endif /* EXCLUDE_TEST_MODE */
