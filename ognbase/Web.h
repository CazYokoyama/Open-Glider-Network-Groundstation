#ifndef WEBHELPER_H
#define WEBHELPER_H

#include "SoC.h"

#if defined(ARDUINO) && !defined(EXCLUDE_WIFI)
#include <WiFiClient.h>
#endif /* ARDUINO */

#include <TinyGPS++.h>

#include "EEPROM.h"
#include "RF.h"

#define BOOL_STR(x) (x ? "true":"false")
#define JS_MAX_CHUNK_SIZE 4096

void Web_setup(void);
void Web_loop(void);
void Web_fini(void);

#if DEBUG
void Hex2Bin(String, byte *);
#endif

extern uint32_t tx_packets_counter, rx_packets_counter;
//extern byte TxBuffer[PKT_SIZE];
extern String TxDataTemplate;

#if defined(ARDUINO) && !defined(EXCLUDE_WIFI)
extern WiFiClient client;
#endif /* ARDUINO */

extern TinyGPSPlus gps;

#endif /* WEBHELPER_H */
