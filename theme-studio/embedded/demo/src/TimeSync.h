#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

class TimeSync {
public:
  void begin(Preferences& prefs);
  void poll(bool connected);
  bool configure(bool automatic,const String& server,String& error);
  bool sync(String& error);
  bool manual(const String& local,const String& epoch,String& error);
  String json()const;
private:
  Preferences* prefs_=nullptr;
  bool automatic_=true,running_=false;
  String server_="ntp.cloud.aliyuncs.com",activeServer_,source_="unknown";
  const char* state_="waiting-network";
  uint8_t attempt_=0;
  int64_t nextAt_=0,startedAt_=0;
  time_t lastSync_=0;
  void stop();
};
