#include "SoftRF.h"
#include "Traffic.h"
#include "RF.h"
#include "global.h"
#include "Log.h"
#include "OLED.h"
#include "global.h"
#include "PNET.h"

#include "AESLib.h"


byte aes_key[N_BLOCK] = { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };
byte aes_iv[N_BLOCK] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


AESLib aesLib;

void aes_init() {
    File keyFile;

    if (SPIFFS.exists("/key.bin")){
      keyFile = SPIFFS.open("/key.bin");
      if (!keyFile)
      {
          Serial.println(F("Failed to open key file, using default key!."));
      }      
    }
    else{
      Serial.println(F("Failed to open key file, using default key!."));
    }
    if(keyFile.size() >= N_BLOCK){
          byte buffer[N_BLOCK];
          keyFile.read(buffer,N_BLOCK);
      for(size_t i=0;i<N_BLOCK;i++){
        aes_key[i] = buffer[i];
      }
      keyFile.close();
    }

    if (SPIFFS.exists("/iv.bin")){
      keyFile = SPIFFS.open("/iv.bin");
      if (!keyFile)
      {
          Serial.println(F("Failed to open iv file, using default key!."));
      }      
    }
    if(keyFile.size() >= N_BLOCK){
          byte buffer[N_BLOCK];
          keyFile.read(buffer,N_BLOCK);
      for(size_t i=0;i<N_BLOCK;i++){
        aes_iv[i] = buffer[i];
      }
    }
    else{
      Serial.println(F("IV file wrong size, using default key!."));
    }    

    Serial.println(F("using AES key"));
    for(size_t i=0;i<N_BLOCK;i++){
      Serial.print(aes_key[i], HEX);
    }
    Serial.println();
    Serial.println(F("using AES iv"));
    for(size_t i=0;i<N_BLOCK;i++){
      Serial.print(aes_iv[i], HEX);
    }
    Serial.println();
        
    aesLib.gen_iv(aes_iv);
    aesLib.set_paddingmode((paddingMode)0);
}

//unsigned char cleartext[INPUT_BUFFER_LIMIT] = {0};
//unsigned char ciphertext[2*INPUT_BUFFER_LIMIT] = {0};

void PNETencrypt(unsigned char msg[],size_t msgLen, char **arr, size_t *arr_len) {
  if(1){
    char *encrypted = (char*)malloc(64);
    uint16_t enclen = aesLib.encrypt(msg, msgLen, encrypted , aes_key, sizeof(aes_key), aes_iv);
    *arr = encrypted;
    *arr_len = enclen;
  }
  else{
   *arr_len = 0; 
  }
}

void PNETdecrypt(unsigned char msg[],size_t msgLen, char **arr, size_t *arr_len) {
  if(1){
  char *decrypted = (char*)malloc(64);
  uint16_t declen = aesLib.decrypt(msg, msgLen, decrypted , aes_key, sizeof(aes_key), aes_iv);
  *arr = decrypted;
  *arr_len = declen;
  }
  else{
   *arr_len = 0; 
  }  
}
