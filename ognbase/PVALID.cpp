#include "SoftRF.h"
#include "Traffic.h"
#include "RF.h"
#include "global.h"
#include "Log.h"
#include "OLED.h"
#include "global.h"
#include "PVALID.h"

#include <math.h>

#define pi 3.14159265358979323846
#define PACKET_EXPIRY_TIME 10

//time_t timestamp;
//uint32_t addr;
//float latitude;
//float longitude;
//float altitude;
//float course;
//float speed;


//aircrafts_t ac, ai[MAX_TRACKING_OBJECTS], EmptyFO;

aircrafts_t ac[MAX_TRACKING_OBJECTS];


bool isPacketValid(uint32_t addr, double lat1, double lon1, double speed, time_t timestamp) {
  double theta, dist;
  String msg;

  /*V0.1.0-24*/

  if (!testmode_enable){
    return true;
  }

  
  for(uint8_t i=0;i<MAX_TRACKING_OBJECTS;i++){
    msg = "checking database: ";
    msg += String(ac[i].addr, HEX);
    msg += " CNT: ";
    msg += String(ac[i].pkt_counter);
    Logger_send_udp(&msg);  
  } 
  for(uint8_t i=0; i<MAX_TRACKING_OBJECTS;i++){

    if(addr == ac[i].addr){
      
      msg = "ADDR found in database..";
      Logger_send_udp(&msg);
      
      int dist = distance(lat1, lon1, ac[i].latitude, ac[i].longitude);
      
      msg = "Transmitted distance: ";
      msg += dist;
      Logger_send_udp(&msg);
      
      int calcdist = calcMaxDistance(speed, timestamp - ac[i].timestamp);
      if(dist > calcdist){

        msg = "Max calculated distance: ";
        msg += calcdist;
        Logger_send_udp(&msg);        
        
        msg = "Packet seems to be invalid";
        Logger_send_udp(&msg);

        cleanUpPacket(i);
        shiftPackets();
        return false;
      }
      else{

        msg = "Max calculated distance: ";
        msg += calcdist;
        Logger_send_udp(&msg);        
        

        uint8_t cnt = ac[i].pkt_counter + 1;

        msg = "Packet seems to be valid - counter: ";
        msg += cnt;
        Logger_send_udp(&msg);        
        
        cleanUpPacket(i);
        shiftPackets();
        appendPacket(addr, lat1, lon1, timestamp, cnt);
        return true;
      }
    }
  }
  msg = "Address not found, appending";
  Logger_send_udp(&msg);
  shiftPackets();
  appendPacket(addr, lat1, lon1, timestamp, 0);
  return(false);
}


static int distance(double lat1, double lon1, double lat2, double lon2) {
  double theta, dist;
  if ((lat1 == lat2) && (lon1 == lon2)) {
    return 0;
  }
  else {
    theta = lon1 - lon2;
    dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
    dist = acos(dist);
    dist = rad2deg(dist);
    dist = dist * 60 * 1.1515;
    dist = dist * 1.609344;
    }

    return (int)(dist);
  }

static int calcMaxDistance(double speed, time_t time_diff) {
  return (int)((speed / 3.6 * time_diff) * 2 );
  }  

static bool appendPacket(uint32_t addr, double lat, double lon, time_t timestamp, uint8_t cnt){
  ac[MAX_TRACKING_OBJECTS-1].timestamp = timestamp;
  ac[MAX_TRACKING_OBJECTS-1].addr = addr;
  ac[MAX_TRACKING_OBJECTS-1].latitude = lat;
  ac[MAX_TRACKING_OBJECTS-1].longitude = lon;
  ac[MAX_TRACKING_OBJECTS-1].pkt_counter = cnt;
}

static void shiftPackets(void){
  for(uint8_t i=0; i<MAX_TRACKING_OBJECTS-1; i++){
    ac[i].timestamp = ac[i+1].timestamp;
    ac[i].addr = ac[i+1].addr;
    ac[i].latitude = ac[i+1].latitude;
    ac[i].longitude = ac[i+1].longitude;
    ac[i].pkt_counter = ac[i+1].pkt_counter;
  }
}

static void cleanUpPacket(uint8_t index){
  ac[index].addr = 0x00000000;
}

static double deg2rad(double deg) {
  return (deg * pi / 180);
}

static double rad2deg(double rad) {
  return (rad * 180 / pi);
}
