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
#define MAX_PACKET_TRACKING MAX_TRACKING_OBJECTS*2
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

  for(uint8_t i=0; i<MAX_PACKET_TRACKING; i++){
    if(addr == ac[i].addr){
      Serial.println("ADDR found in database..");
      int dist = distance(lat1, lon1, ac[i].latitude, ac[i].longitude);
      Serial.print("transmitted distance: ");
      Serial.println(dist);
      int calcdist = calcMaxDistance(speed, timestamp - ac[i].timestamp);
      if(dist > calcdist){
        Serial.println("Packet seems to be invalid");
        cleanUpPacket(i);
        return false;
      }
      else{
        Serial.println("Packet seems to be valid");
        cleanUpPacket(i);
        shiftPackets();
        appendPacket(addr, lat1, lon1, timestamp);
        return true;
      }
    }
  Serial.println("ADDR not found in database, appending");
  shiftPackets();
  appendPacket(addr, lat1, lon1, timestamp);
  return(false);
 }
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
    Serial.print("Real distance: ");
    Serial.println(dist);  
    return (int)(dist);
  }

static int calcMaxDistance(double speed, time_t time_diff) {
  return (int)((speed / 3.6 * time_diff) * 2 );
  }  

static bool appendPacket(uint32_t addr, double lat, double lon, time_t timestamp){
  ac[MAX_PACKET_TRACKING-1].timestamp = timestamp;
  ac[MAX_PACKET_TRACKING-1].addr = addr;
  ac[MAX_PACKET_TRACKING-1].latitude = lat;
  ac[MAX_PACKET_TRACKING-1].longitude = lon;
  //ac[MAX_PACKET_TRACKING-1].speed
}

static void shiftPackets(void){
  for(uint8_t i=0; i<MAX_PACKET_TRACKING-1; i++){
    ac[i].timestamp = ac[i+1].timestamp;
    ac[i].addr = ac[i+1].addr;
    ac[i].latitude = ac[i+1].latitude;
    ac[i].longitude = ac[i+1].longitude;
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
