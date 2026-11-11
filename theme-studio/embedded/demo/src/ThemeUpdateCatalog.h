#pragma once
#include "AppManifest.h"
#include <Stream.h>
#include <functional>

// Keep one catalog entry in RAM, including nested requirements/parameters.
class ThemeUpdateCatalog final:public Stream {
  enum Phase { Header, Entry, Separator, Tail } phase_=Header;
  String header_,row_,tail_;
  size_t bytes_=0;
  int depth_=0;
  bool quoted_=false,escaped_=false,failed_=false,afterComma_=false;
  std::function<void(const String&)> entry_;
public:
  explicit ThemeUpdateCatalog(std::function<void(const String&)> entry):entry_(entry){}
  int available()override{return 0;}
  int read()override{return -1;}
  int peek()override{return -1;}
  void flush()override{}
  size_t write(uint8_t byte)override{
    if(failed_||++bytes_>262144){failed_=true;return 0;}
    const char c=char(byte);
    if(phase_==Tail){if(tail_.length()>=1024||!tail_.concat(c))failed_=true;return failed_?0:1;}
    if(phase_==Header){
      if(c=='['&&!quoted_){String value;bool found=false;
        if(!CwJson::field(header_+"[]}","apps",value,found)||!found||value!="[]"){failed_=true;return 0;}
        phase_=Entry;return 1;
      }
      if(header_.length()>=1024||!header_.concat(c)){failed_=true;return 0;}
    }else if(!depth_){
      if(isspace((unsigned char)c))return 1;
      if(c==']'&&!afterComma_){phase_=Tail;return 1;}
      if(phase_==Separator&&c==','){phase_=Entry;afterComma_=true;return 1;}
      if(phase_!=Entry||c!='{'){failed_=true;return 0;}
      row_="";depth_=1;quoted_=escaped_=false;afterComma_=false;
      if(!row_.concat(c)){failed_=true;return 0;}return 1;
    }else{
      if(row_.length()>=8192||!row_.concat(c)){failed_=true;return 0;}
      if(!quoted_){if(c=='{'||c=='[')depth_++;else if(c=='}'||c==']')depth_--;}
    }
    if(escaped_)escaped_=false;
    else if(quoted_&&c=='\\')escaped_=true;
    else if(c=='"')quoted_=!quoted_;
    if(phase_!=Header&&!depth_){entry_(row_);row_="";phase_=Separator;}
    return 1;
  }
  size_t write(const uint8_t* data,size_t size)override{size_t n=0;while(n<size&&write(data[n]))n++;return n;}
  bool complete()const{
    String value;bool found=false;
    return !failed_&&phase_==Tail&&CwJson::field(header_+"[]"+tail_,"format",value,found)&&found&&value=="3";
  }
};
