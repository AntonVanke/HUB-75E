#pragma once
#include <stdio.h>
#include "qr/qrcodegen.h"

class SetupQr {
  uint8_t code_[qrcodegen_BUFFER_LEN_FOR_VERSION(3)]={};
  bool ready_=false;
public:
  // The device-generated SSID and setup password contain only ASCII letters,
  // digits and '-'; no Wi-Fi QR field escaping is needed for these values.
  bool begin(const char* ssid,const char* password){
    char text[80];uint8_t temp[sizeof(code_)];
    const int n=snprintf(text,sizeof(text),"WIFI:T:WPA;S:%s;P:%s;;",ssid,password);
    ready_=n>0&&size_t(n)<sizeof(text)&&qrcodegen_encodeText(text,temp,code_,qrcodegen_Ecc_LOW,3,3,qrcodegen_Mask_AUTO,true);
    return ready_;
  }
  int size()const{return ready_?qrcodegen_getSize(code_):0;}
  bool dark(int x,int y)const{return ready_&&qrcodegen_getModule(code_,x,y);}
};
