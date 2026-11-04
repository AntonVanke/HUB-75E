#pragma once
#include <Arduino.h>
#include <math.h>

class BoardSensors {
public:
  float temperature=NAN,humidity=NAN,accel[3]={NAN,NAN,NAN},gyro[3]={NAN,NAN,NAN};
  uint32_t ahtSamples=0,imuSamples=0,ahtErrors=0,imuErrors=0;
  bool ahtPresent=false,imuPresent=false,accelSelfTest=false,gyroSelfTest=false;
  void begin();
  void poll();
  bool temperatureValid()const{return ahtPresent&&ahtSamples&&uint32_t(millis()-ahtGood_)<10000&&isfinite(temperature);}
  bool humidityValid()const{return ahtPresent&&ahtSamples&&uint32_t(millis()-ahtGood_)<10000&&isfinite(humidity);}
  bool imuValid()const{return imuPresent&&imuSamples&&uint32_t(millis()-imuGood_)<500;}
  String json()const;
  bool testImu();
private:
  uint32_t ahtAt_=0,ahtReady_=0,imuAt_=0,reportAt_=0,ahtGood_=0,imuGood_=0;
  uint8_t imuAddress_=0x6a;uint32_t probeAt_=0;
  void probe(bool startup);
  bool waiting_=false;
  bool writeRegister(uint8_t reg,uint8_t value);
  bool readRegisters(uint8_t reg,uint8_t* data,size_t count);
  bool average(uint8_t ready,uint8_t reg,float* values);
  bool selfTest(bool gyro);
};
