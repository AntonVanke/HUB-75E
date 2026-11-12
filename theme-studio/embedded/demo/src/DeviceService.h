#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include <FS.h>
#include <DNSServer.h>
#include <WiFiUdp.h>
#include <functional>
#include <atomic>
#include <mbedtls/sha256.h>
#include "ThemeSchedule.h"
#include "DeviceLimits.h"
#include "AppManifest.h"
#include "AppParameters.h"
#include "PanelSettings.h"
#include "PanelTrial.h"
#include "AutoBrightness.h"
#include "FirmwareUpdate.h"
#include "TimeSync.h"
#include "EventLog.h"
class DeviceService {
  class UploadServer final:public WebServer {
  public:
    explicit UploadServer(int port):WebServer(port){}
    void releaseParsedUpload(){_currentUpload.reset();_currentRaw.reset();}
  };
public:
  int theme=0,brightness=80,rotationMinutes=0,themeCount=0;
  String lastError,networkState="idle";
  char metrics[512]="{}";
  bool hasCustom=false;
  AppParameters activeParameters;
  bool loadParameters(const char* path,String& error);
  PanelTrial panelTrial;
  AutoBrightnessController ambient;
  FirmwareUpdate updater;
  EventLog eventLog;
  bool needsSetup()const{return setupRequired_;}
  const String& setupName()const{return apName_;}
  const String& setupPassword()const{return apPassword_;}
  uint32_t generation=0,buttonAUntil=0,buttonBUntil=0,panelTestUntil=0;
  std::function<bool(const char*,String&)> applyScript;
  std::function<bool(const char*,String&)> validateScript;
  std::function<void()> ioTick;
  std::function<void()> stopScript;
  std::function<void(uint32_t,uint32_t,const char*)> updateProgress;
  std::function<bool(bool)> renderLock;
  std::function<bool(uint16_t*,uint8_t&)> copyFrame;
  std::function<String()> sensorStatus;
  std::function<String()> hardwareStatus;
  std::function<bool(const String&,String&)> hardwareTest;
  std::function<String()> panelStatus;
  bool panelReady=false;
  bool autoThemeUpdate_=false,themeUpdateBusy_=false;
  std::atomic<bool> themeUpdateDone_{false};
  String themeUpdateState_="idle",themeUpdateError_,themeUpdatePath_,themeUpdatePackage_;
  String themeUpdateResult_,themeUpdateWorkError_,themeUpdateVersion_;
  uint16_t themeUpdateSkipped_=0,themeUpdateWorkSkipped_=0;
  size_t themeUpdateBytes_=0;
  int themeUpdateIndex_=-1;
  uint32_t themeUpdateChecked_=0;
  uint32_t themeUpdateDelay_=21600000UL;
  std::function<void(const PanelOrientation&)> orientationChanged;
  std::function<bool(PanelOrientation&,bool)> orientationCalibrate;
  std::function<String()> orientationStatus;
  void begin(int count,const char* titlesJson);
  void poll();
  void pollSerial();
  bool appInstalled(int index)const;
  String appPath(int index)const;
  bool checkPackage(const char* path,String& error,int* targetFps=nullptr);
private:
  UploadServer web_{80};
  Preferences prefs_;
  TimeSync timeSync_;
  DNSServer dns_;
  WiFiUDP discoveryUdp_;
  struct DiscoveredDevice { IPAddress ip; char id[17]={},chip[32]={},version[16]={}; } discovered_[12];
  uint8_t discoveredCount_=0,discoveryQueries_=0;
  bool discoveryListening_=false,discovering_=false,discoveryTruncated_=false;
  uint32_t discoveryStarted_=0,discoverySent_=0,discoveryReply_=0,discoveryLastScan_=0;
  String discoveryNonce_;
  void pollDiscovery(uint32_t now);
  bool startDiscovery(String& error);
  String discoveryJson()const;
  String discoveryIdentity()const;
  String ssid_,password_,pendingSsid_,pendingPassword_,uploadError_,expectedHash_;
  File scriptFile_;
  size_t scriptBytes_=0,otaBytes_=0,otaTotal_=0;
  String apName_,apPassword_;
  bool ap_=false,connecting_=false,wifiStartPending_=false,ota_=false,locked_=false,fs_=false,scriptLocked_=false;
  uint32_t connectAt_=0,retryAt_=0,restartAt_=0,rotateAt_=0,otaAt_=0;
  uint32_t apConnectedAt_=0;
  const char* restartReason_="unspecified";
  void scheduleRestart(uint32_t delay,const char* reason){restartAt_=millis()+delay;restartReason_=reason;}
  mbedtls_sha256_context hash_;
  void reply(int code,const String& body);
  void sendFlash(const char* type,const char* data,size_t size);
  void access();
  void startAP();
  void finishOTA();
  void uploadScript();
  void finishScript();
  void sendFrame(bool serial);
  void uploadOTA();
  const char* titlesJson_=nullptr;
  const char* catalogRows_[64]={};
  uint16_t catalogLengths_[64]={};
  void indexCatalog();
  uint64_t installed_=0;
  uint64_t incompatible_=0;
  DeviceLimits limits_;
  uint32_t appBytes_[64]={};
  uint32_t appIdentityHash_[64]={};
  uint8_t appTargetFps_[64]={};
  uint8_t candidateTargetFps_=60;
  bool candidateChecked_=false;
  char serialLine_[512]={};
  uint16_t serialLength_=0;
  bool serialOverflow_=false;
  String networksJson(int& code);
  bool scanPending_=false;
  uint32_t scanStartedAt_=0;
  int installIndex_=-1;
  String installId_,installTitle_,installPackageName_;
  bool legacyUpload_=false;
  bool serialInstall_=false;
  uint32_t serialInstallAt_=0,serialInstallStarted_=0;
  size_t serialInstallExpected_=0;
  String serialInstallPackage_,serialInstallHash_;
  mbedtls_sha256_context serialInstallDigest_;
  void abortSerialInstall();
  void serialInstallCommand(const String& cmd,const String& line);
  String completeInstall(int& code,bool serial=false);
  uint8_t sensorCapabilities()const;
  String sensorCapabilitiesJson()const;
  bool saveOrientation(int manualRotation,bool automatic,int gravityRotation,const String& calibration,String& error);
  String appId(int index);
  String appTitle(int index);
  String appPackageName(int index);
  int appIndex(const String& identity);
  int requestAppIndex(String& error);
  bool readManifest(const char* path,AppManifest& manifest,String& error);
  bool catalogManifest(int index,AppManifest& manifest);
  bool compatible(const AppManifest& manifest,String& error)const;
  bool appCompatible(int index,String& error);
  String catalogField(int index,const char* key)const;
  String catalogJson(int index)const;
  int catalogIndexByName(const String& name,const String& id)const;
  void refreshApps();
  void reconcileApps();
  void sendApps(bool serial=false);
  String appConfig(int index,const String* values,int& code);
  void exportApp(int index,bool archive=false,bool source=false);
  void finishInstall();
  bool uninstallApp(int index,String& error);
  bool runApp(int index,String& error);
  bool connectWifi(const String& ssid,const String& password,String& error);
  bool saveConfig(int nextTheme,int nextBrightness,int nextRotation,const String& tz,String& error);
  bool savePanel(const String& order,PanelTiming timing,String& error,bool reboot=true);
  bool trialAction(const String& action,String& error);
  String trialJson()const;
  bool factoryReset(const String& confirmation,String& error);
  bool factoryResetting_=false,setupRequired_=false;
  bool autoSetupAttempted_=false;
  int setupStep_=1;
  bool panelRestartRequired_=false;
  String activePanelOrder_;
  bool bootRaw_=true,bootStable_=true,bootArmed_=false;
  uint32_t bootChanged_=0,bootClicked_=0;
  void pollSetupButton();
  bool setupAction(const String& action,String& error);
  void finishSetupAfterWifi();
  String panelOrder_="RGB";
  PanelTiming panelTiming_;
  String statusJson();
  String brightnessJson()const;
  void serialCommand(const String& line);
  void pruneSchedule();
  ThemeSchedule schedule_;
  ThemeConditions conditions_;
  int baseTheme_=0,windowTheme_=ThemeSchedule::None;int64_t lastDaily_=-1;
  bool readSchedule(const String& text,ThemeSchedule& out,bool allowMissing=false,bool migrateLegacy=false);
  String scheduleText()const;
  void pollSchedule(uint32_t now);
  void pollThemeUpdates(uint32_t now);
  void startThemeUpdateCheck();
  void themeUpdateWorker();
  bool finishThemeUpdate();
  String themeUpdateJson()const;
};
