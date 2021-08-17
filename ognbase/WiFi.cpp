/*
 * WiFi.cpp
 * Copyright (C) 2019-2020 Linar Yusupov
 *
 * Bug fixes and improvements
 * Copyright (C) 2020 Manuel Roesel
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

#if defined(EXCLUDE_WIFI)
void WiFi_setup()
{}
void WiFi_loop()
{}
void WiFi_fini()
{}
#else

#include <FS.h>
#include <TimeLib.h>

#include "OTA.h"
#include "GNSS.h"
#include "EEPROM.h"
#include "OLED.h"
#include "WiFi.h"
#include "Traffic.h"
#include "RF.h"
#include "Web.h"
//#include "NMEA.h"
#include "Battery.h"

#include "config.h"
#include "global.h"
#include "logos.h"

String host_name = HOSTNAME;

IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

/**
 * Default WiFi connection information.
 *
 */

String station_ssid = "ognbase";
String station_psk  = "123456789";

const char* ap_default_psk = "987654321"; ///< Default PSK.

#if defined(USE_DNS_SERVER)
#include <DNSServer.h>

const byte DNS_PORT = 53;
DNSServer  dnsServer;
bool       dns_active = false;
#endif

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Uni_Udp;

unsigned int RFlocalPort = RELAY_SRC_PORT;      // local port to listen for UDP packets

char UDPpacketBuffer[256]; // buffer to hold incoming and outgoing packets

#if defined(POWER_SAVING_WIFI_TIMEOUT)
static unsigned long WiFi_No_Clients_Time_ms = 0;
#endif

size_t Raw_Receive_UDP(uint8_t* buf)
{
    int noBytes = Uni_Udp.parsePacket();
    if (noBytes)
    {
        if (noBytes > MAX_PKT_SIZE)
            noBytes = MAX_PKT_SIZE;

        // We've received a packet, read the data from it
        Uni_Udp.read(buf, noBytes); // read the packet into the buffer

        return (size_t) noBytes;
    }
    else
        return 0;
}

void Raw_Transmit_UDP()
{
    size_t rx_size = RF_Payload_Size(ogn_protocol_1);
    rx_size = rx_size > sizeof(fo.raw) ? sizeof(fo.raw) : rx_size;
    String str = Bin2Hex(fo.raw, rx_size);
    size_t len = str.length();
    // ASSERT(sizeof(UDPpacketBuffer) > 2 * PKT_SIZE + 1)
    str.toCharArray(UDPpacketBuffer, sizeof(UDPpacketBuffer));
    UDPpacketBuffer[len] = '\n';
    //SoC->WiFi_transmit_UDP(RELAY_DST_PORT, (byte *)UDPpacketBuffer, len + 1);
}

/**
 * @brief Arduino setup function.
 */
void WiFi_setup()
{
    char buf[32];

    if (WiFi.getMode() != WIFI_STA)
    {
        WiFi.mode(WIFI_STA);
        delay(10);
    }

    if (OGN_read_config())
    {
        Serial.println(F("WiFi config changed."));

        delay(1500);

        for (int i=0; i < 5; i++) {
            if (ogn_ssid[i] != "")
            {
                OLED_draw_Bitmap(85, 20, 1, true);
                snprintf(buf, sizeof(buf), "try connecting");
                OLED_write(buf, 0, 15, false);
                snprintf(buf, sizeof(buf), "%d: %s", i, ogn_ssid[i].c_str());
                OLED_write(buf, 0, 26, false);

                WiFi.begin(ogn_ssid[i].c_str(), ogn_wpass[i].c_str());
                ssid_index = i;

                //
                unsigned long startTime = millis();

                while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000)
                {
                    Serial.write('.');
                    Serial.print(WiFi.status());
                    delay(500);
                }
            }

            // Check connection
            if (WiFi.status() == WL_CONNECTED)
            {
                // ... print IP Address
                Serial.print(F("IP address: "));
                Serial.println(WiFi.localIP());
                snprintf(buf, sizeof(buf), "success..");
                OLED_write(buf, 0, 44, false);
                host_name += String((SoC->getChipId() & 0xFFFFFF), HEX);
                SoC->WiFi_hostname(host_name);

                // Print hostname.
                Serial.println("Hostname: " + host_name);
                break;
            }
            if (WiFi.status() != WL_CONNECTED)
            {
                Serial.println();
                Serial.println(F("Can not connect to WiFi station"));
                snprintf(buf, sizeof(buf), "failed..");
                OLED_write(buf, 0, 44, false);
            }
        }
    }
    else
    {
        station_ssid = MY_ACCESSPOINT_SSID;
        station_psk  = MY_ACCESSPOINT_PSK;
        Serial.println(F("No WiFi connection information available."));
        snprintf(buf, sizeof(buf), "no config file found..");
        OLED_write(buf, 0, 25, false);
        WiFi.begin();
        delay(1000);
    }

    if (WiFi.status() != WL_CONNECTED)
    {
        host_name += String((SoC->getChipId() & 0xFFFFFF), HEX);
        SoC->WiFi_hostname(host_name);

        // Print hostname.
        Serial.println("Hostname: " + host_name);
        Serial.println(F("Wait for WiFi connection."));

        WiFi.mode(WIFI_AP);
        SoC->WiFi_setOutputPower(WIFI_TX_POWER_MED); // 10 dB
        // WiFi.setOutputPower(0); // 0 dB
        //system_phy_set_max_tpw(4 * 0); // 0 dB
        delay(10);

        Serial.print(F("Setting soft-AP configuration ... "));
        Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ?
                       F("Ready") : F("Failed!"));

        Serial.print(F("Setting soft-AP ... "));
        Serial.println(WiFi.softAP(host_name.c_str(), ap_default_psk) ?
                       F("Ready") : F("Failed!"));
        Serial.print(F("IP address: "));
        Serial.println(WiFi.softAPIP());
    }


    Uni_Udp.begin(RFlocalPort);
    Serial.print(F("UDP server has started at port: "));
    Serial.println(RFlocalPort);

#if defined(POWER_SAVING_WIFI_TIMEOUT)
    WiFi_No_Clients_Time_ms = millis();
#endif
}

bool Wifi_connected()
{
    if (WiFi.status() != WL_CONNECTED)
        return false;
    else
        return true;
}

void WiFi_loop()
{
#if defined(USE_DNS_SERVER)
    if (dns_active)
        dnsServer.processNextRequest();

#endif

#if defined(POWER_SAVING_WIFI_TIMEOUT)
    if ((settings->power_save & POWER_SAVE_WIFI) && WiFi.getMode() == WIFI_AP)
    {
        if (SoC->WiFi_clients_count() == 0)
        {
            if ((millis() - WiFi_No_Clients_Time_ms) > POWER_SAVING_WIFI_TIMEOUT)
            {
                //NMEA_fini();
                Web_fini();
                WiFi_fini();

                if (settings->nmea_p)
                    StdOut.println(F("$PSRFS,WIFI_OFF"));
            }
        }
        else
            WiFi_No_Clients_Time_ms = millis();
    }
#endif
}

void WiFi_fini()
{
    Uni_Udp.stop();

    WiFi.mode(WIFI_OFF);
}

#endif /* EXCLUDE_WIFI */
