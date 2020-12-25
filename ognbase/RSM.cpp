/*
   RSM.cpp
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
#include "ogn_service.pb.h"
#include "pb_common.h"
#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"

#include "SoftRF.h"
#include "WiFi.h"
#include "AsyncUDP.h"
#include "EEPROM.h"
#include "RF.h"
#include "global.h"
#include "Web.h"

WiFiUDP udp;

static void RSM_DEBUG(String* buf)
{
    int  debug_len = buf->length() + 1;
    byte debug_msg[debug_len];
    buf->getBytes(debug_msg, debug_len);
    SoC->WiFi_transmit_UDP_debug(settings->ogndebugp + 1, debug_msg, debug_len);
}

static bool RSM_transmit(uint8_t* msg_buffer, size_t msg_size)
{
//in progress
}

bool RSM_Setup(int port)
{
    if (udp.begin(port))
    {
        String status = "starting RSM server..";
        RSM_DEBUG(&status);
        return true;
    }
    String status = "cannot start RSM server..";
    RSM_DEBUG(&status);
    return false;
}

void RSM_receiver()
{
    int packetSize = udp.parsePacket();
    int msgSize    = 0;

    if (packetSize)
    {
        uint8_t buffer[packetSize];
        udp.read(buffer, packetSize);

        while (packetSize) {
            uint8_t msg_buffer[buffer[msgSize]];

            for (int i = 0; i < buffer[msgSize]; i++)
                msg_buffer[i] = buffer[i + msgSize + 1];

            OneMessage   message = OneMessage_init_zero;
            pb_istream_t stream  = pb_istream_from_buffer(msg_buffer, buffer[msgSize]);
            int          status  = pb_decode(&stream, OneMessage_fields, &message);

            //RSM_transmit(msg_buffer, buffer[msgSize]);
            if (message.has_fanetService)
                Serial.println("found fanet service");
            if (message.has_receiverConfiguration)
            {
                Serial.println("found receiver configuration");


                if (message.receiverConfiguration.has_maxrange)
                    settings->range = message.receiverConfiguration.maxrange;
                if (message.receiverConfiguration.has_band)
                {
                    settings->band = message.receiverConfiguration.band;
                    Serial.println("setting band");
                }
                if (message.receiverConfiguration.has_protocol)
                {
                    settings->rf_protocol = message.receiverConfiguration.protocol;
                    Serial.println("setting protocol");
                }
                if (message.receiverConfiguration.has_aprsd)
                {
                    settings->ogndebug = message.receiverConfiguration.aprsd;
                    Serial.println("enable aprs debugging");
                }
                if (message.receiverConfiguration.has_aprsp)
                {
                    settings->ogndebugp = message.receiverConfiguration.aprsp;
                    Serial.println("change aprs debug port");
                }
                if (message.receiverConfiguration.has_itrackb)
                {
                    settings->ignore_no_track = message.receiverConfiguration.itrackb;
                    Serial.println("ignore track bit set");
                }
                if (message.receiverConfiguration.has_istealthb)
                {
                    settings->ignore_stealth = message.receiverConfiguration.istealthb;
                    Serial.println("ignore stealth bit set");
                }
                if (message.receiverConfiguration.has_sleepm)
                {
                    settings->sleep_mode = message.receiverConfiguration.sleepm;
                    Serial.println("enabling sleep mode");
                }
                if (message.receiverConfiguration.has_rxidle)
                {
                    settings->sleep_after_rx_idle = message.receiverConfiguration.rxidle;
                    Serial.println("enable sleep after rx idle");
                }
                if (message.receiverConfiguration.has_wakeup)
                {
                    settings->wake_up_timer = message.receiverConfiguration.wakeup;
                    Serial.println("setting wakeup timer value");
                }
                if (message.receiverConfiguration.has_reset)
                    if (message.receiverConfiguration.reset)
                    {
                        Serial.println("reset");
                        SPIFFS.format();
                        RF_Shutdown();
                        delay(1000);
                        SoC->reset();
                    }
                if (message.receiverConfiguration.has_reboot)
                    if (message.receiverConfiguration.reboot)
                    {
                        Serial.println("rebooting");
                        EEPROM_store();
                        RF_Shutdown();
                        delay(2000);
                        SoC->reset();
                    }
                Serial.println("store eeprom values");
                EEPROM_store();
            }

            msgSize    = msgSize + buffer[msgSize];
            packetSize = packetSize - msgSize - 1;
        }
    }
}
