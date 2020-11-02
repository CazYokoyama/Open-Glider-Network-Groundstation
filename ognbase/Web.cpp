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

#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "SoC.h"
#include "EEPROM.h"
#include "RF.h"
#include "global.h"
#include "Battery.h"
#include <FS.h>   // Include the SPIFFS library

#define  U_PART U_FLASH


File fsUploadFile;

String ogn_ssid     = "ognbase";
String ogn_wpass    = "123456789";
String ogn_callsign = "callsign";

float   ogn_lat              = 0;
float   ogn_lon              = 0;
int     ogn_alt              = 0;
int     ogn_geoid_separation = 0;
uint8_t largest_range        = 0;

#define countof(a) (sizeof(a) / sizeof(a[0]))

// Create AsyncWebServer object on port 80
AsyncWebServer wserver(80);
AsyncWebSocket ws("/ws");

AsyncWebSocketClient* globalClient = NULL;

size_t content_len;

static const char upload_html[] PROGMEM = "<div class = 'upload'>\
                                            <form method = 'POST' action = '/doUpload' enctype='multipart/form-data'>\
                                            <input type='file' name='data'/><input type='submit' name='upload' value='Upload' title = 'Upload Files'>\
                                            </form></div>";


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

bool OGN_config_store(String* ssid, String* pass, String* callsign, float lat, float lon, int alt)
{
    // Deleting old file
    SPIFFS.remove("/ogn_conf.txt");

    // Open config file for writing.
    File configFile = SPIFFS.open("/ogn_conf.txt", "w");
    if (!configFile)
    {
        Serial.println(F("Failed to open ogn_conf.txt for writing"));
        return false;
    }

    // Save SSID and PSK.
    configFile.println(*ssid);
    configFile.println(*pass);
    configFile.println(*callsign);

    configFile.println(lat, 6);
    configFile.println(lon, 6);
    configFile.println(alt);
    configFile.println(ogn_geoid_separation);

    configFile.close();

    return true;
}

bool loadConfig()
{
    int line = 0;

    // open file for reading.
    File configFile = SPIFFS.open("/ogn_conf.txt", "r");
    if (!configFile)
    {
        Serial.println(F("Failed to open ogn_conf.txt."));
        return false;
    }

    while (configFile.available() && line < 7)
    {
        if (line == 0)
            ogn_ssid = configFile.readStringUntil('\n').c_str();
        if (line == 1)
            ogn_wpass = configFile.readStringUntil('\n').c_str();
        if (line == 2)
            ogn_callsign = configFile.readStringUntil('\n').c_str();
        if (line == 3)
            ogn_lat = configFile.readStringUntil('\n').toFloat();
        if (line == 4)
            ogn_lon = configFile.readStringUntil('\n').toFloat();
        if (line == 5)
            ogn_alt = configFile.readStringUntil('\n').toInt();
        if (line == 6)
            ogn_geoid_separation = configFile.readStringUntil('\n').toInt();
        line++;
    }

    configFile.close();

    if (line < 2)
        return false;

    ogn_ssid.trim();
    ogn_wpass.trim();
    ogn_callsign.trim();

    return true;
} // loadConfig

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
    if (!index)
    {
        Serial.println("Update");
        content_len = request->contentLength();
        // if filename includes spiffs, update the spiffs partition
        int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
#ifdef ESP8266
        Update.runAsync(true);
        if (!Update.begin(content_len, cmd))
        {
#else
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
        {
#endif
            Update.printError(Serial);
        }
    }

    if (Update.write(data, len) != len)
    {
        Update.printError(Serial);
#ifdef ESP8266
    }
    else
    {
        Serial.printf("Progress: %d%%\n", (Update.progress() * 100) / Update.size());
#endif
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
            Serial.println("Update complete");
            Serial.flush();
            ESP.restart();
        }
    }
}

void Web_setup(void)
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

        wserver.begin();
        return;
    }

    loadConfig();

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
    index_html[filesize + 1] = '\0';

    size_t size = 8700;
    char*  offset;
    char*  Settings_temp = (char *) malloc(size);

    if (Settings_temp == NULL)
        return;

    offset = Settings_temp;

    snprintf(offset, size, index_html, 
             SOFTRF_FIRMWARE_VERSION,
             ogn_callsign,
             String(ogn_lat, 5),
             String(ogn_lon, 5),
             String(ogn_alt),
             String(ogn_geoid_separation),
             String(settings->range),
             (settings->band == RF_BAND_AUTO ? "selected" : ""), RF_BAND_AUTO,
             (settings->band == RF_BAND_EU ? "selected" : ""), RF_BAND_EU,
             (settings->band == RF_BAND_RU ? "selected" : ""), RF_BAND_RU,
             (settings->band == RF_BAND_CN ? "selected" : ""), RF_BAND_CN,
             (settings->band == RF_BAND_US ? "selected" : ""), RF_BAND_US,
             (settings->band == RF_BAND_NZ ? "selected" : ""), RF_BAND_NZ,
             (settings->band == RF_BAND_UK ? "selected" : ""), RF_BAND_UK,
             (settings->band == RF_BAND_AU ? "selected" : ""), RF_BAND_AU,
             (settings->band == RF_BAND_IN ? "selected" : ""), RF_BAND_IN,
             (settings->rf_protocol == RF_PROTOCOL_LEGACY ? "selected" : ""),
             RF_PROTOCOL_LEGACY, legacy_proto_desc.name,
             (settings->rf_protocol == RF_PROTOCOL_OGNTP ? "selected" : ""),
             RF_PROTOCOL_OGNTP, ogntp_proto_desc.name,
             (settings->rf_protocol == RF_PROTOCOL_P3I ? "selected" : ""),
             RF_PROTOCOL_P3I, p3i_proto_desc.name,
             (settings->rf_protocol == RF_PROTOCOL_FANET ? "selected" : ""),
             RF_PROTOCOL_FANET, fanet_proto_desc.name,

             (settings->rf_protocol2 == RF_PROTOCOL_LEGACY ? "selected" : ""),
             RF_PROTOCOL_LEGACY, legacy_proto_desc.name,
             (settings->rf_protocol2 == RF_PROTOCOL_OGNTP ? "selected" : ""),
             RF_PROTOCOL_OGNTP, ogntp_proto_desc.name,
             (settings->rf_protocol2 == RF_PROTOCOL_P3I ? "selected" : ""),
             RF_PROTOCOL_P3I, p3i_proto_desc.name,
             (settings->rf_protocol2 == RF_PROTOCOL_FANET ? "selected" : ""),
             RF_PROTOCOL_FANET, fanet_proto_desc.name,

             (settings->ogndebug == true ? "selected" : ""),
             (settings->ogndebug == false ? "selected" : ""),
             String(settings->ogndebugp),

             (settings->ignore_no_track == true ? "selected" : ""), "True",
             (settings->ignore_no_track == false ? "selected" : ""), "False",

             (settings->ignore_stealth == true ? "selected" : ""), "True",
             (settings->ignore_stealth == false ? "selected" : ""), "False",

             ogn_ssid.c_str(),
             ogn_wpass.c_str(),

             (settings->sleep_mode == 0 ? "selected" : ""), "Disabled",
             (settings->sleep_mode == 1 ? "selected" : ""), "Full",
             (settings->sleep_mode == 2 ? "selected" : ""), "without GPS",

             String(settings->sleep_after_rx_idle),
             String(settings->wake_up_timer)
             );

    size_t len = strlen(offset);
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

    wserver.on("/doUpload", HTTP_POST, [](AsyncWebServerRequest* request) {}, handleUpload);


    // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
    wserver.on("/get", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (request->hasParam("callsign"))
        {
            ogn_callsign = request->getParam("callsign")->value();
            Serial.println(ogn_callsign);
        }


        if (request->hasParam("ogn_lat"))
        {
            ogn_lat = request->getParam("ogn_lat")->value().toFloat();
            Serial.println(ogn_lat);
        }

        if (request->hasParam("ogn_lon"))
        {
            ogn_lon = request->getParam("ogn_lon")->value().toFloat();
            Serial.println(ogn_lon);
        }

        if (request->hasParam("ogn_alt"))
        {
            ogn_alt = request->getParam("ogn_alt")->value().toInt();
            Serial.println(ogn_alt);
        }

        if (request->hasParam("ogn_freq"))
        {
            settings->band = request->getParam("ogn_freq")->value().toInt();
            Serial.println(settings->band);
        }

        if (request->hasParam("ogn_proto"))
        {
            settings->rf_protocol = request->getParam("ogn_proto")->value().toInt();
            Serial.println(settings->rf_protocol);
        }

        if (request->hasParam("ogn_proto2"))
        {
            settings->rf_protocol2 = request->getParam("ogn_proto2")->value().toInt();
            Serial.println(settings->rf_protocol2);
        }

        if (request->hasParam("ogn_d1090"))
        {
            settings->d1090 = request->getParam("ogn_d1090")->value().toInt();
            Serial.println(settings->d1090);
        }

        if (request->hasParam("ogn_gdl90"))
        {
            settings->gdl90 = request->getParam("ogn_gdl90")->value().toInt();
            Serial.println(settings->gdl90);
        }

        if (request->hasParam("ogn_nmea"))
        {
            settings->nmea_out = request->getParam("ogn_nmea")->value().toInt();
            Serial.println(settings->nmea_out);
        }

        if (request->hasParam("ogn_no_track_bit"))
        {
            settings->no_track = request->getParam("ogn_no_track_bit")->value().toInt();
            Serial.println(settings->no_track);
        }

        if (request->hasParam("ogn_stealth_bit"))
        {
            settings->stealth = request->getParam("ogn_stealth_bit")->value().toInt();
            Serial.println(settings->stealth);
        }

        if (request->hasParam("ogn_aprs_debug"))
        {
            settings->ogndebug = request->getParam("ogn_aprs_debug")->value().toInt();
            Serial.println(settings->ogndebug);
        }

        if (request->hasParam("aprs_debug_port"))
        {
            settings->ogndebugp = request->getParam("aprs_debug_port")->value().toInt();
            Serial.println(settings->ogndebugp);
        }

        if (request->hasParam("ogn_range"))
        {
            settings->range = request->getParam("ogn_range")->value().toInt();
            Serial.println(settings->range);
        }

        if (request->hasParam("ogn_agc"))
        {
            settings->sxlna = request->getParam("ogn_agc")->value().toInt();
            Serial.println(settings->sxlna);
        }

        if (request->hasParam("ogn_ssid"))
        {
            ogn_ssid = request->getParam("ogn_ssid")->value();
            Serial.println(ogn_ssid);
        }

        if (request->hasParam("ogn_wifi_password"))
        {
            ogn_wpass = request->getParam("ogn_wifi_password")->value();
            Serial.println(ogn_wpass);
        }
        //geoid_separation
        if (request->hasParam("ogn_geoid"))
        {
            ogn_geoid_separation = request->getParam("ogn_geoid")->value().toInt();
            Serial.println(ogn_geoid_separation);
        }

        if (request->hasParam("ogn_ignore_track"))
            settings->ignore_no_track = request->getParam("ogn_ignore_track")->value().toInt();

        if (request->hasParam("ogn_ignore_stealth"))
            settings->ignore_stealth = request->getParam("ogn_ignore_stealth")->value().toInt();

        if (request->hasParam("ogn_deep_sleep"))
            settings->sleep_mode = request->getParam("ogn_deep_sleep")->value().toInt();

        if (request->hasParam("ogn_sleep_time"))
        {
            if (request->getParam("ogn_sleep_time")->value().toInt() < 180)
                settings->sleep_after_rx_idle = 180;
            else
                settings->sleep_after_rx_idle = request->getParam("ogn_sleep_time")->value().toInt();
        }
        if (request->hasParam("ogn_wakeup_time"))
            settings->wake_up_timer = request->getParam("ogn_wakeup_time")->value().toInt();


        request->redirect("/");

        // ogn_reset_all
        if (request->hasParam("ogn_reset_all"))
            if(request->getParam("ogn_reset_all")->value() == "on"){
              SPIFFS.format();
              RF_Shutdown();
              delay(1000);
              SoC->reset();
            }
            
       EEPROM_store();
       OGN_config_store(&ogn_ssid, &ogn_wpass, &ogn_callsign, ogn_lat, ogn_lon, ogn_alt);
       RF_Shutdown();
       delay(1000);
       SoC->reset();

    });

    SoC->swSer_enableRx(true);
    free(Settings_temp);

    // Start server
    wserver.begin();
}

void Web_loop(void)
{
    if (globalClient != NULL && globalClient->status() == WS_CONNECTED)
    {
        String values;
        values  =+SoC->Battery_voltage();
        values += "_";
        values += RF_last_rssi;
        values += "_";
        values += settings->range;
        values += "_";
        values += gnss.satellites.value();
        values += "_";
        values += ThisAircraft.timestamp;
        values += "_";
        values += largest_range;
        globalClient->text(values);
        delay(1000);
    }
}
