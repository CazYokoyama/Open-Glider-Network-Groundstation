/*
 * Web.cpp
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

#include "git-version.h"
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "SoC.h"
#include "EEPROM.h"
#include "RF.h"
#include "global.h"
#include "Battery.h"
#include "Log.h"
#include "config.h"
#include <ArduinoJson.h>

#include <ErriezCRC32.h>

#include <FS.h>   // Include the SPIFFS library

#define  U_PART U_FLASH
#define INDEX_CRC 3948812196

#define hours() (millis() / 3600000)


File fsUploadFile;

#define countof(a) (sizeof(a) / sizeof(a[0]))

// Create AsyncWebServer object on port 80
AsyncWebServer wserver(80);
AsyncWebSocket ws("/ws");

AsyncWebSocketClient* globalClient = NULL;

size_t content_len;

static const char upload_html[] PROGMEM = "<html>\
                                            <head>\
                                            <meta http-equiv='Content-Type' content='text/html; charset=utf-8'>\
                                            </head>\
                                            <div class = 'upload'>\
                                            <form method = 'POST' action = '/doUpload' enctype='multipart/form-data'>\
                                            <input type='file' name='data'/><input type='submit' name='upload' value='Upload' title = 'Upload Files'>\
                                            </form></div>\
                                            </html>";


void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    if (type == WS_EVT_CONNECT)

        globalClient = client;

    else if (type == WS_EVT_DISCONNECT)

        globalClient = NULL;
}

// Replaces placeholder with LED state value
String processor(const String& var)
{
    Serial.println(var);
    return "ON";
}

void Web_fini()
{}

void handleUpdate(AsyncWebServerRequest* request)
{
    char* html = "<form method='POST' action='/doUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
    request->send(200, "text/html", html);
}

void handleUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final)
{
    if (!index)
        request->_tempFile = SPIFFS.open("/" + filename, "w");
    if (len)
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
    if (final)
    {
        request->_tempFile.close();
        request->redirect("/");
    }
}

void handleDoUpdate(AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data, size_t len, bool final)
{
    String msg;

    if (!index)
    {
        msg = "updating firmware";
        Logger_send_udp(&msg);

       //SPIFFS.format();
        
        content_len = request->contentLength();
        // if filename includes spiffs, update the spiffs partition
        int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;

        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
        {

            Update.printError(Serial);
        }
    }

    if (Update.write(data, len) != len)
    {
        Update.printError(Serial);
    }

    if (final)
    {
        AsyncWebServerResponse* response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
        response->addHeader("Refresh", "20");
        response->addHeader("Location", "/");
        request->send(response);
        if (!Update.end(true))
            Update.printError(Serial);
        else
        {
            delay(10000);
            ESP.restart();
        }
    }
}

void Web_start()
{
    wserver.begin();
}

void Web_stop()
{
    wserver.end();
}

void Web_setup(ufo_t* this_aircraft)
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    if (!SPIFFS.exists("/index.html"))
    {
        wserver.on("/", HTTP_GET, [upload_html](AsyncWebServerRequest* request){
            request->send(200, "text/html", upload_html);
        });

        wserver.on("/doUpload", HTTP_POST, [](AsyncWebServerRequest* request) {}, handleUpload);

        Web_start();
        return;
    }

    File file = SPIFFS.open("/index.html", "r");
    if (!file)
    {
        Serial.println("An Error has occurred while opening index.html");
        return;
    }

    ws.onEvent(onWsEvent);
    wserver.addHandler(&ws);


    size_t filesize   = file.size();
    char*  index_html = (char *) malloc(filesize + 1);

    file.read((uint8_t *)index_html, filesize);
    index_html[filesize] = '\0';

    size_t size = 8700;
    char*  offset;
    char*  Settings_temp = (char *) malloc(size);

    if (Settings_temp == NULL)
        return;

    offset = Settings_temp;

    String station_addr = String(this_aircraft->addr, HEX);
    station_addr.toUpperCase();

    /*Bugfix Issue 20*/
    String pos_lat = "";
    String pos_lon = "";    
    String pos_alt = "";
    String pos_geo = "";

    pos_lat.concat(ogn_lat);
    pos_lon.concat(ogn_lon);
    pos_alt.concat(ogn_alt);
    pos_geo.concat(ogn_geoid_separation);
    /*END  Bugfix*/ 

    snprintf(offset, size, index_html,
             station_addr,
             GIT_VERSION,
             ogn_callsign,
             pos_lat,
             pos_lon,
             pos_alt,
             pos_geo,
             String(ogn_range),
             (ogn_band == RF_BAND_AUTO ? "selected" : ""), RF_BAND_AUTO,
             (ogn_band == RF_BAND_EU ? "selected" : ""), RF_BAND_EU,
             (ogn_band == RF_BAND_RU ? "selected" : ""), RF_BAND_RU,
             (ogn_band == RF_BAND_CN ? "selected" : ""), RF_BAND_CN,
             (ogn_band == RF_BAND_US ? "selected" : ""), RF_BAND_US,
             (ogn_band == RF_BAND_NZ ? "selected" : ""), RF_BAND_NZ,
             (ogn_band == RF_BAND_UK ? "selected" : ""), RF_BAND_UK,
             (ogn_band == RF_BAND_AU ? "selected" : ""), RF_BAND_AU,
             (ogn_band == RF_BAND_IN ? "selected" : ""), RF_BAND_IN,
             (ogn_protocol_1 == RF_PROTOCOL_LEGACY ? "selected" : ""),
             RF_PROTOCOL_LEGACY, legacy_proto_desc.name,
             (ogn_protocol_1 == RF_PROTOCOL_OGNTP ? "selected" : ""),
             RF_PROTOCOL_OGNTP, ogntp_proto_desc.name,
             (ogn_protocol_1 == RF_PROTOCOL_P3I ? "selected" : ""),
             RF_PROTOCOL_P3I, p3i_proto_desc.name,
             (ogn_protocol_1 == RF_PROTOCOL_FANET ? "selected" : ""),
             RF_PROTOCOL_FANET, fanet_proto_desc.name,

             (ogn_protocol_2 == RF_PROTOCOL_LEGACY ? "selected" : ""),
             RF_PROTOCOL_LEGACY, legacy_proto_desc.name,
             (ogn_protocol_2 == RF_PROTOCOL_OGNTP ? "selected" : ""),
             RF_PROTOCOL_OGNTP, ogntp_proto_desc.name,
             (ogn_protocol_2 == RF_PROTOCOL_P3I ? "selected" : ""),
             RF_PROTOCOL_P3I, p3i_proto_desc.name,
             (ogn_protocol_2 == RF_PROTOCOL_FANET ? "selected" : ""),
             RF_PROTOCOL_FANET, fanet_proto_desc.name,

             (ogn_debug == true ? "selected" : ""),
             (ogn_debug == false ? "selected" : ""),
             String(ogn_debugIP),

             (ogn_itrackbit == true ? "selected" : ""), "True",
             (ogn_itrackbit == false ? "selected" : ""), "False",

             (ogn_istealthbit == true ? "selected" : ""), "True",
             (ogn_istealthbit == false ? "selected" : ""), "False",

             ogn_ssid[0].c_str(),
             
             /*Hide Wifi Password*/
             "hidepass",

             (ogn_sleepmode == OSM_DISABLED ? "selected" : ""), "Disabled",
             (ogn_sleepmode == OSM_FULL ? "selected" : ""), "Full",
             (ogn_sleepmode == OSM_WITHOUT_GPS ? "selected" : ""), "without GPS", //zabbix_trap_en

             String(ogn_rxidle),
             String(ogn_wakeuptimer),

             (zabbix_enable == 0 ? "selected" : ""), "Disabled",
             (zabbix_enable == 1 ? "selected" : ""), "Enabled"
             );
            

    size_t len  = strlen(offset);
    String html = String(offset);

    wserver.on("/", HTTP_GET, [html](AsyncWebServerRequest* request){
        request->send(200, "text/html", html);
    });


    // Route to load style.css file
    wserver.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request){
        request->send(SPIFFS, "/style.css", "text/css");
    });

    wserver.on("/update", HTTP_GET, [](AsyncWebServerRequest* request){
        handleUpdate(request);
        request->redirect("/");
    });

    wserver.on("/doUpdate", HTTP_POST,
               [](AsyncWebServerRequest* request) {},
               [](AsyncWebServerRequest* request, const String& filename, size_t index, uint8_t* data,
                  size_t len, bool final) {
        handleDoUpdate(request, filename, index, data, len, final);
    });

    wserver.on("/upload", HTTP_GET, [upload_html](AsyncWebServerRequest* request){
        request->send(200, "text/html", upload_html);
    });

    wserver.on("/reboot", HTTP_GET, [](AsyncWebServerRequest* request){
        request->redirect("/");
        SoC->reset();
    });    

    wserver.on("/doUpload", HTTP_POST, [](AsyncWebServerRequest* request) {}, handleUpload);


    // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
    wserver.on("/get", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (request->hasParam("callsign"))
            ogn_callsign = request->getParam("callsign")->value().c_str();
            ogn_callsign.trim();
            ogn_callsign.replace("_","");
            if(ogn_callsign.length() > 9){
              ogn_callsign = ogn_callsign.substring(0,9);
            }
        if (request->hasParam("ogn_lat"))
            ogn_lat = request->getParam("ogn_lat")->value().toFloat();

        if (request->hasParam("ogn_lon"))
            ogn_lon = request->getParam("ogn_lon")->value().toFloat();

        if (request->hasParam("ogn_alt"))
            ogn_alt = request->getParam("ogn_alt")->value().toInt();

        if (request->hasParam("ogn_freq"))
            ogn_band = request->getParam("ogn_freq")->value().toInt();

        if (request->hasParam("ogn_proto"))
            ogn_protocol_1 = request->getParam("ogn_proto")->value().toInt();

        if (request->hasParam("ogn_proto2"))
            ogn_protocol_2 = request->getParam("ogn_proto2")->value().toInt();

        if (request->hasParam("ogn_d1090"))
            settings->d1090 = request->getParam("ogn_d1090")->value().toInt();

        if (request->hasParam("ogn_gdl90"))
            settings->gdl90 = request->getParam("ogn_gdl90")->value().toInt();

        if (request->hasParam("ogn_nmea"))
            settings->nmea_out = request->getParam("ogn_nmea")->value().toInt();

        if (request->hasParam("ogn_no_track_bit"))
            settings->no_track = request->getParam("ogn_no_track_bit")->value().toInt();

        if (request->hasParam("ogn_stealth_bit"))
            settings->stealth = request->getParam("ogn_stealth_bit")->value().toInt();

        if (request->hasParam("ogn_aprs_debug"))
            ogn_debug= request->getParam("ogn_aprs_debug")->value().toInt();

        if (request->hasParam("aprs_debug_IP"))
            ogn_debugIP = request->getParam("aprs_debug_IP")->value().toInt();

        if (request->hasParam("ogn_range"))
            ogn_range = request->getParam("ogn_range")->value().toInt();

        //if (request->hasParam("ogn_agc"))
        //    settings->sxlna = request->getParam("ogn_agc")->value().toInt();

        if (request->hasParam("ogn_ssid"))
            ogn_ssid[0] = request->getParam("ogn_ssid")->value().c_str();

        if (request->hasParam("ogn_wifi_password")){
            if (request->getParam("ogn_wifi_password")->value() != "hidepass"){
              ogn_wpass[0] = request->getParam("ogn_wifi_password")->value().c_str();
            }
        }
            
        //geoid_separation
        if (request->hasParam("ogn_geoid"))
            ogn_geoid_separation = request->getParam("ogn_geoid")->value().toInt();

        if (request->hasParam("ogn_ignore_track"))
            ogn_itrackbit= request->getParam("ogn_ignore_track")->value().toInt();

        if (request->hasParam("ogn_ignore_stealth"))
            ogn_istealthbit= request->getParam("ogn_ignore_stealth")->value().toInt();

        if (request->hasParam("ogn_deep_sleep"))
            ogn_sleepmode = request->getParam("ogn_deep_sleep")->value().toInt();

        if (request->hasParam("ogn_sleep_time"))
        {
            if (request->getParam("ogn_sleep_time")->value().toInt() <= 60)
                ogn_rxidle = 60;
            else
                ogn_rxidle = request->getParam("ogn_sleep_time")->value().toInt();
        }
        if (request->hasParam("ogn_wakeup_time"))
            ogn_wakeuptimer= request->getParam("ogn_wakeup_time")->value().toInt();

        if (request->hasParam("zabbix_trap_en"))
            zabbix_enable = request->getParam("zabbix_trap_en")->value().toInt();


        request->redirect("/");

        // ogn_reset_all
        if (request->hasParam("ogn_reset_all"))
            if (request->getParam("ogn_reset_all")->value() == "on")
            {
                SPIFFS.format();
                RF_Shutdown();
                delay(200);
                SoC->reset();
            }

        EEPROM_store();
        OGN_save_config();
        RF_Shutdown();
        delay(200);
        SoC->reset();
    });

    SoC->swSer_enableRx(true);
    free(Settings_temp);
    free(index_html);

    // Start server
    Web_start();
}

void Web_loop(void)
{
    if (globalClient != NULL && globalClient->status() == WS_CONNECTED)
    {
        String values;
        if(SoC->Battery_voltage() > 3.2)
          values  =+SoC->Battery_voltage();
        else
          values += 0.0;
        values += "_";
        values += RF_last_rssi;
        values += "_";
        values += hours();
        values += "_";
        values += gnss.satellites.value();
        values += "_";
        values += now();
        values += "_";
        values += largest_range;
        globalClient->text(values);
    }
}
