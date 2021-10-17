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

#include "SoftRF.h"
#include "WiFi.h"
#include "Traffic.h"
#include "AsyncUDP.h"
#include "EEPROM.h"
#include "RF.h"
#include "global.h"
#include "Web.h"
#include "Log.h"
#include "PNET.h"


#include "ogn_service_generated.h"

WiFiUDP udp;

using namespace ogn;


bool RSM_Setup(int port)
{
    if (udp.begin(port))
    {
        String status = "starting RSM server..";
        Logger_send_udp(&status);
        return true;
    }
    String status = "cannot start RSM server..";
    Logger_send_udp(&status);
    return false;
}

// AircraftPos(int32_t _callsign, int32_t _timestamp, int8_t _type, bool _stealth, bool _notrack, int32_t _heading, int32_t _speed, int32_t _lat, int32_t _lon, int32_t _alt)

void RSM_ExportAircraftPosition() {
  
    flatbuffers::FlatBufferBuilder builder;

     time_t this_moment = now();
    
    for (int i = 0; i < MAX_TRACKING_OBJECTS; i++)
        if (Container[i].addr && (this_moment - Container[i].timestamp) <= EXPORT_EXPIRATION_TIME && Container[i].distance < ogn_range * 1000)
        {    
          auto AircPosition = AircraftPos( Container[i].addr,
                                           Container[i].timestamp,
                                           Container[i].aircraft_type,
                                           Container[i].stealth,
                                           Container[i].no_track,
                                           Container[i].course,
                                           Container[i].speed,
                                           Container[i].latitude,
                                           Container[i].longitude,
                                           Container[i].altitude);
                                           
          auto message = CreateOneMessage(builder, &AircPosition);                                                                                                                                                                 
          builder.Finish(message);
          
          auto ptr = builder.GetBufferPointer();
          auto size = builder.GetSize();

          //next release
          if(false){
            
            char *encrypted;
            size_t encrypted_len;
            
            PNETencrypt(ptr, size, &encrypted, &encrypted_len);
            SoC->WiFi_transmit_UDP(new_protocol_server.c_str(), new_protocol_port, (byte*)encrypted, encrypted_len); 
        
          }
      
          else{
           SoC->WiFi_transmit_UDP(new_protocol_server.c_str(), new_protocol_port, ptr, size);  
          }                   
        }
}

void RSM_receiver()
{
//ToDo....
}
