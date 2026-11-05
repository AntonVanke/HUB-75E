#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <esp_rom_crc.h>
#include <time.h>
#include <sys/time.h>
#include <NativeRuntime.h>
#include "NativeCatalog.h"
#include "DeviceService.h"
#include "SetupQr.h"
#include <WiFi.h>
#include <esp_system.h>
#include <LittleFS.h>
#include "ThemePackage.h"
#include "PanelSettings.h"
#ifdef HUB75_S3_BOARD
#include "BoardSensors.h"
#include "BoardDiagnostics.h"
#include <soc/lcd_cam_struct.h>
#include "GpioPanelProbe.h"
#include "ProtomatterPanelProbe.h"
static BoardSensors sensors;
static BoardDiagnostics diagnostics;
static GpioPanelProbe gpioProbe;
static ProtomatterPanelProbe protomatterProbe;
#endif

// One 64x64 panel; S3 mapping follows enet/20261101-1.enet.
#ifndef PANEL_ROTATION
#define PANEL_ROTATION 3
#endif
constexpr int FRAME_US=16667;
constexpr int THEME_COUNT=6;
struct Snapshot {
  ThemeClock clock;int theme=0,brightness=80,rotation=PANEL_ROTATION;uint32_t generation=0;
  float temperature=NAN,humidity=NAN,accel[3]={},gyro[3]={},gravity[3]={};
  bool a=false,b=false,panelTest=false,imuSupported=false,imuValid=false,temperatureValid=false,humidityValid=false,autoRotate=false;
};
struct Metrics { uint32_t frames=0,presents=0,scans=0,late=0,missed=0,maxUs=0,computeUs=0,blitUs=0,heap=0,lua=0,luaUsed=0,graphics=0,generation=0,sampleMs=0; int theme=-1,level=0,area=0,deaths=0,coins=0,targetFps=60; char error[256]={}; };
static QueueHandle_t snapshotQueue,metricsQueue;
static TaskHandle_t renderHandle;
static esp_timer_handle_t frameTimer;
static MatrixPanel_I2S_DMA* matrix;
#ifdef HUB75_S3_BOARD
static GFXcanvas16* diagnosticCanvas=nullptr;
#endif
static int selectedPanelDriver=0;
static bool panelInitialized=false;
static String panelInitError;
static SetupQr setupQr;
static bool diagnosticPanel(){return selectedPanelDriver==7;}
static Adafruit_GFX* panelGraphics(){
#ifdef HUB75_S3_BOARD
  if(selectedPanelDriver==7)return diagnosticCanvas;
  if(selectedPanelDriver==9)return protomatterProbe.graphics();
#endif
  return matrix;
}
static uint16_t panelColor(uint16_t color){
#ifdef HUB75_S3_BOARD
  if(selectedPanelDriver==9)return protomatterProbe.color(color);
#endif
  return color;
}
static void showPanel(){
#ifdef HUB75_S3_BOARD
  if(selectedPanelDriver==7){if(diagnosticCanvas)gpioProbe.present(diagnosticCanvas->getBuffer());return;}
  if(selectedPanelDriver==9){protomatterProbe.show();return;}
#endif
  matrix->flipDMABuffer();
}
static int appliedBrightness=-1;
static void panelBrightness(uint8_t brightness){
  if(appliedBrightness==brightness)return;
  appliedBrightness=brightness;
#ifdef HUB75_S3_BOARD
  if(selectedPanelDriver==7){gpioProbe.setBrightness(brightness);return;}
  if(selectedPanelDriver==9){protomatterProbe.setBrightness(brightness);return;}
#endif
  matrix->setBrightness8(brightness);
}
static Snapshot snapshot;
static DeviceService service;
static PanelOrientationController orientation;
static DeviceLimits limits;
static SemaphoreHandle_t renderGate;
static uint32_t bootReadyAt=0,bootDrawAt=0;
static bool bootReady=false;

class DmaHost final:public ThemeHost {
public:
  ThemePackage package;File packageFile;
  uint8_t* packageCache[8]={};bool cacheAttempted[8]={};size_t packageCacheBytes=0;
  void releasePackage(){
    packageFile.close();package={};
    for(int i=0;i<8;i++){free(packageCache[i]);packageCache[i]=nullptr;cacheAttempted[i]=false;}
    packageCacheBytes=0;
  }
  const uint8_t* cachedResource(uint32_t i){
    if(!cacheAttempted[i]){
      cacheAttempted[i]=true;const auto& m=package.media[i];
      if(m.size<=limits.packageCacheBytes-packageCacheBytes){
        auto* bytes=(uint8_t*)heap_caps_malloc(m.size,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
        if(bytes){if(packageFile.seek(m.offset)&&packageFile.read(bytes,m.size)==m.size){packageCache[i]=bytes;packageCacheBytes+=m.size;}else free(bytes);}
      }
    }
    return packageCache[i];
  }
  int level=0,area=0,deaths=0,coins=0;
  void telemetryNumber(const char* key,double value)override{
    if(!strcmp(key,"level"))level=int(value);else if(!strcmp(key,"area"))area=int(value);
    else if(!strcmp(key,"deaths"))deaths=int(value);else if(!strcmp(key,"coins"))coins=int(value);
  }
  struct MediaReader { DmaHost* host;uint32_t offset,size; } readers[8];
  size_t resourceSize(const char* id)override{for(uint32_t i=0;i<package.count;i++)if(!strcmp(id,package.media[i].id))return package.media[i].size;return ThemeHost::resourceSize(id);}
  size_t readResource(const char* id,size_t offset,uint8_t* out,size_t count)override{
    for(uint32_t i=0;i<package.count;i++)if(!strcmp(id,package.media[i].id)){const auto& m=package.media[i];if(offset>m.size||count>m.size-offset)return 0;if(const auto* cache=cachedResource(i)){memcpy(out,cache+offset,count);return count;}if(!packageFile.seek(m.offset+offset))return 0;return packageFile.read(out,count);}
    return ThemeHost::readResource(id,offset,out,count);
  }
  bool loadMedia(const char* id,MediaClip& clip)override{
    for(uint32_t i=0;i<package.count;i++)if(!strcmp(id,package.media[i].id)){
      if(const auto* cache=cachedResource(i))return clip.load(cache,package.media[i].size);
      readers[i]={this,package.media[i].offset,package.media[i].size};
      return clip.load([](void* context,size_t offset,uint8_t* out,size_t n)->size_t{auto& r=*(MediaReader*)context;if(offset>r.size)return 0;n=std::min(n,size_t(r.size-offset));if(!r.host->packageFile.seek(r.offset+offset))return 0;return r.host->packageFile.read(out,n);},&readers[i],readers[i].size);
    }return ThemeHost::loadMedia(id,clip);
  }
  uint32_t blitUs=0;bool presented=false;
  uint32_t tiles[2][128]={};bool valid[2]={};int back=0;
#ifdef HUB75_S3_BOARD
  uint16_t previousPixels[4096]={};
#endif
  uint32_t randomSeed()override{return esp_random();}
  const uint8_t* media(const char* id,size_t& bytes)override{
    bytes=0;return nullptr;
  }
  // Snapshots are owned/read only by the render task.
  void pixel(int x,int y,uint16_t c)override{panelGraphics()->drawPixel(x,y,panelColor(c));}
  ThemeClock clock()override{
    struct timeval tv;gettimeofday(&tv,nullptr);time_t epoch=tv.tv_sec;
    static time_t previous=-1;static ThemeClock result;
    if(epoch!=previous){struct tm t;localtime_r(&epoch,&t);result.hour=t.tm_hour;result.minute=t.tm_min;result.second=t.tm_sec;result.day=t.tm_mday;result.month=t.tm_mon+1;result.year=t.tm_year+1900;result.weekday=t.tm_wday;previous=epoch;}
    result.epoch=epoch;result.millisecond=tv.tv_usec/1000;return result;
  }
  bool pressed(const char* key)override{return !strcmp(key,"A")?snapshot.a:!strcmp(key,"B")?snapshot.b:false;}
  bool data(const char* key,ThemeValue& value)override{
    if(!strncmp(key,"setting.",8)){float n;if(!service.activeParameters.get(key+8,n))return false;value.type=ThemeValue::Number;value.number=n;return true;}
    bool boolean=false,isBoolean=true;
    if(!strcmp(key,"imu_supported"))boolean=snapshot.imuSupported;
    else if(!strcmp(key,"imu_valid"))boolean=snapshot.imuValid;
    else if(!strcmp(key,"temperature_valid"))boolean=snapshot.temperatureValid;
    else if(!strcmp(key,"humidity_valid"))boolean=snapshot.humidityValid;
    else if(!strcmp(key,"auto_rotate"))boolean=snapshot.autoRotate;
    else isBoolean=false;
    if(isBoolean){value.type=ThemeValue::Boolean;value.boolean=boolean;return true;}
    if(!strcmp(key,"screen_rotation")){value.number=snapshot.rotation*90;return true;}
    for(int axis=0;axis<3;axis++){
      const char names[]="xyz";const size_t n=strlen(key);
      if(n&&key[n-1]==names[axis]){
        const float* vector=nullptr;
        if(n==7&&!strncmp(key,"accel_",6))vector=snapshot.accel;
        else if(n==6&&!strncmp(key,"gyro_",5))vector=snapshot.gyro;
        else if(n==9&&!strncmp(key,"gravity_",8))vector=snapshot.gravity;
        if(vector){if(!snapshot.imuValid)return false;value.number=vector[axis];return true;}
      }
    }
    if(!strcmp(key,"temperature")){if(!snapshot.temperatureValid)return false;value.number=snapshot.temperature;}
    else if(!strcmp(key,"humidity")){if(!snapshot.humidityValid)return false;value.number=snapshot.humidity;}
    else if(!strcmp(key,"message")){value.type=ThemeValue::String;value.string="HELLO PIXELS";}
    else return false;return true;
  }
  void present(const uint16_t* pixels)override{
#ifdef HUB75_S3_BOARD
    if(selectedPanelDriver==9){
      blitUs=0;
      if(valid[0]&&!memcmp(previousPixels,pixels,sizeof(previousPixels)))return;
      const int64_t start=esp_timer_get_time();
      if(!protomatterProbe.present(pixels))return;memcpy(previousPixels,pixels,sizeof(previousPixels));
      valid[0]=true;presented=true;blitUs=esp_timer_get_time()-start;return;
    }
#endif
    const bool doubleBuffer=selectedPanelDriver!=9&&matrix->getCfg().double_buff;const int front=doubleBuffer?back^1:back;
    blitUs=0;uint32_t hashes[128];bool changed=!valid[front];
    for(int tile=0;tile<128;tile++){
      const int x=(tile%8)*8,y=(tile/8)*4;uint32_t hash=0;
      for(int row=0;row<4;row++)hash=esp_rom_crc32_le(hash,(const uint8_t*)(pixels+(y+row)*64+x),16);
      hashes[tile]=hash;if(hash!=tiles[front][tile])changed=true;
    }
    if(!changed)return;
    presented=true;
    const int64_t start=esp_timer_get_time();
    // Small dirty tiles avoid rewriting a full row for a moving sprite or raindrop.
    for(int tile=0;tile<128;tile++){
      if(valid[back]&&hashes[tile]==tiles[back][tile])continue;
      const int leftTile=(tile%8)*8,top=(tile/8)*4;
      for(int y=top;y<top+4;y++)for(int x=leftTile;x<leftTile+8;){
      const int left=x;const uint16_t color=pixels[y*64+x++];
      while(x<leftTile+8&&pixels[y*64+x]==color)x++;
      matrix->drawFastHLine(left,y,x-left,color);
      }
    }
    memcpy(tiles[back],hashes,sizeof(hashes));valid[back]=true;
    matrix->flipDMABuffer();if(doubleBuffer)back^=1;blitUs=esp_timer_get_time()-start;
  }
};
static DmaHost host;
static NativeRuntime runtime(host);
static int targetFps=60;
static uint32_t framePeriodUs=FRAME_US;
static String packageError;
static bool loadScript(const char* path){
  runtime.stop();host.releasePackage();
  packageError="";targetFps=60;framePeriodUs=FRAME_US;
  if(!service.checkPackage(path,packageError,&targetFps)||!service.loadParameters(path,packageError))return false;
  framePeriodUs=(1000000+targetFps-1)/targetFps;
  host.packageFile=LittleFS.open(path,"r");
  if(!host.package.read(host.packageFile,limits)||host.package.version!=3){packageError="需要 C/C++ 二进制主题包 CWP3";return false;}
  
  return runtime.load([](void* context,size_t offset,char* out,size_t count)->size_t{
    auto& h=*(DmaHost*)context;if(!h.packageFile.seek(h.package.sourceOffset+offset))return 0;return h.packageFile.read((uint8_t*)out,count);
  },&host,host.package.sourceSize);
}
static bool loadTheme(int index){
  host.level=host.area=host.deaths=host.coins=0;
  if(index>=0)return loadScript(service.appPath(index).c_str());
  runtime.stop();host.releasePackage();
  packageError="";targetFps=60;framePeriodUs=FRAME_US;
  return runtime.basicClock();
}
static void timerTick(void*){xTaskNotifyGive(renderHandle);}
static bool showBoot(){
  panelBrightness(service.brightness);
  const uint32_t now=millis();
  const bool connected=WiFi.status()==WL_CONNECTED&&WiFi.localIP()!=IPAddress(0,0,0,0);
  const IPAddress ip=connected?WiFi.localIP():WiFi.softAPIP();
  const bool ready=ip!=IPAddress(0,0,0,0);
  if(!connected)bootReady=false;
  else if(!bootReady){bootReady=true;bootReadyAt=now;}
  if(bootReady&&now-bootReadyAt>=5000)return false;
  if(now-bootDrawAt<250)return true;
  bootDrawAt=now;
  auto* graphics=panelGraphics();graphics->setRotation(orientation.rotation());graphics->fillScreen(0);graphics->setTextSize(1);graphics->setTextWrap(false);
  auto line=[](int y,const char* text,uint16_t color){auto* graphics=panelGraphics();graphics->setTextColor(panelColor(color));graphics->setCursor(std::max(0,(64-int(strlen(text))*6)/2),y);graphics->print(text);};
  line(2,"WI-FI",0x07ff);
  line(14,connected?"CONNECTED":ready?"SETUP AP":"CONNECTING",0xffff);
  char text[16];
  if(ready){
    snprintf(text,sizeof(text),"%u.%u.",ip[0],ip[1]);line(28,text,0xffe0);
    snprintf(text,sizeof(text),"%u.%u",ip[2],ip[3]);line(38,text,0xffe0);
  }else line(36,"PLEASE WAIT",0xffe0);
  if(bootReady){snprintf(text,sizeof(text),"START IN %u",unsigned(5-(now-bootReadyAt)/1000));line(53,text,0xffff);}
  else line(53,"WAIT WI-FI",0xffff);
  showPanel();return true;
}
static void drawPanelTest(){
  auto* graphics=panelGraphics();graphics->fillScreen(0);
  // Use individual pixels so the test also checks the optimized line blitter independently.
  for(int y=0;y<64;y++)for(int x=0;x<64;x++){
    uint16_t c=0;
    if(x==0||x==63||y==0||y==63)c=0xffff;
    else if(y>=46&&y<=60){if(x>=3&&x<=19)c=0xf800;else if(x>=23&&x<=39)c=0x07e0;else if(x>=43&&x<=59)c=0x001f;}
    else if(y<43&&(x%16==0||y%16==0))c=0x4208;
    const int dx=x-32,dy=y-23,d=dx*dx+dy*dy;
    if((x>=8&&x<=55&&(y==4||y==42))||(y>=4&&y<=42&&(x==8||x==55))||(d>=132&&d<=156))c=0xffff;
    graphics->drawPixel(x,y,panelColor(c));
  }
  graphics->setTextSize(1);graphics->setTextColor(panelColor(0xffff));graphics->setTextWrap(false);
  for(int i=0;i<3;i++){graphics->setCursor(9+i*20,50);graphics->print("RGB"[i]);}
  graphics->setCursor(29,22);graphics->print(selectedPanelDriver);
  showPanel();host.valid[0]=host.valid[1]=false;
}
static void drawDriverGuide(){
  auto* g=panelGraphics();if(!g)return;
  const bool qr=service.needsSetup()&&setupQr.size()&&(service.panelTrial.qrManual?service.panelTrial.showQr:(millis()-service.panelTrial.startedAt)%PanelTrial::durationMs>=8000);
  panelBrightness(qr?std::min(service.brightness,24):service.brightness);
  static bool previousQr=false;static int qrRotation=-1;
  const bool unchanged=qr&&previousQr&&qrRotation==orientation.configuration().rotation;
  previousQr=qr;qrRotation=orientation.configuration().rotation;
  if(unchanged)return;
  g->setRotation(orientation.configuration().rotation);
  if(!service.needsSetup()){
    drawPanelTest();
    return;
  }
  if(qr){
    g->fillScreen(0);g->setTextSize(1);g->setTextWrap(false);g->setTextColor(panelColor(0xffff));
    char label[12];snprintf(label,sizeof(label),"D%d WIFI",selectedPanelDriver);g->setCursor(11,2);g->print(label);
    const int size=setupQr.size(),offset=(64-size)/2;
    // Inverted QR: white modules with a black quiet zone of at least four modules.
    for(int y=0;y<size;y++)for(int x=0;x<size;x++)if(setupQr.dark(x,y))g->drawPixel(offset+x,offset+y,panelColor(0xffff));
    g->setCursor(5,55);g->print("JOIN WIFI");showPanel();return;
  }
  g->fillScreen(0);g->drawRect(0,0,64,64,panelColor(0xffff));
  g->setTextSize(1);g->setTextWrap(false);
  auto text=[g](int x,int y,const char* value,uint16_t color){g->setTextColor(panelColor(color));g->setCursor(x,y);g->print(value);};
  char number[8];snprintf(number,sizeof(number),"D%d",selectedPanelDriver);
  text(3,2,number,0xffff);text(27,2,"R",0xf800);text(39,2,"G",0x07e0);text(51,2,"B",0x001f);
  text(20,12,"WIFI",0x07ff);
  text(2,22,service.setupName().c_str(),0xffff);
  String password="P:"+service.setupPassword();text(2,32,password.c_str(),0xffe0);
  text(8,42,"192.168.",0xffff);text(8,51,"4.1",0xffff);
  if(service.panelTrial.active){
    if(service.panelTrial.paused)text(38,51,"HOLD",0xffe0);
    else{snprintf(number,sizeof(number),"%lus",(unsigned long)service.panelTrial.remaining(millis()));text(38,51,number,0xffe0);}
  }
  for(int i=0;i<3;i++)g->fillRect(2+i*20,60,19,2,panelColor(i==0?0xf800:i==1?0x07e0:0x001f));
  showPanel();
}
static void publishInputs();
static void renderTask(void*){
  Metrics metrics;int current=-2;uint32_t generation=0;int64_t previous=esp_timer_get_time()/1000,reportAt=esp_timer_get_time(),nextFrameAt=0;bool healthy=false;
  bool testing=false;
  for(;;){
    const uint32_t pending=ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
    xSemaphoreTake(renderGate,portMAX_DELAY);
    if(pending>1)metrics.missed+=pending-1;
    const bool oldA=snapshot.a,oldB=snapshot.b;
    xQueueReceive(snapshotQueue,&snapshot,0);
    if(!panelInitialized){xSemaphoreGive(renderGate);continue;}
#ifdef HUB75_S3_BOARD
    if(diagnosticPanel()){
      const int64_t start=esp_timer_get_time();
      gpioProbe.scan();
      metrics.frames++;metrics.computeUs=esp_timer_get_time()-start;metrics.maxUs=std::max(metrics.maxUs,metrics.computeUs);
      if(esp_timer_get_time()-reportAt>=1000000){
#ifdef HUB75_S3_BOARD
      metrics.scans+=protomatterProbe.takeScanCount();
#endif
        metrics.theme=snapshot.theme;metrics.generation=snapshot.generation;metrics.sampleMs=esp_timer_get_time()/1000;
        metrics.heap=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);xQueueOverwrite(metricsQueue,&metrics);reportAt=esp_timer_get_time();
      }
      xSemaphoreGive(renderGate);continue;
    }
#endif
    bool rotated=false;
    if(panelGraphics()->getRotation()!=snapshot.rotation){panelGraphics()->setRotation(snapshot.rotation);host.valid[0]=host.valid[1]=false;rotated=true;}
    if(snapshot.brightness!=appliedBrightness){panelBrightness(snapshot.brightness);host.valid[0]=host.valid[1]=false;}
    if(snapshot.panelTest){
      if(!testing||rotated)drawPanelTest();testing=true;
#ifdef HUB75_S3_BOARD
      if(esp_timer_get_time()-reportAt>=1000000){metrics.scans+=protomatterProbe.takeScanCount();metrics.sampleMs=esp_timer_get_time()/1000;xQueueOverwrite(metricsQueue,&metrics);reportAt=esp_timer_get_time();}
#endif
      xSemaphoreGive(renderGate);continue;
    }
    if(testing){testing=false;previous=esp_timer_get_time()/1000;nextFrameAt=esp_timer_get_time();}
    host.blitUs=0;host.presented=false;
    const int64_t start=esp_timer_get_time();
    if(snapshot.theme!=current||snapshot.generation!=generation){
      current=snapshot.theme;generation=snapshot.generation;
      healthy=loadTheme(current);
      esp_timer_stop(frameTimer);esp_timer_start_periodic(frameTimer,framePeriodUs);
      metrics={};
#ifdef HUB75_S3_BOARD
      protomatterProbe.takeScanCount();
#endif
      metrics.theme=current;metrics.generation=generation;metrics.targetFps=targetFps;previous=esp_timer_get_time()/1000;nextFrameAt=esp_timer_get_time()+framePeriodUs;
      // Initialization is reported separately from the frame budget.
      if(!healthy){snprintf(metrics.error,sizeof(metrics.error),"%s",packageError.length()?packageError.c_str():runtime.error());runtime.stop();}
    }else if(healthy){
      if(snapshot.a&&!oldA)healthy=runtime.button("A");
      if(healthy&&snapshot.b&&!oldB)healthy=runtime.button("B");
      if(healthy&&(start>=nextFrameAt||rotated||(snapshot.a&&!oldA)||(snapshot.b&&!oldB))){
        const uint32_t dt=std::min<int64_t>(std::max(uint32_t(100),(framePeriodUs+999)/1000),start/1000-previous);previous=start/1000;
        healthy=runtime.update(dt)&&runtime.draw();
        const uint32_t elapsed=esp_timer_get_time()-start;
        if(healthy){metrics.frames++;if(host.presented)metrics.presents++;}metrics.computeUs=elapsed-host.blitUs;metrics.blitUs=host.blitUs;
        metrics.maxUs=std::max(metrics.maxUs,elapsed);if(elapsed>framePeriodUs)metrics.late++;
        while(nextFrameAt<=start)nextFrameAt+=framePeriodUs;
      }
      if(!healthy){snprintf(metrics.error,sizeof(metrics.error),"%s",runtime.error());runtime.stop();}
    }
    if(esp_timer_get_time()-reportAt>=1000000){
#ifdef HUB75_S3_BOARD
      metrics.scans+=protomatterProbe.takeScanCount();
#endif
      metrics.heap=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
      metrics.sampleMs=esp_timer_get_time()/1000;metrics.level=host.level;metrics.area=host.area;metrics.deaths=host.deaths;metrics.coins=host.coins;
      metrics.lua=runtime.peakMemoryUsed();metrics.luaUsed=runtime.memoryUsed();metrics.graphics=runtime.graphicsMemoryUsed();
      xQueueOverwrite(metricsQueue,&metrics);reportAt=esp_timer_get_time();
    }
    xSemaphoreGive(renderGate);
  }
}
void setup(){
  Serial.begin(115200);setCpuFrequencyMhz(240);
  service.panelTrial.begin();
  orientation.configure(panelOrientation());
#ifdef HUB75_S3_BOARD
  delay(1500);
  Serial.printf("[BOARD] HUB75-S3 20261101-1 chip=%s flash=%u psram=%u\n",ESP.getChipModel(),ESP.getFlashChipSize(),ESP.getPsramSize());
  sensors.begin();
  diagnostics.begin();
  service.ambient.supported=true;
  service.hardwareStatus=[](){return diagnostics.json();};
  service.hardwareTest=[](const String& test,String& error){if(test=="buzzer"){diagnostics.beep();return true;}if(test=="imu"){if(sensors.testImu())return true;error="IMU 自检失败或未检测到";return false;}error="未知测试项目";return false;};
  service.sensorStatus=[](){return sensors.json();};
#else
  service.sensorStatus=[](){return String("{\"ahtPresent\":false,\"imuPresent\":false,\"imuSupported\":false,\"temperatureValid\":false,\"humidityValid\":false,\"imuValid\":false,\"ahtHealthy\":false,\"imuHealthy\":false,\"temperature\":null,\"humidity\":null,\"accelG\":null,\"gyroDps\":null}");};
#endif
  service.orientationChanged=[](const PanelOrientation& value){orientation.configure(value);};
  service.orientationCalibrate=[](PanelOrientation& value,bool flat){return orientation.calibrate(value,flat,millis());};
  service.orientationStatus=[](){
#ifdef HUB75_S3_BOARD
    const bool supported=true,valid=sensors.imuValid();
#else
    const bool supported=false,valid=false;
#endif
    char gravity[96]="null";
    if(orientation.gravityValid()){float x,y,z;orientation.screenGravity(x,y,z);snprintf(gravity,sizeof(gravity),"[%.5f,%.5f,%.5f]",x,y,z);}
    const auto& config=orientation.configuration();
    return "{\"gravityRotation\":"+String(config.gravityRotation*90)+",\"gravityZero\":["+String(config.zeroX,5)+","+String(config.zeroY,5)+"],\"gravityFilterMs\":18,\"manualRotation\":"+String(config.rotation*90)+",\"rotationDegrees\":"+String(orientation.rotation()*90)+",\"autoRotate\":"+(config.autoRotate?"true":"false")+",\"autoRotateSupported\":"+(supported?"true":"false")+",\"imuValid\":"+(valid?"true":"false")+",\"source\":\""+(orientation.automatic()?"gravity":config.autoRotate?"sensor-unavailable":"manual")+"\",\"gravityG\":"+gravity+",\"coordinates\":\"screen +x right,+y down; -accel in g; calibrated IMU X/Y basis at rotation 0\"}";
  };
  HUB75_I2S_CFG cfg(64,64,1);
#ifdef HUB75_S3_BOARD
  cfg.gpio={4,14,5,6,13,7,15,11,16,10,12,9,18,17};
#else
  cfg.gpio={25,26,27,14,12,13,23,19,5,17,18,4,15,16};
#endif
  applyPanelColorOrder(cfg.gpio,panelColorOrder());
  const PanelTiming timing=service.panelTrial.timing;
  selectedPanelDriver=timing.driver;
  cfg.driver=static_cast<HUB75_I2S_CFG::shift_driver>(timing.driver<=6?timing.driver:0);
  cfg.i2sspeed=static_cast<HUB75_I2S_CFG::clk_speed>(timing.clockMHz*1000000);
  cfg.clkphase=timing.clockPhase;cfg.latch_blanking=timing.latchBlanking;
  cfg.double_buff=true;cfg.min_refresh_rate=120;
  cfg.setPixelColorDepthBits(6);
  if(timing.driver==8){cfg.double_buff=false;cfg.min_refresh_rate=60;cfg.setPixelColorDepthBits(8);}
  Serial.printf("[PANEL] profile=%d %s 64x64 1/32 clock=%dMHz phase=%d blank=%d\n",timing.driver,panelDriverName(timing.driver),timing.clockMHz,timing.clockPhase,timing.latchBlanking);
#ifdef HUB75_S3_BOARD
  if(timing.driver==7){diagnosticCanvas=new(std::nothrow) GFXcanvas16(64,64);panelInitialized=diagnosticCanvas&&diagnosticCanvas->getBuffer()&&gpioProbe.begin(cfg.gpio);}
  else if(timing.driver==9)panelInitialized=protomatterProbe.begin(cfg.gpio,timing.refreshHz);
  else
#endif
  {
    matrix=new MatrixPanel_I2S_DMA(cfg);panelInitialized=matrix->begin();
    if(panelInitialized){
      Serial.printf("[PANEL] active_driver=%d phase=%d depth=%d double=%d refresh_min=%d\n",int(matrix->getCfg().driver),matrix->getCfg().clkphase,matrix->getCfg().getPixelColorDepthBits(),matrix->getCfg().double_buff,matrix->getCfg().min_refresh_rate);
#ifdef HUB75_S3_BOARD
      Serial.printf("[PANEL] LCD divider=%u actual_clock=%uHz\n",unsigned(LCD_CAM.lcd_clock.lcd_clkm_div_num),160000000U/unsigned(LCD_CAM.lcd_clock.lcd_clkm_div_num));
#endif
    }
  }
  if(panelInitialized&&!diagnosticPanel()){panelGraphics()->setRotation(orientation.rotation());panelBrightness(80);panelGraphics()->fillScreen(0);showPanel();}
  if(!panelInitialized)panelInitError="屏幕驱动初始化失败，可通过串口或后台切回 SHIFTREG";
  service.panelStatus=[](){
    String result="{\"initialized\":"+String(panelInitialized?"true":"false")+",\"diagnostic\":"+(diagnosticPanel()?"true":"false")+",\"backend\":\""+(selectedPanelDriver==7?"GPIO":selectedPanelDriver==9?"Protomatter":"HUB75_DMA")+"\"";
#ifdef HUB75_S3_BOARD
    if(selectedPanelDriver==9)result+=",\"colorDepth\":4,\"doubleBuffer\":true,\"refreshHz\":"+String(protomatterProbe.refreshRate(),1);
#endif
    if(matrix&&panelInitialized){const auto& active=matrix->getCfg();result+=",\"driver\":"+String(int(active.driver))+",\"clockPhase\":"+String(active.clkphase?1:0)+",\"colorDepth\":"+String(active.getPixelColorDepthBits())+",\"doubleBuffer\":"+(active.double_buff?"true":"false")+",\"minRefreshHz\":"+String(active.min_refresh_rate);
#ifdef HUB75_S3_BOARD
      const unsigned divider=LCD_CAM.lcd_clock.lcd_clkm_div_num;result+=",\"clockHz\":"+String(divider?160000000U/divider:0);
#endif
    }return result+"}";
  };
  limits=DeviceLimits::detect();
  renderGate=xSemaphoreCreateMutex();
  snapshotQueue=xQueueCreate(1,sizeof(Snapshot));metricsQueue=xQueueCreate(1,sizeof(Metrics));
  if(!snapshotQueue||!metricsQueue||!renderGate){Serial.println("Queue allocation failed");return;}
  service.renderLock=[](bool lock){if(lock)return xSemaphoreTake(renderGate,pdMS_TO_TICKS(2000))==pdTRUE;xSemaphoreGive(renderGate);return true;};
  service.stopScript=[](){runtime.stop();host.releasePackage();};
  service.ioTick=publishInputs;
  service.copyFrame=[](uint16_t* out,uint8_t& rotation){
    if(!renderHandle||service.panelTrial.active||service.needsSetup()||diagnosticPanel()||(service.panelTestUntil&&int32_t(service.panelTestUntil-millis())>0))return false;
    if(xSemaphoreTake(renderGate,pdMS_TO_TICKS(40))!=pdTRUE)return false;
    memcpy(out,runtime.pixels(),8192);rotation=snapshot.rotation;xSemaphoreGive(renderGate);return true;
  };
  service.validateScript=[](const char* path,String& error){
    File file=LittleFS.open(path,"r");ThemePackage package;
    if(!package.read(file,limits)||package.version!=3){error="无效的原生应用包";return false;}
    struct Input{File* file;size_t offset;};Input input{&file,package.sourceOffset};
    auto reader=[](void* p,size_t at,char* out,size_t n)->size_t{auto& i=*(Input*)p;if(!i.file->seek(i.offset+at))return 0;size_t read=i.file->read((uint8_t*)out,n);if(service.ioTick)service.ioTick();return read;};
    const char* result=NativeRuntime::validate(reader,&input,package.sourceSize);
    if(result){error=result;return false;}return true;
  };
  service.applyScript=[](const char* path,String& error){
    if(!panelInitialized||diagnosticPanel()){error="当前为屏幕驱动自检模式，请切回普通驱动再运行主题";return false;}
    if(loadScript(path))return true;
    error=packageError.length()?packageError:runtime.error();
    if(error.indexOf("memory")>=0){
      char detail[112];snprintf(detail,sizeof(detail)," (native %u/%u B, free %u B, largest %u B)",(unsigned)runtime.memoryUsed(),(unsigned)runtime.peakMemoryUsed(),heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));error+=detail;
    }
    runtime.stop();return false;
  };
  service.panelReady=panelInitialized;
  service.begin(THEME_COUNT,THEME_TITLES);
  if(service.needsSetup())setupQr.begin(service.setupName().c_str(),service.setupPassword().c_str());
  service.lastError=panelInitError;
  Serial.printf("64x64, 240 MHz, render core 1, service core 0; %d catalog apps. Serial: {\"cmd\":\"help\"}\n",THEME_COUNT);
}
static void publishInputs(){
  static uint32_t last=0;const uint32_t now=millis();if(now-last<4)return;last=now;
#ifdef HUB75_S3_BOARD
  sensors.poll();if(diagnostics.poll())service.ambient.sample(diagnostics.lightRaw(),diagnostics.lightMillivolts(),now);
#endif
  if(!snapshotQueue||service.panelTrial.active||service.needsSetup())return;
  static Snapshot next;next.a=service.buttonAUntil&&int32_t(service.buttonAUntil-millis())>0;next.b=service.buttonBUntil&&int32_t(service.buttonBUntil-millis())>0;next.theme=service.theme;next.generation=service.generation;next.brightness=service.ambient.tick(service.brightness,millis());
  next.panelTest=service.panelTestUntil&&int32_t(service.panelTestUntil-millis())>0;
#ifdef HUB75_S3_BOARD
  next.temperatureValid=sensors.temperatureValid();next.humidityValid=sensors.humidityValid();
  next.temperature=next.temperatureValid?sensors.temperature:NAN;next.humidity=next.humidityValid?sensors.humidity:NAN;
  next.imuSupported=true;next.imuValid=sensors.imuValid();
  orientation.update(millis(),true,next.imuValid,sensors.imuSamples,sensors.accel[0],sensors.accel[1],sensors.accel[2]);
  if(next.imuValid){for(int i=0;i<3;i++){next.accel[i]=sensors.accel[i];next.gyro[i]=sensors.gyro[i];}orientation.screenGravity(next.gravity[0],next.gravity[1],next.gravity[2]);}
#else
  orientation.update(millis(),false,false,0,0,0,0);
#endif
  next.rotation=orientation.rotation();next.autoRotate=orientation.configuration().autoRotate;
  struct timeval tv;gettimeofday(&tv,nullptr);time_t epoch=tv.tv_sec;struct tm t;localtime_r(&epoch,&t);
  next.clock.hour=t.tm_hour;next.clock.minute=t.tm_min;next.clock.second=t.tm_sec;
  next.clock.day=t.tm_mday;next.clock.month=t.tm_mon+1;next.clock.year=t.tm_year+1900;next.clock.weekday=t.tm_wday;
  next.clock.millisecond=tv.tv_usec/1000;
  xQueueOverwrite(snapshotQueue,&next);
}
void loop(){
  publishInputs();
  if(!snapshotQueue||!metricsQueue){delay(100);return;}
  service.poll();
  if(service.panelTrial.active||service.needsSetup()){
    if(panelInitialized){
      if(millis()-bootDrawAt>=250){bootDrawAt=millis();drawDriverGuide();}
#ifdef HUB75_S3_BOARD
      if(diagnosticPanel())gpioProbe.scan();
      else if(selectedPanelDriver==9)protomatterProbe.takeScanCount();
#endif
    }
    delay(2);return;
  }
  publishInputs();
  if(!renderHandle){
    if(panelInitialized&&!diagnosticPanel()&&showBoot()){delay(10);return;}
    if(xTaskCreatePinnedToCore(renderTask,"theme-render",12288,nullptr,3,&renderHandle,1)!=pdPASS){Serial.println("Render task allocation failed");delay(100);return;}
    esp_timer_create_args_t args={};args.callback=timerTick;args.name="theme-frame";
    if(esp_timer_create(&args,&frameTimer)!=ESP_OK||esp_timer_start_periodic(frameTimer,FRAME_US)!=ESP_OK){Serial.println("Frame timer failed");return;}
  }
  Metrics m;
  if(xQueueReceive(metricsQueue,&m,0)==pdTRUE){
    service.lastError=m.error;
    snprintf(service.metrics,sizeof(service.metrics),"{\"theme\":%d,\"generation\":%u,\"frames\":%u,\"presents\":%u,\"scans\":%u,\"late\":%u,\"missed\":%u,\"computeUs\":%u,\"dmaUs\":%u,\"maxUs\":%u,\"nativeBytes\":%u,\"nativeUsed\":%u,\"graphicsBytes\":%u,\"sampleMs\":%u,\"targetFps\":%d,\"level\":%d,\"area\":%d,\"deaths\":%d,\"coins\":%d}",m.theme,m.generation,m.frames,m.presents,m.scans,m.late,m.missed,m.computeUs,m.blitUs,m.maxUs,m.lua,m.luaUsed,m.graphics,m.sampleMs,m.targetFps,m.level,m.area,m.deaths,m.coins);
    Serial.printf("frames=%u late=%u missed=%u cpu_us=%u dma_us=%u max_us=%u free_internal=%u lua_peak=%u graphics=%u level=%d area=%d deaths=%d coins=%d error=%s\n",m.frames,m.late,m.missed,m.computeUs,m.blitUs,m.maxUs,m.heap,m.lua,m.graphics,m.level,m.area,m.deaths,m.coins,m.error);
  }
  delay(2);
}
