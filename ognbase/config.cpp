/*
   CONFIG.cpp
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


#include "SoftRF.h"
#include "EEPROM.h"
#include "global.h"
#include "config.h"
#include "OLED.h"

//START SD
#include <SD.h>
#include <FS.h>
#include "OLED.h"

// SD card
#define SD_SCK  14
#define SD_MISO 2
#define SD_MOSI 15
#define SD_CS   13
//END SD

#define ARDUINOJSON_USE_DOUBLE 0
#include <ArduinoJson.h>



//wifi
String ogn_ssid[5];
String ogn_wpass[5];
int    ssid_index = 0;

//aprs default values
String   ogn_callsign    = "callsign";
String   ogn_server      = "aprs.glidernet.org";
uint16_t ogn_port        = 14580;
uint8_t  ogn_band        = 1;
uint8_t  ogn_protocol_1  = 0;
uint8_t  ogn_protocol_2  = 1;
bool     ogn_debug       = false;
uint16_t ogn_debugport   = 12000;
bool     ogn_itrackbit   = false;
bool     ogn_istealthbit = false;
bool     ogn_sleepmode   = false;
uint16_t ogn_rxidle      = 3600;
uint16_t ogn_wakeuptimer = 3600;
uint8_t  ogn_range       = 100;

//position
float   ogn_lat              = 0;
float   ogn_lon              = 0;
int     ogn_alt              = 0;
int16_t ogn_geoid_separation = 0;
uint8_t largest_range        = 0;

//fanet service
bool fanet_enable = false;

//zabbix
bool      zabbix_enable = false;
String    zabbix_server = "127.0.0.1";
uint16_t  zabbix_port   = 10051;
String    zabbix_key    = "ogn_base";

//supporters
bool beers_show = false;

//remote logs
bool      remotelogs_enable = false;
String    remotelogs_server = "127.0.0.1";
uint16_t  remotelogs_port = 12000;

//oled 
unsigned long  oled_disable = 0;

//tesmode for new functions
bool  testmode_enable = false;

//private network
bool  private_network = false;

//new protocol
bool new_protocol_enable;
String new_protocol_server;
uint32_t new_protocol_port;

//#define TTGO

#ifdef TTGO

void performUpdate(Stream &updateSource, size_t updateSize) {

    char buf[32];

    snprintf(buf, sizeof(buf), "update firmware");
    OLED_write(buf, 0, 15, true);
  
   if (Update.begin(updateSize)) {      
      size_t written = Update.writeStream(updateSource);
      if (written == updateSize) {
         Serial.println("Written : " + String(written) + " successfully");
      }
      else {
        snprintf(buf, sizeof(buf), "update error");
        OLED_write(buf, 0, 16, true);         
        }
      if (Update.end()) {
         if (Update.isFinished()) {
            snprintf(buf, sizeof(buf), "update success");
            OLED_write(buf, 0, 24, false); 
            delay(1000);               
         }
         else {
            snprintf(buf, sizeof(buf), "update error");
            OLED_write(buf, 0, 16, true);               
         }
      }
      else {
         snprintf(buf, sizeof(buf), "update error");
         OLED_write(buf, 0, 16, true);         
      }

   }
   else
   {
    snprintf(buf, sizeof(buf), "update error");
    OLED_write(buf, 0, 16, true);   
   }
}

#endif

bool OGN_read_config(void)
{
    const size_t        capacity = 2048;
    DynamicJsonDocument baseConfig(capacity);
    JsonObject          obj;
    File configFile;

    const char *config_files[6] = { "/config.json", 
                                    "/index.html", 
                                    "/update.html", 
                                    "/style.css",
                                    "/key.bin",
                                    "/iv.bin"};    

    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return false;
    }

#ifdef TTGO    
    /*READ SD Card - config.json & firmware update*/

    char buf[32];

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI); // TTGO V2
    if(!SD.begin(SD_CS)){
      Serial.println("Card Mount Failed");
    }
    else{
      uint8_t cardType = SD.cardType();
      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
      Serial.printf("SD Card Size: %lluMB\n", cardSize);
  
      delay(500);

      for(size_t i=0;i<6;i++){
        File file = SD.open(config_files[i], FILE_READ);       
        File tempFile = SPIFFS.open(config_files[i], FILE_WRITE);
  
        size_t config_size = file.size();
        if (config_size > 0){
          Serial.print("update config from sd ");
          Serial.println(config_files[i]);
          snprintf(buf, sizeof(buf), "found config on sdcard");
          OLED_write(buf, 0, 16, true); 
          snprintf(buf, sizeof(buf), config_files[i]);
          OLED_write(buf, 0, 25, true);           
          snprintf(buf, sizeof(buf), "updating");
          OLED_write(buf, 0, 34, false);         
          while(file.available()){
            tempFile.write(file.read());
          }
        }
        delay(1000);
        tempFile.close();
        file.close();
      }
  
      //checking firmware file
      File updateBin = SD.open("/firmware/ognbase.bin");
      size_t updateSize = updateBin.size();
        
      if (updateSize > 0) {
        Serial.println("Try to start update");
        performUpdate(updateBin, updateSize);
        updateBin.close();
        SD.remove("/firmware/ognbase.bin");
        snprintf(buf, sizeof(buf), "reboot");
        OLED_write(buf, 0, 33, false);      
        ESP.restart();         
      }
      else {
         Serial.println("No firmware found");
      }
    }
    SPI.end();
#endif    


    if (SPIFFS.exists(config_files[0])){
      configFile = SPIFFS.open(config_files[0]);
      if (!configFile)
      {
          Serial.println(F("Failed to open config.json."));
          return false;
      }      
    }
    else{
      Serial.println(F("config.json doesnt exist, please upload config.json"));
      return(false);
    }

    DeserializationError error = deserializeJson(baseConfig, configFile);

    if (error)
    {
        Serial.println(F("Failed to parse json file, using default configuration"));
        Serial.println(error.f_str());
        configFile.close();
        return false;
    }
    else
    {
        obj = baseConfig.as<JsonObject>();
        configFile.close();
    }

    if (!obj.containsKey(F("wifi"))){
        //Serial.println("no wifi confgiuration found, return setup mode");
        configFile.close();        
        return false;
    }
    else
    {
        //Serial.println(F("found wifi config!"));
        if (1)
            for (int i=0; i < 5; i++) {
                ogn_ssid[i]  = obj["wifi"]["ssid"][i].as<String>();
                ogn_wpass[i] = obj["wifi"]["pass"][i].as<String>();
            }
    }

    if (obj.containsKey(F("coordinates")))
    {
        //Serial.println(F("found coordinates config!"));
        if (1)
        {
            ogn_lat              = obj["coordinates"]["lat"];
            ogn_lon              = obj["coordinates"]["lon"];
            ogn_alt              = obj["coordinates"]["alt"];
            ogn_geoid_separation = obj["coordinates"]["geoidsep"];
        }
    }

    if (obj.containsKey(F("aprs")))
    {
        //Serial.println(F("found aprs config!"));
        if (1)
        {
            ogn_callsign = obj["aprs"]["callsign"].as<String>();
            ogn_server   = obj["aprs"]["server"].as<String>();
            ogn_port     = obj["aprs"]["port"];

            ogn_band        = obj["aprs"]["band"];
            ogn_protocol_1  = obj["aprs"]["protocol_1"];
            ogn_protocol_2  = obj["aprs"]["protocol_2"];
            ogn_debug       = obj["aprs"]["debug"];
            ogn_debugport   = obj["aprs"]["debugport"];
            ogn_itrackbit   = obj["aprs"]["itrackbit"];
            ogn_istealthbit = obj["aprs"]["istealthbit"];
            ogn_sleepmode   = obj["aprs"]["sleepmode"];
            ogn_rxidle      = obj["aprs"]["rxidle"];
            ogn_wakeuptimer = obj["aprs"]["wakeuptimer"];
            ogn_range       = obj["aprs"]["range"];
        }
    }


    if (obj.containsKey(F("zabbix")))
    {
        //Serial.println(F("found zabbix config!"));
        if (1)
        {
            zabbix_enable = obj["zabbix"]["enable"];
            zabbix_server = obj["zabbix"]["server"].as<String>();
            ;
            zabbix_port = obj["zabbix"]["port"];
            zabbix_key  = obj["zabbix"]["key"].as<String>();
            ;
        }
    }

    if (obj.containsKey(F("remotelogs")))
    {
        //Serial.println(F("found remotelogs config!"));
        if (1)
        {
            remotelogs_enable = obj["remotelogs"]["enable"];
            remotelogs_server = obj["remotelogs"]["server"].as<String>();
            remotelogs_port = obj["remotelogs"]["port"];
        }
    }

    if (obj.containsKey(F("testmode")))
    {
        //Serial.println(F("found testmode config!"));
        if (1)
        {
            testmode_enable = obj["testmode"]["enable"];
        }
    }      

    if (obj.containsKey(F("fanetservice")))
    {
        //Serial.println(F("found fanetservice config!"));
        if (1)
            fanet_enable = obj["fanetservice"]["enable"];
    }

    /*oled_disable*/
    if (obj.containsKey(F("oled")))
    {
        //Serial.println(F("found oled config!"));
        if (1)
            oled_disable = obj["oled"]["disable"];
    }    
    
    /*private network*/
    if (obj.containsKey(F("private")))
    {
        //Serial.println(F("found private network config!"));
        if (1)
            private_network = obj["private"]["enable"];
    }

    /*new binary prot*/
    if (obj.containsKey(F("newprot")))
    {
        if (1)
            new_protocol_enable = obj["newprot"]["enable"];
            new_protocol_server = obj["newprot"]["server"].as<String>();
            new_protocol_port = obj["newprot"]["port"];
    }        

    if (obj.containsKey(F("beers")))
        beers_show = obj["beers"]["show"];

    return true;
}

bool OGN_save_config(void)
{
    const size_t        capacity = 2048;
    DynamicJsonDocument baseConfig(capacity);
    JsonObject          obj;

    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return false;
    }


    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile)
    {
        Serial.println(F("Failed to open config.json readonly"));
        return false;
    }

    DeserializationError error = deserializeJson(baseConfig, configFile);

    if (error)
    {
        Serial.println(F("Failed to read file, using default configuration, format spiffs"));
        configFile.close();
        SPIFFS.format();
        return false;
    }
    else
    {
        obj = baseConfig.as<JsonObject>();
        configFile.close();
    }

    configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println(F("Failed to open config.json write operation"));
        return false;
    }

    //position config
    obj["coordinates"]["lat"]      = ogn_lat;
    obj["coordinates"]["lon"]      = ogn_lon;
    obj["coordinates"]["alt"]      = ogn_alt;
    obj["coordinates"]["geoidsep"] = ogn_geoid_separation;
    //aps config
    obj["aprs"]["callsign"] = ogn_callsign;

    obj["aprs"]["server"] = ogn_server;
    obj["aprs"]["port"]   = ogn_port;

    obj["aprs"]["band"]        = ogn_band;
    obj["aprs"]["protocol_1"]  = ogn_protocol_1;
    obj["aprs"]["protocol_2"]  = ogn_protocol_2;
    obj["aprs"]["debug"]       = ogn_debug;
    obj["aprs"]["debugport"]   = ogn_debugport;
    obj["aprs"]["itrackbit"]   = ogn_itrackbit;
    obj["aprs"]["istealthbit"] = ogn_istealthbit;
    obj["aprs"]["sleepmode"]   = ogn_sleepmode;
    obj["aprs"]["rxidle"]      = ogn_rxidle;
    obj["aprs"]["wakeuptimer"] = ogn_wakeuptimer;
    obj["aprs"]["range"]       = ogn_range;

    //wifi config
    obj["wifi"]["ssid"][0] =  ogn_ssid[0];
    obj["wifi"]["pass"][0] =  ogn_wpass[0];
    //fanet config
    obj["fanetservice"]["enable"] = fanet_enable;
    //zabbix config
    obj["zabbix"]["enable"] = zabbix_enable;
    obj["zabbix"]["server"] = zabbix_server;
    obj["zabbix"]["port"]   = zabbix_port;
    obj["zabbix"]["key"]    = zabbix_key;

    
    if (serializeJson(obj, configFile) == 0)
        Serial.println(F("Failed to write to file"));

    configFile.close();
    return true;
}
