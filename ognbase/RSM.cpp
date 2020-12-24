/*
 * RSM.cpp
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
#include "ogn_service.pb.h"
#include "pb_common.h"
#include "pb.h"
#include "pb_encode.h"
#include <pb_decode.h>

#include "SoftRF.h"
#include "WiFi.h"
#include "AsyncUDP.h"
#include "EEPROM.h"
#include "RF.h"


WiFiUDP udp;

static void RSM_DEBUG(String* buf)
{
  int  debug_len = buf->length() + 1;
  byte debug_msg[debug_len];
  buf->getBytes(debug_msg, debug_len);
  SoC->WiFi_transmit_UDP_debug(settings->ogndebugp+1, debug_msg, debug_len);
}

static bool RSM_transmit(uint8_t* msg_buffer, size_t msg_size){

  UnionMessage message = UnionMessage_init_zero;
  pb_istream_t stream = pb_istream_from_buffer(msg_buffer, msg_size);
  int status = pb_decode(&stream, UnionMessage_fields, &message);

  int base_proto = settings->rf_protocol;               // save state
  settings->rf_protocol = RF_PROTOCOL_FANET;            // set FANET protocol
  RF_setup();                                           // setup RF chip
  RF_Transmit(RF_Encode_Fanet_s(&ThisAircraft), false); // trasmit packet ThisArcraft anpassen
  settings->rf_protocol = base_proto;                   // restore old state
  RF_setup();                                          // setup RF chip

  String state = "service packet transmited";
  RSM_DEBUG(&state);
}

bool RSM_Setup (int port)
{

  if (udp.begin(port)){
    String status = "starting RSM server..";
    RSM_DEBUG(&status);
    return true;
  }
  String status = "cannot start RSM server..";
  RSM_DEBUG(&status);
  return false;
}

void RSM_receiver(){

  int packetSize = udp.parsePacket();
  int msgSize = 0;

  if (packetSize){
    uint8_t buffer[packetSize];
    udp.read(buffer, packetSize);

    while ( packetSize ){
      uint8_t msg_buffer[buffer[msgSize]];
      
      for(int i=0;i<buffer[msgSize];i++){
        msg_buffer[i] = buffer[i+msgSize+1];
      }

      RSM_transmit(msg_buffer, buffer[msgSize]);
      
      msgSize = msgSize + buffer[msgSize];
      packetSize = packetSize - msgSize - 1;
    }

  }
}
