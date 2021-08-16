/*
 * GNSS.cpp
 * Copyright (C) 2019-2020 Linar Yusupov
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

#if defined(ARDUINO)
#include <Arduino.h>
#endif
#include <TimeLib.h>

#include "GNSS.h"
#include "EEPROM.h"
#include "NMEA.h"
#include "SoC.h"
#include "WiFi.h"
#include "RF.h"
#include "Battery.h"

#if !defined(EXCLUDE_EGM96)
#include <egm96s.h>
#endif /* EXCLUDE_EGM96 */

#if !defined(DO_GNSS_DEBUG)
#define GNSS_DEBUG_PRINT
#define GNSS_DEBUG_PRINTLN
#else
#define GNSS_DEBUG_PRINT    Serial.print
#define GNSS_DEBUG_PRINTLN  Serial.println
#endif

unsigned long          GNSSTimeSyncMarker = 0;
volatile unsigned long PPS_TimeMarker     = 0;

#if 0
unsigned long GGA_Start_Time_Marker = 0;
unsigned long GGA_Stop_Time_Marker  = 0;
#endif

boolean     gnss_set_sucess = false;
TinyGPSPlus gnss;  // Create an Instance of the TinyGPS++ object called gnss

uint8_t GNSSbuf[250]; // at least 3 lines of 80 characters each
                      // and 40+30*N bytes for "UBX-MON-VER" payload
int GNSS_cnt = 0;

/* CFG-MSG */
const uint8_t setGLL[] PROGMEM = {0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
const uint8_t setGSV[] PROGMEM = {0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
const uint8_t setVTG[] PROGMEM = {0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
#if !defined(NMEA_TCP_SERVICE)
const uint8_t setGSA[] PROGMEM = {0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
#endif
/* CFG-PRT */
uint8_t setBR[] = {0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x00, 0x96,
                   0x00, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};

const uint8_t setNav5[] PROGMEM = {0xFF, 0xFF, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00,
                                   0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00,
                                   0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                   0x00, 0x00, 0x00, 0x00};

const uint8_t CFG_RST[12]   PROGMEM = {0xb5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00,
                                       0x00, 0x01, 0x00, 0x0F, 0x66};

const uint8_t RXM_PMREQ_OFF[16] PROGMEM = {0xb5, 0x62, 0x02, 0x41, 0x08, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
                                           0x00, 0x00, 0x4d, 0x3b};

const byte RXM_PMREQ[16] = {0xb5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4d, 0x3b};   //power off until wakeup call
uint8_t    GPSon[16]     = {0xB5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x4C, 0x37};

#if defined(USE_GNSS_PSM)
static bool gnss_psm_active = false;

/* Max Performance Mode (default) */
const uint8_t RXM_MAXP[] PROGMEM = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x00, 0x21, 0x91};

/* Power Save Mode */
const uint8_t RXM_PSM[] PROGMEM  = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92};
#endif /* USE_GNSS_PSM */

const char* GNSS_name[] = {
    [GNSS_MODULE_NONE] = "NONE",
    [GNSS_MODULE_NMEA] = "NMEA",
    [GNSS_MODULE_U6]   = "U6",
    [GNSS_MODULE_U7]   = "U7",
    [GNSS_MODULE_U8]   = "U8",
    [GNSS_MODULE_U9]   = "U9",
    [GNSS_MODULE_MAV]  = "MAV",
    [GNSS_MODULE_S7XG] = "S7XG"
};

#if defined(USE_NMEA_CFG)

#include "RF.h"       /* RF_Shutdown() */


#include "GDL90.h"
#include "D1090.h"

TinyGPSCustom C_Version(gnss, "PSRFC", 1);
TinyGPSCustom C_Mode(gnss, "PSRFC", 2);
TinyGPSCustom C_Protocol(gnss, "PSRFC", 3);
TinyGPSCustom C_Band(gnss, "PSRFC", 4);
TinyGPSCustom C_AcftType(gnss, "PSRFC", 5);
TinyGPSCustom C_Alarm(gnss, "PSRFC", 6);
TinyGPSCustom C_TxPower(gnss, "PSRFC", 7);
TinyGPSCustom C_Volume(gnss, "PSRFC", 8);
TinyGPSCustom C_Pointer(gnss, "PSRFC", 9);
TinyGPSCustom C_NMEA_gnss(gnss, "PSRFC", 10);
TinyGPSCustom C_NMEA_private(gnss, "PSRFC", 11);
TinyGPSCustom C_NMEA_legacy(gnss, "PSRFC", 12);
TinyGPSCustom C_NMEA_sensors(gnss, "PSRFC", 13);
TinyGPSCustom C_NMEA_Output(gnss, "PSRFC", 14);
TinyGPSCustom C_GDL90_Output(gnss, "PSRFC", 15);
TinyGPSCustom C_D1090_Output(gnss, "PSRFC", 16);
TinyGPSCustom C_Stealth(gnss, "PSRFC", 17);
TinyGPSCustom C_noTrack(gnss, "PSRFC", 18);
TinyGPSCustom C_PowerSave(gnss, "PSRFC", 19);

#endif /* USE_NMEA_CFG */

static uint8_t makeUBXCFG(uint8_t cl, uint8_t id, uint8_t msglen, const uint8_t* msg)
{
    if (msglen > (sizeof(GNSSbuf) - 8))
        msglen = sizeof(GNSSbuf) - 8;

    // Construct the UBX packet
    GNSSbuf[0] = 0xB5;   // header
    GNSSbuf[1] = 0x62;   // header
    GNSSbuf[2] = cl;     // class
    GNSSbuf[3] = id;     // id
    GNSSbuf[4] = msglen; // length
    GNSSbuf[5] = 0x00;

    GNSSbuf[6 + msglen] = 0x00; // CK_A
    GNSSbuf[7 + msglen] = 0x00; // CK_B

    for (int i = 2; i < 6; i++) {
        GNSSbuf[6 + msglen] += GNSSbuf[i];
        GNSSbuf[7 + msglen] += GNSSbuf[6 + msglen];
    }

    for (int i = 0; i < msglen; i++) {
        GNSSbuf[6 + i]       = pgm_read_byte(&msg[i]);
        GNSSbuf[6 + msglen] += GNSSbuf[6 + i];
        GNSSbuf[7 + msglen] += GNSSbuf[6 + msglen];
    }
    return msglen + 8;
}

// Send a byte array of UBX protocol to the GPS
static void sendUBX(const uint8_t* MSG, uint8_t len)
{
    for (int i = 0; i < len; i++) {
        swSer.write(MSG[i]);
        GNSS_DEBUG_PRINT(MSG[i], HEX);
    }
//  swSer.println();
}

// Calculate expected UBX ACK packet and parse UBX response from GPS
static boolean getUBX_ACK(uint8_t cl, uint8_t id)
{
    uint8_t       b;
    uint8_t       ackByteID    = 0;
    uint8_t       ackPacket[2] = {cl, id};
    unsigned long startTime    = millis();
    GNSS_DEBUG_PRINT(F(" * Reading ACK response: "));

    // Construct the expected ACK packet
    makeUBXCFG(0x05, 0x01, 2, ackPacket);

    while (1) {
        // Test for success
        if (ackByteID > 9)
        {
            // All packets in order!
            GNSS_DEBUG_PRINTLN(F(" (SUCCESS!)"));
            return true;
        }

        // Timeout if no valid response in 2 seconds
        if (millis() - startTime > 2000)
        {
            GNSS_DEBUG_PRINTLN(F(" (FAILED!)"));
            return false;
        }

        // Make sure data is available to read
        if (swSer.available())
        {
            b = swSer.read();

            // Check that bytes arrive in sequence as per expected ACK packet
            if (b == GNSSbuf[ackByteID])
            {
                ackByteID++;
                GNSS_DEBUG_PRINT(b, HEX);
            }
            else
            {
                ackByteID = 0; // Reset and look again, invalid order
            }
        }
        yield();
    }
}

static void setup_UBX()
{
    uint8_t msglen;

#if 0
    unsigned int baudrate =;

    setBR[ 8] = (baudrate) & 0xFF;
    setBR[ 9] = (baudrate >>  8) & 0xFF;
    setBR[10] = (baudrate >> 16) & 0xFF;

    SoC->swSer_begin(9600);

    Serial.print(F("Switching baud rate onto "));
    Serial.println(baudrate);

    msglen = makeUBXCFG(0x06, 0x00, sizeof(setBR), setBR);
    sendUBX(GNSSbuf, msglen);
    gnss_set_sucess = getUBX_ACK(0x06, 0x00);

    if (!gnss_set_sucess)
    {
        Serial.print(F("WARNING: Unable to set baud rate onto "));
        Serial.println(baudrate);
    }
    swSer.flush();
    SoC->swSer_begin(baudrate);
#endif

    GNSS_DEBUG_PRINTLN(F("Airborne <2g navigation mode: "));

    // Set the navigation mode (Airborne, < 2g)
    msglen = makeUBXCFG(0x06, 0x24, sizeof(setNav5), setNav5);
    sendUBX(GNSSbuf, msglen);
    gnss_set_sucess = getUBX_ACK(0x06, 0x24);

    if (!gnss_set_sucess)
        GNSS_DEBUG_PRINTLN(F("WARNING: Unable to set airborne <2g navigation mode."));

    GNSS_DEBUG_PRINTLN(F("Switching off NMEA GLL: "));

    msglen = makeUBXCFG(0x06, 0x01, sizeof(setGLL), setGLL);
    sendUBX(GNSSbuf, msglen);
    gnss_set_sucess = getUBX_ACK(0x06, 0x01);

    if (!gnss_set_sucess)
        GNSS_DEBUG_PRINTLN(F("WARNING: Unable to disable NMEA GLL."));

    GNSS_DEBUG_PRINTLN(F("Switching off NMEA GSV: "));

    msglen = makeUBXCFG(0x06, 0x01, sizeof(setGSV), setGSV);
    sendUBX(GNSSbuf, msglen);
    gnss_set_sucess = getUBX_ACK(0x06, 0x01);

    if (!gnss_set_sucess)
        GNSS_DEBUG_PRINTLN(F("WARNING: Unable to disable NMEA GSV."));

    GNSS_DEBUG_PRINTLN(F("Switching off NMEA VTG: "));

    msglen = makeUBXCFG(0x06, 0x01, sizeof(setVTG), setVTG);
    sendUBX(GNSSbuf, msglen);
    gnss_set_sucess = getUBX_ACK(0x06, 0x01);

    if (!gnss_set_sucess)
        GNSS_DEBUG_PRINTLN(F("WARNING: Unable to disable NMEA VTG."));

#if !defined(NMEA_TCP_SERVICE)

    GNSS_DEBUG_PRINTLN(F("Switching off NMEA GSA: "));

    msglen = makeUBXCFG(0x06, 0x01, sizeof(setGSA), setGSA);
    sendUBX(GNSSbuf, msglen);
    gnss_set_sucess = getUBX_ACK(0x06, 0x01);

    if (!gnss_set_sucess)
        GNSS_DEBUG_PRINTLN(F("WARNING: Unable to disable NMEA GSA."));

#endif
}

static void setup_NMEA()
{
#if 0
    //swSer.write("$PUBX,41,1,0007,0003,9600,0*10\r\n");
    swSer.write("$PUBX,41,1,0007,0003,38400,0*20\r\n");

    swSer.flush();
    SoC->swSer_begin(38400);

    // Turning off some GPS NMEA strings on the uBlox modules
    swSer.write("$PUBX,40,GLL,0,0,0,0*5C\r\n");
    delay(250);
    swSer.write("$PUBX,40,GSV,0,0,0,0*59\r\n");
    delay(250);
    swSer.write("$PUBX,40,VTG,0,0,0,0*5E\r\n");
    delay(250);
#if !defined(NMEA_TCP_SERVICE)
    swSer.write("$PUBX,40,GSA,0,0,0,0*4E\r\n");
    delay(250);
#endif
#endif

#if defined(USE_AT6558_SETUP)
    /* Assume that we deal with fake NEO module (AT6558 based) */
    swSer.write("$PCAS04,5*1C\r\n"); /* GPS + GLONASS */ delay(250);
#if defined(NMEA_TCP_SERVICE)
    /* GGA,RMC and GSA */
    swSer.write("$PCAS03,1,0,1,0,1,0,0,0,0,0,,,0,0*03\r\n");
    delay(250);
#else
    /* GGA and RMC */
    swSer.write("$PCAS03,1,0,0,0,1,0,0,0,0,0,,,0,0*02\r\n");
    delay(250);
#endif
    swSer.write("$PCAS11,6*1B\r\n"); /* Aviation < 2g */ delay(250);
#endif /* USE_AT6558_SETUP */
}

/* ------ BEGIN -----------  https://github.com/Black-Thunder/FPV-Tracker */

enum ubloxState
{
    WAIT_SYNC1, WAIT_SYNC2, GET_CLASS, GET_ID, GET_LL, GET_LH, GET_DATA, GET_CKA, GET_CKB
};

ubloxState ubloxProcessDataState = WAIT_SYNC1;

unsigned short ubloxExpectedDataLength;
unsigned short ubloxClass, ubloxId;
unsigned char  ubloxCKA, ubloxCKB;

// process serial data
// data is stored inside #GNSSbuf, data size inside #GNSS_cnt
// warning : if #GNSSbuf is too short, data is truncated.
static int ubloxProcessData(unsigned char data)
{
    int parsed = 0;

    switch (ubloxProcessDataState)
    {
        case WAIT_SYNC1:
            if (data == 0xb5)
                ubloxProcessDataState = WAIT_SYNC2;
            break;

        case WAIT_SYNC2:
            if (data == 0x62)
                ubloxProcessDataState = GET_CLASS;
            else if (data == 0xb5)
            {
                // ubloxProcessDataState = GET_SYNC2;
            }
            else
                ubloxProcessDataState = WAIT_SYNC1;
            break;
        case GET_CLASS:
            ubloxClass            = data;
            ubloxCKA              = data;
            ubloxCKB              = data;
            ubloxProcessDataState = GET_ID;
            break;

        case GET_ID:
            ubloxId               = data;
            ubloxCKA             += data;
            ubloxCKB             += ubloxCKA;
            ubloxProcessDataState = GET_LL;
            break;

        case GET_LL:
            ubloxExpectedDataLength = data;
            ubloxCKA               += data;
            ubloxCKB               += ubloxCKA;
            ubloxProcessDataState   = GET_LH;
            break;

        case GET_LH:
            ubloxExpectedDataLength += data << 8;
            GNSS_cnt                 = 0;
            ubloxCKA                += data;
            ubloxCKB                += ubloxCKA;
            ubloxProcessDataState    = GET_DATA;
            break;

        case GET_DATA:
            ubloxCKA += data;
            ubloxCKB += ubloxCKA;
            if (GNSS_cnt < sizeof(GNSSbuf))
                GNSSbuf[GNSS_cnt++] = data;
            if ((--ubloxExpectedDataLength) == 0)
                ubloxProcessDataState = GET_CKA;
            break;

        case GET_CKA:
            if (ubloxCKA != data)
                ubloxProcessDataState = WAIT_SYNC1;
            else
                ubloxProcessDataState = GET_CKB;
            break;

        case GET_CKB:
            if (ubloxCKB == data)
                parsed = 1;
            ubloxProcessDataState = WAIT_SYNC1;
            break;
    }

    return parsed;
}

/* ------ END -----------  https://github.com/Black-Thunder/FPV-Tracker */

static boolean GNSS_probe()
{
    unsigned long startTime = millis();
    char          c1, c2;
    c1 = c2 = 0;

    // clean any leftovers
    swSer.flush();

    // Serial.println(F("INFO: Waiting for NMEA data from GNSS module..."));

    // Timeout if no valid response in 3 seconds
    while (millis() - startTime < 3000) {
        if (swSer.available() > 0)
        {
            c1 = swSer.read();
            if ((c1 == '$') && (c2 == 0))
            {
                c2 = c1;
                continue;
            }
            if ((c2 == '$') && (c1 == 'G'))
                /* got $G */

                /* leave the function with GNSS port opened */
                return true;
            else
                c2 = 0;
        }

        delay(1);
    }

    return false;
}

static byte GNSS_version()
{
    byte          rval      = GNSS_MODULE_NMEA;
    unsigned long startTime = millis();

    uint8_t msglen = makeUBXCFG(0x0A, 0x04, 0, NULL); // MON-VER
    sendUBX(GNSSbuf, msglen);

    // Get the message back from the GPS
    GNSS_DEBUG_PRINT(F(" * Reading response: "));

    while ((millis() - startTime) < 2000) {
        if (swSer.available())
        {
            unsigned char c   = swSer.read();
            int           ret = 0;

            GNSS_DEBUG_PRINT(c, HEX);
            ret = ubloxProcessData(c);

            // Upon a successfully parsed sentence, do the version detection
            if (ret)
            {
                if (ubloxClass == 0x0A) // MON
                {
                    if (ubloxId == 0x04) // VER

                    // UBX-MON-VER data description
                    // uBlox 6  - page 166 : https://www.u-blox.com/sites/default/files/products/documents/u-blox6_ReceiverDescrProtSpec_%28GPS.G6-SW-10018%29_Public.pdf
                    // uBlox 7  - page 153 : https://www.u-blox.com/sites/default/files/products/documents/u-blox7-V14_ReceiverDescriptionProtocolSpec_%28GPS.G7-SW-12001%29_Public.pdf
                    // uBlox M8 - page 300 : https://www.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_%28UBX-13003221%29_Public.pdf

                    {
                        Serial.print(F("INFO: GNSS module HW version: "));
                        Serial.println((char *) &GNSSbuf[30]);

                        Serial.print(F("INFO: GNSS module FW version: "));
                        Serial.println((char *) &GNSSbuf[0]);

#ifdef DO_GNSS_DEBUG
                        for (unsigned i = 30 + 10; i < GNSS_cnt; i+=30) {
                            Serial.print(F("INFO: GNSS module extension: "));
                            Serial.println((char *) &GNSSbuf[i]);
                        }
#endif

                        if (GNSSbuf[33] == '4')
                            rval = GNSS_MODULE_U6;
                        else if (GNSSbuf[33] == '7')
                            rval = GNSS_MODULE_U7;
                        else if (GNSSbuf[33] == '8')
                            rval = GNSS_MODULE_U8;

                        break;
                    }
                }
            }
        }
    }

    return rval;
}

byte GNSS_setup()
{
    byte rval = GNSS_MODULE_NONE;

    SoC->swSer_begin(SERIAL_IN_BR);

    if (hw_info.model == SOFTRF_MODEL_PRIME_MK2 ||
        hw_info.model == SOFTRF_MODEL_UNI)
    {
        // power on by wakeup call
        swSer.write((uint8_t) 0);
        swSer.flush();
        delay(500);
    }

    if (!GNSS_probe())
        return rval;

    rval = GNSS_MODULE_NMEA;

    if (hw_info.model == SOFTRF_MODEL_PRIME_MK2 ||
        hw_info.model == SOFTRF_MODEL_RASPBERRY ||
        hw_info.model == SOFTRF_MODEL_UNI)
    {
        rval = GNSS_version();

        if (rval == GNSS_MODULE_U6 ||
            rval == GNSS_MODULE_U7 ||
            rval == GNSS_MODULE_U8)

            // Set the navigation mode (Airborne, 1G)
            // Turning off some GPS NMEA sentences on the uBlox modules
            setup_UBX();
        else
            setup_NMEA();
    }

    if (SOC_GPIO_PIN_GNSS_PPS != SOC_UNUSED_PIN)
    {
        pinMode(SOC_GPIO_PIN_GNSS_PPS, SOC_GPIO_PIN_MODE_PULLDOWN);
        attachInterrupt(digitalPinToInterrupt(SOC_GPIO_PIN_GNSS_PPS),
                        SoC->GNSS_PPS_handler, RISING);
    }

    return rval;
}

void GNSS_sleep()
{
    for (int i = 0; i < sizeof(CFG_RST); i++)
        swSer.write(pgm_read_byte(&CFG_RST[i]));
    delay(600);

    for (int i = 0; i < sizeof(RXM_PMREQ); i++)
        swSer.write(pgm_read_byte(&RXM_PMREQ[i]));
}

void GNSS_wakeup()
{
    for (int i = 0; i < 20; i++) //send random to trigger respose
        swSer.write(0xFF);
}

void GNSS_loop(bool TimeSync)
{
    PickGNSSFix();

    if (!TimeSync)
        GNSSTimeSync();

#if defined(USE_GNSS_PSM)
    if (settings->power_save & POWER_SAVE_GNSS)
        if (hw_info.model == SOFTRF_MODEL_UNI)
            if (hw_info.gnss == GNSS_MODULE_U6 ||
                hw_info.gnss == GNSS_MODULE_U7 ||
                hw_info.gnss == GNSS_MODULE_U8)
            {
                if (!gnss_psm_active && isValidGNSSFix() && gnss.satellites.value() > 5)
                {
                    // Setup for Power Save Mode (Default Cyclic 1s)
                    for (int i = 0; i < sizeof(RXM_PSM); i++)
                        swSer.write(pgm_read_byte(&RXM_PSM[i]));

                    GNSS_DEBUG_PRINTLN(F("INFO: GNSS Power Save Mode"));
                    gnss_psm_active = true;
                }
                else if (gnss_psm_active &&
                         ((gnss.satellites.isValid() && gnss.satellites.value() <= 5) ||
                          gnss.satellites.age() > NMEA_EXP_TIME))
                {
                    // Setup for Continuous Mode
                    for (int i = 0; i < sizeof(RXM_MAXP); i++)
                        swSer.write(pgm_read_byte(&RXM_MAXP[i]));

                    GNSS_DEBUG_PRINTLN(F("INFO: GNSS Continuous Mode"));
                    gnss_psm_active = false;
                }
            }

#endif /* USE_GNSS_PSM */
}

void GNSS_fini()
{
    if (hw_info.model == SOFTRF_MODEL_PRIME_MK2 ||
        hw_info.model == SOFTRF_MODEL_UNI)
        if (hw_info.gnss == GNSS_MODULE_U6 ||
            hw_info.gnss == GNSS_MODULE_U7 ||
            hw_info.gnss == GNSS_MODULE_U8)
        {
            // Controlled Software reset
            for (int i = 0; i < sizeof(CFG_RST); i++)
                swSer.write(pgm_read_byte(&CFG_RST[i]));

            delay(hw_info.gnss == GNSS_MODULE_U8 ? 1000 : 600);

            // power off until wakeup call
            for (int i = 0; i < sizeof(RXM_PMREQ_OFF); i++)
                swSer.write(pgm_read_byte(&RXM_PMREQ_OFF[i]));
        }
}

void GNSSTimeSync()
{
    if (GNSSTimeSyncMarker == 0 && gnss.time.isValid() && gnss.time.isUpdated())
    {
        setTime(gnss.time.hour(), gnss.time.minute(), gnss.time.second(), gnss.date.day(), gnss.date.month(), gnss.date.year());
        GNSSTimeSyncMarker = millis();
    }
    else
    {
        if ((millis() - GNSSTimeSyncMarker > 60000) /* 1m */ && gnss.time.isValid() &&
            gnss.time.isUpdated() && (gnss.time.age() <= 1000) /* 1s */)
        {
  #if 0
            Serial.print("Valid: ");
            Serial.println(gnss.time.isValid());
            Serial.print("isUpdated: ");
            Serial.println(gnss.time.isUpdated());
            Serial.print("age: ");
            Serial.println(gnss.time.age());
  #endif
            setTime(gnss.time.hour(), gnss.time.minute(), gnss.time.second(), gnss.date.day(), gnss.date.month(), gnss.date.year());
            GNSSTimeSyncMarker = millis();
        }
    }
}

void PickGNSSFix()
{

}

#if !defined(EXCLUDE_EGM96)
/*
 *  Algorithm of EGM96 geoid offset approximation was taken from XCSoar
 */

static float AsBearing(float angle)
{
    float retval = angle;

    while (retval < 0) {
        retval += 360.0;
    }

    while (retval >= 360.0) {
        retval -= 360.0;
    }

    return retval;
}

int LookupSeparation(float lat, float lon)
{
    int ilat, ilon;

    ilat = round((90.0 - lat) / 2.0);
    ilon = round(AsBearing(lon) / 2.0);

    int offset = ilat * 180 + ilon;

    if (offset >= egm96s_dem_len)
        return 0;

    if (offset < 0)
        return 0;

    return (int) pgm_read_byte(&egm96s_dem[offset]) - 127;
}

#endif /* EXCLUDE_EGM96 */
