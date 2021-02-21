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
#include <tinyxml2.h>

using namespace tinyxml2;

 
static bool OGN_save_config() {

    SPIFFS.remove("/config.xml");

    File configFile = SPIFFS.open("/config.xml", "w");
    if (!configFile)
    {
        Serial.println(F("Failed to open config.txt for writing"));
        return false;
    }
   
}

bool OGN_read_config() {

	if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return false;
    }
   
    File file = SPIFFS.open("/config.xml", "r");
    if (!file)
    {
        Serial.println("An Error has occurred while opening index.html");
        return false;
    }
    
    size_t filesize   = file.size();
    char * config_file = (char *) malloc(filesize + 1);

    file.read((uint8_t *)config_file, filesize);
    config_file[filesize+1] = '\0';

    XMLDocument xmlDocument;

    /*
    if(xmlDocument.Parse(values)!= XML_SUCCESS){
      Serial.println("Error parsing");  
      return false;
    };

    XMLElement * pRootElement = xmlDocument.RootElement();

    if (NULL != pRootElement) {
      XMLElement * pWifi = pRootElement->FirstChildElement("wifi");
      
      while (pWifi) {
      const char* value = pWifi->Attribute( "ssid" );
      Serial.println(value);
      
      value = pWifi->Attribute( "pass" );
      Serial.println(value);
      }
      
    }*/

    free(config_file);
    return true;   
}
