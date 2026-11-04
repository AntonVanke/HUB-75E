#pragma once
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include "AppManifest.h"
#include "UpdateTrust.h"
class FirmwareUpdate {
  uint32_t checked_=0;bool checkedOnce_=false;
  static bool field(const String& json,const char* key,String& value){bool found=false;return CwJson::field(json,key,value,found)&&found;}
  static bool number(const String& json,const char* key,uint32_t& value){String s;return field(json,key,s)&&CwJson::natural(s,value);}
public:
  bool enabled=false,busy=false;String state="idle";
  bool due(uint32_t now)const{return enabled&&(!checkedOnce_?now>=60000:now-checked_>=21600000);}
  void schedule(){checkedOnce_=false;}
  static bool newer(const String& a,const String& b){unsigned x[3],y[3];int n=0;if(sscanf(a.c_str(),"%u.%u.%u%n",x,x+1,x+2,&n)!=3||n!=int(a.length())||sscanf(b.c_str(),"%u.%u.%u",y,y+1,y+2)!=3)return false;for(int i=0;i<3;i++)if(x[i]!=y[i])return x[i]>y[i];return false;}
  bool run(const char* current){
    checked_=millis();checkedOnce_=true;state="checking";
#ifdef HUB75_S3_BOARD
    const char* chip="esp32s3";const int chipCode=9;
#else
    const char* chip="esp32";const int chipCode=0;
#endif
    const String base="https://antonvanke.github.io/HUB-75E/";
    WiFiClientSecure tls;tls.setCACert(UPDATE_ROOT_CA);tls.setHandshakeTimeout(12);
    HTTPClient http;http.setConnectTimeout(8000);http.setTimeout(10000);http.useHTTP10(true);
    auto fail=[&](const char* reason){state=reason;http.end();tls.stop();return false;};
    if(!http.begin(tls,base+"updates/"+chip+".json"))return fail("connect-failed");
    if(http.GET()!=200||http.getSize()<1||http.getSize()>2048)return fail("manifest-download-failed");
    String json=http.getString();http.end();String version,target,path,hash;uint32_t bytes,flash,psram;
    if(!field(json,"version",version)||!field(json,"chip",target)||target!=chip||!field(json,"path",path)||!field(json,"sha256",hash)||!number(json,"bytes",bytes)||!number(json,"flashBytes",flash)||!number(json,"psramBytes",psram))return fail("invalid-manifest");
    if(!newer(version,current))return fail("up-to-date");
    const esp_partition_t* slot=esp_ota_get_next_update_partition(nullptr);
    if(!slot||bytes<256||bytes>slot->size||flash>ESP.getFlashChipSize()||psram>((ESP.getPsramSize()+1048575)/1048576)*1048576)return fail("incompatible-hardware");
    if(!path.startsWith("firmware/")||path.indexOf("..")>=0||strspn(path.c_str(),"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._/")!=path.length()||hash.length()!=64||strspn(hash.c_str(),"0123456789abcdef")!=64)return fail("invalid-asset");
    state="downloading";
    if(!http.begin(tls,base+path)||http.GET()!=200||http.getSize()!=int(bytes))return fail("firmware-download-failed");
    uint8_t buffer[1024];auto* stream=http.getStreamPtr();
    if(stream->readBytes(buffer,24)!=24||buffer[0]!=0xe9||(buffer[12]|(buffer[13]<<8))!=chipCode)return fail("wrong-chip-image");
    if(!Update.begin(bytes,U_FLASH))return fail("ota-begin-failed");
    mbedtls_sha256_context digest;mbedtls_sha256_init(&digest);mbedtls_sha256_starts_ret(&digest,0);
    uint32_t received=0,last=millis();size_t n=24;bool ok=true;
    while(received<bytes){
      if(n){mbedtls_sha256_update_ret(&digest,buffer,n);if(Update.write(buffer,n)!=n){ok=false;break;}received+=n;last=millis();}
      if(received==bytes)break;
      n=stream->readBytes(buffer,std::min(size_t(sizeof(buffer)),size_t(bytes-received)));
      if(!n&&(!http.connected()||millis()-last>15000)){ok=false;break;}delay(1);
    }
    unsigned char result[32];char hex[65];mbedtls_sha256_finish_ret(&digest,result);mbedtls_sha256_free(&digest);
    for(int i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",result[i]);
    if(!ok||received!=bytes||hash!=hex){Update.abort();return fail("download-integrity-failed");}
    if(!Update.end(true)){Update.abort();return fail("ota-validation-failed");}
    http.end();tls.stop();state="restarting";return true;
  }
};
