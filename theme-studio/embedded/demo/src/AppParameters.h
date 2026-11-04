#pragma once
#include "AppManifest.h"
#include <cmath>

// Parsed once when loading an app; render callbacks only read these scalars.
struct AppParameters {
  struct Entry {char key[24];float value,min,max,step;};
  Entry entries[12]{};int count=0;
  static bool number(const String& json,const char* key,float& value){
    String raw;bool found;char kind;
    if(!CwJson::field(json,key,raw,found,&kind)||!found||kind=='"')return false;
    char* end;value=strtof(raw.c_str(),&end);return !*end&&std::isfinite(value);
  }
  bool parse(const String& schema,const String& values="{}",bool strict=false){
    count=0;const char* p=schema.c_str();CwJson::space(p);if(*p++!='{')return false;CwJson::space(p);
    while(*p&&*p!='}'){
      String key;if(count==12||!CwJson::string(p,key)||key.isEmpty()||key.length()>23||strspn(key.c_str(),"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_")!=key.length())return false;
      for(int i=0;i<count;i++)if(key==entries[i].key)return false;
      CwJson::space(p);if(*p++!=':')return false;CwJson::space(p);const char* start=p;if(*p!='{'||!CwJson::skip(p))return false;
      String definition;definition.concat(start,p-start);auto& e=entries[count++];strcpy(e.key,key.c_str());
      if(!number(definition,"min",e.min)||!number(definition,"max",e.max)||!number(definition,"step",e.step)||!number(definition,"default",e.value)||e.min>e.max||e.step<=0||e.value<e.min||e.value>e.max)return false;
      String raw;bool found;if(!CwJson::field(values,e.key,raw,found))return false;
      float v;if(found){if(!number(values,e.key,v)||v<e.min||v>e.max){if(strict)return false;}else e.value=e.min+roundf((v-e.min)/e.step)*e.step;}
      e.value=fmaxf(e.min,fminf(e.max,e.value));CwJson::space(p);if(*p=='}')break;if(*p++!=',')return false;CwJson::space(p);
    }
    if(*p++!='}')return false;CwJson::space(p);if(*p)return false;
    if(strict){p=values.c_str();CwJson::space(p);if(*p++!='{')return false;CwJson::space(p);int seen=0;
      while(*p&&*p!='}'){String key;if(!CwJson::string(p,key))return false;float value;if(!get(key.c_str(),value)||++seen>count)return false;CwJson::space(p);if(*p++!=':'||!CwJson::skip(p))return false;CwJson::space(p);if(*p=='}')break;if(*p++!=',')return false;CwJson::space(p);}
      if(*p++!='}')return false;CwJson::space(p);if(*p)return false;
    }return true;
  }
  bool get(const char* key,float& value)const{for(int i=0;i<count;i++)if(!strcmp(key,entries[i].key)){value=entries[i].value;return true;}return false;}
  String json()const{String out="{";for(int i=0;i<count;i++){if(i)out+=',';out+='"';out+=entries[i].key;out+="\":";out+=String(entries[i].value,4);}return out+"}";}
};
