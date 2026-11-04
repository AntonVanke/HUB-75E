#include "BoardSensors.h"
#include <Wire.h>

static constexpr uint8_t IMU=0x6a,AHT=0x38;
bool BoardSensors::writeRegister(uint8_t reg,uint8_t value){
  Wire.beginTransmission(IMU);Wire.write(reg);Wire.write(value);return Wire.endTransmission()==0;
}
bool BoardSensors::readRegisters(uint8_t reg,uint8_t* data,size_t count){
  Wire.beginTransmission(IMU);Wire.write(reg);
  if(Wire.endTransmission(false)!=0)return false;
  if(Wire.requestFrom(IMU,uint8_t(count))!=count)return false;
  for(size_t i=0;i<count;i++)data[i]=Wire.read();return true;
}
bool BoardSensors::average(uint8_t ready,uint8_t reg,float* values){
  for(int a=0;a<3;a++)values[a]=0;
  for(int sample=0;sample<6;sample++){
    uint8_t status=0,bytes[6];const uint32_t start=millis();
    do{if(!readRegisters(0x1e,&status,1))return false;if(status&ready)break;delay(1);}while(millis()-start<250);
    if(!(status&ready)||!readRegisters(reg,bytes,6))return false;
    if(sample)for(int a=0;a<3;a++)values[a]+=int16_t(uint16_t(bytes[a*2])|(uint16_t(bytes[a*2+1])<<8))/5.0f;
  }return true;
}
bool BoardSensors::selfTest(bool gyroTest){
  float baseline[3],stimulated[3],delta[3];
  bool ok=writeRegister(0x10,gyroTest?0:0x38)&&writeRegister(0x11,gyroTest?0x5c:0)&&writeRegister(0x14,0);
  delay(gyroTest?150:100);
  ok=ok&&average(gyroTest?2:1,gyroTest?0x22:0x28,baseline);
  ok=ok&&writeRegister(0x14,gyroTest?0x04:0x01);
  delay(gyroTest?50:100);
  ok=ok&&average(gyroTest?2:1,gyroTest?0x22:0x28,stimulated);
  writeRegister(0x14,0);
  if(!ok){Serial.printf("[IMU_SELFTEST] %s I2C/timeout FAIL\n",gyroTest?"gyro":"accel");return false;}
  for(int a=0;a<3;a++){delta[a]=fabsf(stimulated[a]-baseline[a])*(gyroTest?0.070f:0.122f);if(delta[a]<(gyroTest?150:90)||delta[a]>(gyroTest?700:1700))ok=false;}
  Serial.printf("[IMU_SELFTEST] %s delta=%.2f,%.2f,%.2f %s %s\n",gyroTest?"gyro":"accel",delta[0],delta[1],delta[2],gyroTest?"dps":"mg",ok?"PASS":"FAIL");return ok;
}
void BoardSensors::begin(){
  pinMode(3,INPUT);pinMode(46,INPUT);
  Wire.begin(1,2,100000);Wire.setTimeOut(25);
  Serial.println("[SENSORS] SDA=GPIO1 SCL=GPIO2 I2C=100000");
  for(uint8_t address=1;address<127;address++){Wire.beginTransmission(address);if(Wire.endTransmission()==0)Serial.printf("[I2C] found 0x%02X\n",address);}
  Wire.beginTransmission(AHT);ahtPresent=Wire.endTransmission()==0;
  uint8_t id=0;imuPresent=readRegisters(0x0f,&id,1)&&id==0x6a;
  Serial.printf("[SENSORS] AHT30=%s LSM6DS3TR-C WHO_AM_I=0x%02X %s\n",ahtPresent?"present":"MISSING",id,imuPresent?"PASS":"FAIL");
  if(imuPresent){
    bool ready=writeRegister(0x12,0x01);const uint32_t start=millis();uint8_t control=1;
    do{delay(1);ready=ready&&readRegisters(0x12,&control,1);}while(ready&&(control&1)&&millis()-start<100);
    ready=ready&&!(control&1)&&writeRegister(0x12,0x44);
    if(ready){accelSelfTest=selfTest(false);gyroSelfTest=selfTest(true);}
    imuPresent=ready&&writeRegister(0x14,0)&&writeRegister(0x10,0x40)&&writeRegister(0x11,0x40);
  }
  ahtAt_=millis()-2000;reportAt_=millis();
}
bool BoardSensors::testImu(){
  if(!imuPresent)return false;
  accelSelfTest=selfTest(false);gyroSelfTest=selfTest(true);
  imuPresent=writeRegister(0x14,0)&&writeRegister(0x10,0x40)&&writeRegister(0x11,0x40);
  imuGood_=0;return imuPresent&&accelSelfTest&&gyroSelfTest;
}
void BoardSensors::poll(){
  const uint32_t now=millis();
  if(ahtPresent&&!waiting_&&now-ahtAt_>=2000){
    Wire.beginTransmission(AHT);Wire.write(0xac);Wire.write(0x33);Wire.write(0);
    waiting_=Wire.endTransmission()==0;ahtAt_=now;ahtReady_=now+85;if(!waiting_)ahtErrors++;
  }
  if(waiting_&&int32_t(now-ahtReady_)>=0){
    uint8_t b[7]={},crc=0xff;bool valid=Wire.requestFrom(AHT,uint8_t(7))==7;
    if(valid){for(auto& byte:b)byte=Wire.read();for(int i=0;i<6;i++){crc^=b[i];for(int bit=0;bit<8;bit++)crc=crc&0x80?(crc<<1)^0x31:crc<<1;}
      valid=!(b[0]&0x80)&&(b[0]&0x18)==0x18&&crc==b[6];
      if(valid){humidity=((uint32_t(b[1])<<12)|(uint32_t(b[2])<<4)|(b[3]>>4))*(100.0f/1048576.0f);temperature=((uint32_t(b[3]&15)<<16)|(uint32_t(b[4])<<8)|b[5])*(200.0f/1048576.0f)-50;ahtSamples++;ahtGood_=now;}
    }if(!valid)ahtErrors++;waiting_=false;
  }
  if(imuPresent&&now-imuAt_>=5){
    imuAt_=now;uint8_t status=0,b[12];
    if(!readRegisters(0x1e,&status,1))imuErrors++;
    else if((status&3)==3){
      if(readRegisters(0x22,b,12)){for(int a=0;a<3;a++){gyro[a]=int16_t(uint16_t(b[a*2])|(uint16_t(b[a*2+1])<<8))*0.00875f;accel[a]=int16_t(uint16_t(b[6+a*2])|(uint16_t(b[7+a*2])<<8))*0.000061f;}imuSamples++;imuGood_=now;}else imuErrors++;
    }
  }
  if(now-reportAt_>=1000){reportAt_=now;Serial.printf("[SENSORS] T=%.2fC RH=%.2f%% aht_samples=%u aht_errors=%u accel_g=%.4f,%.4f,%.4f gyro_dps=%.3f,%.3f,%.3f imu_samples=%u imu_errors=%u selftest=%s/%s\n",temperature,humidity,ahtSamples,ahtErrors,accel[0],accel[1],accel[2],gyro[0],gyro[1],gyro[2],imuSamples,imuErrors,accelSelfTest?"PASS":"FAIL",gyroSelfTest?"PASS":"FAIL");}
}
String BoardSensors::json()const{
  const bool tv=temperatureValid(),hv=humidityValid(),iv=imuValid();
  char body[768],t[24]="null",h[24]="null",a[96]="null",g[96]="null";
  if(tv)snprintf(t,sizeof(t),"%.3f",temperature);
  if(hv)snprintf(h,sizeof(h),"%.3f",humidity);
  if(iv){snprintf(a,sizeof(a),"[%.5f,%.5f,%.5f]",accel[0],accel[1],accel[2]);snprintf(g,sizeof(g),"[%.4f,%.4f,%.4f]",gyro[0],gyro[1],gyro[2]);}
  snprintf(body,sizeof(body),"{\"temperature\":%s,\"humidity\":%s,\"ahtPresent\":%s,\"imuPresent\":%s,\"imuSupported\":true,\"temperatureValid\":%s,\"humidityValid\":%s,\"imuValid\":%s,\"ahtSamples\":%u,\"ahtErrors\":%u,\"ahtHealthy\":%s,\"accelG\":%s,\"gyroDps\":%s,\"imuSamples\":%u,\"imuErrors\":%u,\"imuHealthy\":%s,\"accelSelfTest\":%s,\"gyroSelfTest\":%s}",t,h,ahtPresent?"true":"false",imuPresent?"true":"false",tv?"true":"false",hv?"true":"false",iv?"true":"false",ahtSamples,ahtErrors,tv&&hv?"true":"false",a,g,imuSamples,imuErrors,iv?"true":"false",accelSelfTest?"true":"false",gyroSelfTest?"true":"false");return String(body);
}
