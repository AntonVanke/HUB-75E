#pragma once
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include "AppManifest.h"
#include "UpdateTrust.h"
#include <functional>
#include <atomic>
class FirmwareUpdate {
  uint32_t checked_=0;bool checkedOnce_=false;
  std::atomic<bool> checkDone_{false};
  String current_,checkState_,version_,path_,hash_;uint32_t bytes_=0;
  void check(){
#ifdef HUB75_S3_BOARD
    const char* chip="esp32s3";
#else
    const char* chip="esp32";
#endif
    WiFiClientSecure tls;tls.setCACert(UPDATE_ROOT_CA);tls.setHandshakeTimeout(12);
    HTTPClient http;http.setConnectTimeout(8000);http.setTimeout(10000);http.useHTTP10(true);
    checkState_="manifest-download-failed";
    if(http.begin(tls,String("https://antonvanke.github.io/HUB-75E/updates/")+chip+".json")&&http.GET()==200&&http.getSize()>0&&http.getSize()<=2048){
      String json=http.getString(),target;uint32_t flash=0,psram=0;
      checkState_="invalid-manifest";
      if(field(json,"version",version_)&&field(json,"chip",target)&&target==chip&&field(json,"path",path_)&&field(json,"sha256",hash_)&&number(json,"bytes",bytes_)&&number(json,"flashBytes",flash)&&number(json,"psramBytes",psram)){
        const esp_partition_t* slot=esp_ota_get_next_update_partition(nullptr);
        if(!newer(version_,current_))checkState_="up-to-date";
        else if(!slot||bytes_<256||bytes_>slot->size||flash>ESP.getFlashChipSize()||psram>((ESP.getPsramSize()+1048575)/1048576)*1048576)checkState_="incompatible-hardware";
        else if(!path_.startsWith("firmware/")||path_.indexOf("..")>=0||strspn(path_.c_str(),"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._/")!=path_.length()||hash_.length()!=64||strspn(hash_.c_str(),"0123456789abcdef")!=64)checkState_="invalid-asset";
        else checkState_="available";
      }
    }
    http.end();tls.stop();
  }
  static bool field(const String& json,const char* key,String& value){bool found=false;return CwJson::field(json,key,value,found)&&found;}
  static bool number(const String& json,const char* key,uint32_t& value){String s;return field(json,key,s)&&CwJson::natural(s,value);}
public:
  bool enabled=false,busy=false,available=false;String state="idle";
  bool due(uint32_t now)const{return enabled&&(!checkedOnce_?now>=60000:now-checked_>=21600000);}
  void schedule(){checkedOnce_=false;}
  static bool newer(const String& a,const String& b){unsigned x[3],y[3];int n=0;if(sscanf(a.c_str(),"%u.%u.%u%n",x,x+1,x+2,&n)!=3||n!=int(a.length())||sscanf(b.c_str(),"%u.%u.%u",y,y+1,y+2)!=3)return false;for(int i=0;i<3;i++)if(x[i]!=y[i])return x[i]>y[i];return false;}
  void startCheck(const char* current){
    if(busy)return;checked_=millis();checkedOnce_=true;current_=current;available=false;busy=true;state="checking";
    if(xTaskCreatePinnedToCore([](void* p){auto* self=(FirmwareUpdate*)p;self->check();self->checkDone_.store(true,std::memory_order_release);vTaskDelete(nullptr);},"update-check",12288,this,1,nullptr,0)!=pdPASS){busy=false;state="check-memory-failed";}
  }
  bool finishCheck(){
    if(!checkDone_.exchange(false,std::memory_order_acquire))return false;
    busy=false;state=checkState_;available=state=="available";return true;
  }
  const String& targetVersion()const{return version_;}
  bool install(const std::function<void(uint32_t,uint32_t,const char*)>& progress,const std::function<void()>& tick){
    if(!available)return false;available=false;
#ifdef HUB75_S3_BOARD
    const int chipCode=9;
#else
    const int chipCode=0;
#endif
    const uint32_t bytes=bytes_;const String& hash=hash_;
    WiFiClientSecure tls;tls.setCACert(UPDATE_ROOT_CA);tls.setHandshakeTimeout(12);
    HTTPClient http;http.setConnectTimeout(8000);http.setTimeout(10000);http.useHTTP10(true);
    bool displayed=false;
    auto fail=[&](const char* reason){state=reason;http.end();tls.stop();if(displayed&&progress)progress(0,bytes,"FAILED");return false;};
    state="downloading";
    if(!http.begin(tls,String("https://antonvanke.github.io/HUB-75E/")+path_)||http.GET()!=200||http.getSize()!=int(bytes))return fail("firmware-download-failed");
    uint8_t buffer[1024];auto* stream=http.getStreamPtr();
    if(stream->readBytes(buffer,24)!=24||buffer[0]!=0xe9||(buffer[12]|(buffer[13]<<8))!=chipCode)return fail("wrong-chip-image");
    if(!Update.begin(bytes,U_FLASH))return fail("ota-begin-failed");
    displayed=true;if(progress)progress(0,bytes,"UPDATING");
    mbedtls_sha256_context digest;mbedtls_sha256_init(&digest);mbedtls_sha256_starts_ret(&digest,0);
    uint32_t received=0,last=millis();size_t n=24;bool ok=true;
    while(received<bytes){
      if(tick)tick();
      if(n){mbedtls_sha256_update_ret(&digest,buffer,n);if(Update.write(buffer,n)!=n){ok=false;break;}received+=n;last=millis();if(progress)progress(received,bytes,"UPDATING");}
      if(received==bytes)break;
      n=stream->readBytes(buffer,std::min(size_t(sizeof(buffer)),size_t(bytes-received)));
      if(!n&&(!http.connected()||millis()-last>15000)){ok=false;break;}delay(1);
    }
    unsigned char result[32];char hex[65];mbedtls_sha256_finish_ret(&digest,result);mbedtls_sha256_free(&digest);
    for(int i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",result[i]);
    if(!ok||received!=bytes||hash!=hex){Update.abort();return fail("download-integrity-failed");}
    if(progress)progress(received,bytes,"VERIFY");
    if(!Update.end(true)){Update.abort();return fail("ota-validation-failed");}
    http.end();tls.stop();state="restarting";if(progress)progress(received,bytes,"RESTART");return true;
  }
};
