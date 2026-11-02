#pragma once
#include <Arduino.h>
#include <ctype.h>
#include <limits.h>
#include <cstring>

namespace CwJson {
inline void space(const char*& p){while(*p&&isspace((unsigned char)*p))p++;}
inline int hex(char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
inline bool word(const char*& p,uint32_t& code){code=0;for(int i=0;i<4;i++){int n=hex(*p);if(n<0)return false;code=(code<<4)|n;p++;}return true;}
inline bool string(const char*& p,String& out){
  if(*p++!='"')return false;out="";
  while(*p&&*p!='"'){
    unsigned char c=*p++;if(c<32)return false;if(c!='\\'){out+=(char)c;continue;}c=*p++;if(!c)return false;
    if(c=='"'||c=='\\'||c=='/')out+=(char)c;
    else if(c=='b')out+='\b';else if(c=='f')out+='\f';else if(c=='n')out+='\n';else if(c=='r')out+='\r';else if(c=='t')out+='\t';
    else if(c=='u'){
      uint32_t u;if(!word(p,u))return false;
      if(u>=0xd800&&u<=0xdbff){if(*p++!='\\'||*p++!='u')return false;uint32_t low;if(!word(p,low)||low<0xdc00||low>0xdfff)return false;u=0x10000+((u-0xd800)<<10)+(low-0xdc00);}
      else if(u>=0xdc00&&u<=0xdfff)return false;if(!u)return false;
      if(u<0x80)out+=(char)u;else if(u<0x800){out+=(char)(0xc0|(u>>6));out+=(char)(0x80|(u&63));}
      else if(u<0x10000){out+=(char)(0xe0|(u>>12));out+=(char)(0x80|((u>>6)&63));out+=(char)(0x80|(u&63));}
      else{out+=(char)(0xf0|(u>>18));out+=(char)(0x80|((u>>12)&63));out+=(char)(0x80|((u>>6)&63));out+=(char)(0x80|(u&63));}
    }else return false;
  }if(*p!='"')return false;p++;return true;
}
inline bool skip(const char*& p,int depth=0){
  if(depth>8)return false;space(p);
  if(*p=='"'){String value;return string(p,value);}
  if(*p=='{'||*p=='['){const char end=*p++=='{'?'}':']';space(p);if(*p==end){p++;return true;}
    while(*p){if(end=='}'){String key;if(!string(p,key))return false;space(p);if(*p++!=':')return false;}
      if(!skip(p,depth+1))return false;space(p);if(*p==end){p++;return true;}if(*p++!=',')return false;space(p);
    }return false;
  }
  const char* start=p;while(*p&&*p!=','&&*p!='}'&&*p!=']'&&!isspace((unsigned char)*p))p++;
  const size_t length=p-start;if(!length)return false;
  if((length==4&&(!strncmp(start,"true",4)||!strncmp(start,"null",4)))||(length==5&&!strncmp(start,"false",5)))return true;
  const char* number=start;if(*number=='-')number++;
  if(number>=p||!isdigit((unsigned char)*number))return false;
  if(*number=='0')number++;else while(number<p&&isdigit((unsigned char)*number))number++;
  if(number<p&&*number=='.'){number++;const char* digit=number;while(number<p&&isdigit((unsigned char)*number))number++;if(number==digit)return false;}
  if(number<p&&(*number=='e'||*number=='E')){number++;if(number<p&&(*number=='+'||*number=='-'))number++;const char* digit=number;while(number<p&&isdigit((unsigned char)*number))number++;if(number==digit)return false;}
  return number==p;
}
inline bool field(const String& input,const char* wanted,String& value,bool& found,char* kind=nullptr){
  const char* p=input.c_str();space(p);found=false;if(*p++!='{')return false;space(p);if(*p=='}'){p++;space(p);return !*p;}
  while(*p){String key,next;if(!string(p,key))return false;space(p);if(*p++!=':')return false;space(p);const char token=*p;
    if(*p=='"'){if(!string(p,next))return false;}else{const char* start=p;if(!skip(p))return false;next.concat(start,p-start);}
    if(key==wanted){if(found)return false;found=true;value=next;if(kind)*kind=token;}
    space(p);if(*p=='}'){p++;space(p);return !*p;}if(*p++!=',')return false;space(p);
  }return false;
}
inline bool natural(const String& value,uint32_t& number){
  if(value.isEmpty()||value.length()>10)return false;uint64_t n=0;
  for(size_t i=0;i<value.length();i++){if(!isdigit((unsigned char)value[i]))return false;n=n*10+value[i]-'0';if(n>UINT32_MAX)return false;}number=n;return true;
}
}

struct AppManifest {
  String packageName,id,title,appVersion,description,author,requirements="{}",verification="{\"status\":\"unverified\"}";
  uint32_t runtimeApi=1,psramBytes=0,luaHeapBytes=0,luaWorkingBytes=0,graphicsBytes=0,targetFps=60;
  uint8_t chips=3,sensors=0;
  bool declared=false,modern=false;
  static bool nameOK(const String& value){
    if(value.length()<3||value.length()>63)return false;bool first=true,dot=false;
    for(size_t i=0;i<value.length();i++){const char c=value[i];if(first){if(c<'a'||c>'z')return false;first=false;}
      else if(c=='.'){first=true;dot=true;}else if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='-'))return false;
    }return dot&&!first;
  }
  static bool versionOK(const String& value){
    const char* p=value.c_str();for(int i=0;i<3;i++){if(!isdigit((unsigned char)*p))return false;const bool zero=*p=='0';p++;if(zero&&isdigit((unsigned char)*p))return false;while(isdigit((unsigned char)*p))p++;if(i<2&&*p++!='.')return false;}
    for(char separator:{'-','+'})if(*p==separator){p++;const char* start=p;while(isalnum((unsigned char)*p)||*p=='.'||*p=='-')p++;if(p==start)return false;}
    return !*p;
  }
  static bool textOK(const String& value,size_t maximum,bool required=false){if(value.length()>maximum||(required&&value.isEmpty()))return false;for(size_t i=0;i<value.length();i++){const unsigned char c=value[i];if(c<32&&c!=9&&c!=10&&c!=13)return false;}return true;}
  bool parse(const String& input,String& error){
    modern=true;String value;bool found;char kind;
    auto text=[&](const char* key,String& out,size_t max,bool needed=false){if(!CwJson::field(input,key,value,found,&kind)||(!found&&needed)||(found&&kind!='"'))return false;if(found)out=value;return textOK(out,max,needed);};
    if(!text("packageName",packageName,63,true)||!nameOK(packageName)||!text("id",id,32,true)||id.isEmpty()||strspn(id.c_str(),"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")!=id.length()||!text("title",title,96,true)||!text("appVersion",appVersion,31,true)||!versionOK(appVersion)||!text("description",description,384)||!text("author",author,64)){error="应用清单身份、版本或文本字段无效";return false;}
    if(!CwJson::field(input,"runtimeApi",value,found,&kind)||!found||kind=='"'||!CwJson::natural(value,runtimeApi)||!runtimeApi||runtimeApi>65535){error="应用运行时 API 声明无效";return false;}
    if(!CwJson::field(input,"requirements",requirements,found,&kind)||!found||kind!='{'){error="应用 requirements 必须是对象";return false;}
    auto number=[&](const char* key,uint32_t& out){if(!CwJson::field(requirements,key,value,found,&kind))return false;return !found||(kind!='"'&&CwJson::natural(value,out)&&out<=64*1024*1024);};
    if(!number("psramBytes",psramBytes)||!number("luaHeapBytes",luaHeapBytes)||!number("luaWorkingBytes",luaWorkingBytes)||!number("graphicsBytes",graphicsBytes)||!number("targetFps",targetFps)||targetFps<1||targetFps>60||(luaHeapBytes&&luaWorkingBytes>luaHeapBytes)){error="应用内存或帧率声明无效";return false;}
    if(!CwJson::field(requirements,"chips",value,found,&kind)){error="应用芯片声明无效";return false;}
    if(found){const char* p=value.c_str();CwJson::space(p);chips=0;if(kind!='['||*p++!='['){error="chips 必须是数组";return false;}CwJson::space(p);
      while(*p&&*p!=']'){String chip;if(!CwJson::string(p,chip)||(chip!="esp32"&&chip!="esp32s3")){error="应用芯片声明无效";return false;}const uint8_t bit=chip=="esp32"?1:2;if(chips&bit){error="应用芯片声明不能重复";return false;}chips|=bit;CwJson::space(p);if(*p==']')break;if(*p++!=','){error="应用芯片声明无效";return false;}CwJson::space(p);}
      if(*p++!=']'||!chips){error="应用芯片列表为空或无效";return false;}CwJson::space(p);if(*p){error="应用芯片声明无效";return false;}
    }
    if(!CwJson::field(requirements,"sensors",value,found,&kind)){error="应用传感器声明无效";return false;}
    if(found){const char* p=value.c_str();CwJson::space(p);sensors=0;if(kind!='['||*p++!='['){error="sensors 必须是数组";return false;}CwJson::space(p);
      while(*p&&*p!=']'){String sensor;if(!CwJson::string(p,sensor)){error="应用传感器声明无效";return false;}const uint8_t bit=sensor=="temperature"?1:sensor=="humidity"?2:sensor=="imu"?4:0;if(!bit||(sensors&bit)){error="传感器须为 temperature、humidity 或 imu，且不能重复";return false;}sensors|=bit;CwJson::space(p);if(*p==']')break;if(*p++!=','){error="应用传感器声明无效";return false;}CwJson::space(p);}
      if(*p++!=']'){error="应用传感器声明无效";return false;}CwJson::space(p);if(*p){error="应用传感器声明无效";return false;}
    }
    declared=luaHeapBytes>0&&luaWorkingBytes>0&&graphicsBytes>0;
    if(!CwJson::field(input,"verification",value,found,&kind)){error="应用验证声明无效";return false;}if(found){if(kind!='{'){error="应用验证声明必须是对象";return false;}verification=value;}
    return true;
  }
};
