/*
 MIT License

Copyright (c) 2018 IntelliTrend GmbH, http://www.intellitrend.de

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include <string>
#include <ArduinoJson.h>
#include "zabbixSender.h"

ZabbixSender::ZabbixSender() {
}

ZabbixSender::ZabbixSender(const ZabbixSender& orig) {
}

ZabbixSender::~ZabbixSender() {
}

// String createPayload(const char *hostname, float voltage, float rssi, int uptime, int gnss, int timestamp, int lrx);

String ZabbixSender::createPayload(const char *hostname, float voltage, float rssi, int uptime, int gnss, int timestamp, int lrx) {
    StaticJsonDocument<1024> root;
    
    root["request"] = "sender data";
    
    JsonArray data = root.createNestedArray("data");
    
    JsonObject data_0 = data.createNestedObject();
    data_0["host"] = hostname;
    data_0["key"] = "voltage";
    data_0["value"] = voltage;


    JsonObject data_1 = data.createNestedObject();
    data_1["host"] = hostname;
    data_1["key"] = "rssi";
    data_1["value"] = rssi;

    JsonObject data_2 = data.createNestedObject();
    data_2["host"] = hostname;
    data_2["key"] = "uptime";
    data_2["value"] = uptime;

    JsonObject data_3 = data.createNestedObject();
    data_3["host"] = hostname;
    data_3["key"] = "gnss";
    data_3["value"] = gnss;

    JsonObject data_4 = data.createNestedObject();
    data_4["host"] = hostname;
    data_4["key"] = "timestamp";
    data_4["value"] = timestamp;

    JsonObject data_5 = data.createNestedObject();
    data_5["host"] = hostname;
    data_5["key"] = "maxrange";
    data_5["value"] = lrx;
    
    
    String json;
    serializeJson(root, json);
    return json;
}

/**
 * Create message to send to zabbix based on payload
 */
String ZabbixSender::createMessage(String jsonPayload) {
    String header = "ZBXD\x01";
    // we will use only 2 bytes, will never exceed length = 65565
    String placeholderLen = "12345678";  
    String msg = header + placeholderLen + jsonPayload;
    uint16_t len = jsonPayload.length();
         
    // patch the string
    char zNull = '\x00';
    unsigned char lenLsb = (unsigned char) (len & 0xFF);
    unsigned char lenMsb = (unsigned char) ((len >> 8)& 0xFF);  
  
    msg.setCharAt(5, lenLsb);
    msg.setCharAt(6, lenMsb);
    // len padding
    msg.setCharAt(7, zNull);
    msg.setCharAt(8, zNull);
    msg.setCharAt(9, zNull);
    msg.setCharAt(10, zNull);
    msg.setCharAt(11, zNull);
    msg.setCharAt(12, zNull);      
     
    return msg;
}

