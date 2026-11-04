#pragma once
#include <Arduino.h>
// HUB75-S3 20261101-1: BEEP -> R8 -> Q1, LDR filter -> GPIO8.
class BoardDiagnostics {
  const uint8_t pins_[5]={0,21,47,48,38};
  bool raw_[5]={},pressed_[5]={};uint32_t changed_[5]={},count_[5]={};
  uint32_t next_=0,stopBeep_=0;uint16_t light_=0,mv_=0;
public:
  void begin(){for(auto pin:pins_)pinMode(pin,INPUT_PULLUP);pinMode(39,OUTPUT);digitalWrite(39,LOW);pinMode(8,INPUT);analogSetPinAttenuation(8,ADC_11db);}
  bool poll(){
    const uint32_t now=millis();if(stopBeep_&&int32_t(now-stopBeep_)>=0){ledcWrite(7,0);ledcDetachPin(39);digitalWrite(39,LOW);stopBeep_=0;}
    for(int i=0;i<5;i++){bool value=!digitalRead(pins_[i]);if(value!=raw_[i]){raw_[i]=value;changed_[i]=now;}if(value!=pressed_[i]&&now-changed_[i]>=25){pressed_[i]=value;if(value)count_[i]++;}}
    if(int32_t(now-next_)>=0){next_=now+100;light_=analogRead(8);mv_=analogReadMilliVolts(8);return true;}return false;
  }
  uint16_t lightRaw()const{return light_;}uint16_t lightMillivolts()const{return mv_;}
  void beep(){ledcSetup(7,2000,8);ledcAttachPin(39,7);ledcWrite(7,96);stopBeep_=millis()+500;}
  String json()const{
    String s="{\"supported\":true,\"light\":{\"pin\":8,\"raw\":"+String(light_)+",\"millivolts\":"+String(mv_)+"},\"buzzer\":{\"pin\":39,\"active\":"+(stopBeep_?"true":"false")+"},\"buttons\":[";
    for(int i=0;i<5;i++){if(i)s+=',';s+="{\"pin\":"+String(pins_[i])+",\"pressed\":"+(pressed_[i]?"true":"false")+",\"count\":"+String(count_[i])+"}";}return s+"]}";
  }
};
