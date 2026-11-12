#pragma once
#include <Arduino.h>

namespace UpdateSources {
static bool normalize(String& value){
  value.trim();if(value.isEmpty())return true;
  if(value.length()>192||!value.startsWith("https://")||value.indexOf("..")>=0)return false;
  const int end=value.indexOf('/',8);
  const String host=value.substring(8,end<0?value.length():end);
  if(host.indexOf('.')<0||strspn(host.c_str(),"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-:")!=host.length())return false;
  const String path=end<0?String():value.substring(end);
  if(strspn(path.c_str(),"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._/-")!=path.length())return false;
  if(!value.endsWith("/"))value+='/';return value.length()<=192;
}
static String base(unsigned index,const String& mirror){
  switch(index){
    case 0:return mirror;
    case 1:return "https://antonvanke.github.io/HUB-75E/";
    case 2:return "https://cdn.jsdelivr.net/gh/AntonVanke/HUB-75E@main/docs/";
    default:return "https://raw.githubusercontent.com/AntonVanke/HUB-75E/main/docs/";
  }
}
}
