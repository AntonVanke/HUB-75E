#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <esp_timer.h>
#include <esp_heap_caps.h>
#include <esp_rom_crc.h>
#include <time.h>
#include <sys/time.h>
#include <ThemeRuntime.h>
#include "ThemeCatalog.h"
#include "ThemeTitles.h"
#include "DeviceService.h"
#include <WiFi.h>
#include <esp_system.h>
#include <LittleFS.h>
#include "ThemePackage.h"
#include "PanelSettings.h"
#ifdef HUB75_S3_BOARD
#include "BoardSensors.h"
#include <soc/lcd_cam_struct.h>
#include "GpioPanelProbe.h"
#include "ProtomatterPanelProbe.h"
static BoardSensors sensors;
static GpioPanelProbe gpioProbe;
static ProtomatterPanelProbe protomatterProbe;
#endif

// One 64x64 panel; S3 mapping follows enet/20261101-1.enet.
#ifndef PANEL_ROTATION
#define PANEL_ROTATION 3
#endif
constexpr int FRAME_US=16667;
constexpr int THEME_COUNT=sizeof(THEME_CATALOG)/sizeof(THEME_CATALOG[0]);
static const char BASIC_CLOCK[] = R"CW(
function draw()
  gfx.clear(0)
  font.draw("segment",4,20,clock.format("%H"),65535)
  font.draw("segment",35,20,clock.format("%M"),65535)
  if clock.value("second")%2==0 then gfx.rect(30,26,2,2,65535) gfx.rect(30,34,2,2,65535) end
  gfx.text(11,49,clock.format("%m-%d"),33840)
end
)CW";
struct Snapshot {
  ThemeClock clock;int theme=0,brightness=80,rotation=PANEL_ROTATION;uint32_t generation=0;
  float temperature=NAN,humidity=NAN,accel[3]={},gyro[3]={},gravity[3]={};
  bool a=false,b=false,panelTest=false,imuSupported=false,imuValid=false,temperatureValid=false,humidityValid=false,autoRotate=false;
};
struct Metrics { uint32_t frames=0,late=0,missed=0,maxUs=0,computeUs=0,blitUs=0,heap=0,lua=0,luaUsed=0,graphics=0,generation=0,sampleMs=0; int theme=-1,level=0,area=0,deaths=0,coins=0,targetFps=60; char error[256]={}; };
static QueueHandle_t snapshotQueue,metricsQueue;
static TaskHandle_t renderHandle;
static esp_timer_handle_t frameTimer;
static MatrixPanel_I2S_DMA* matrix;
static int selectedPanelDriver=0;
static bool panelInitialized=false;
static String panelInitError;
static bool diagnosticPanel(){return selectedPanelDriver==7;}
static Adafruit_GFX* panelGraphics(){
#ifdef HUB75_S3_BOARD
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
  if(selectedPanelDriver==9){protomatterProbe.show();return;}
#endif
  matrix->flipDMABuffer();
}
static void panelBrightness(uint8_t brightness){
#ifdef HUB75_S3_BOARD
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
  uint32_t blitUs=0;
  uint32_t tiles[2][128]={};bool valid[2]={};int back=0;
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
    const bool doubleBuffer=selectedPanelDriver!=9&&matrix->getCfg().double_buff;const int front=doubleBuffer?back^1:back;
    blitUs=0;uint32_t hashes[128];bool changed=!valid[front];
    for(int tile=0;tile<128;tile++){
      const int x=(tile%8)*8,y=(tile/8)*4;uint32_t hash=0;
      for(int row=0;row<4;row++)hash=esp_rom_crc32_le(hash,(const uint8_t*)(pixels+(y+row)*64+x),16);
      hashes[tile]=hash;if(hash!=tiles[front][tile])changed=true;
    }
    if(!changed)return;
    const int64_t start=esp_timer_get_time();
#ifdef HUB75_S3_BOARD
    if(selectedPanelDriver==9){
      protomatterProbe.present(pixels);memcpy(tiles[back],hashes,sizeof(hashes));valid[back]=true;blitUs=esp_timer_get_time()-start;return;
    }
#endif
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
static ThemeRuntime runtime(host,128*1024,800000,64*1024);
static int targetFps=60;
static uint32_t framePeriodUs=FRAME_US;
static String packageError;
static bool loadScript(const char* path){
  runtime.stop();host.releasePackage();
  packageError="";targetFps=60;framePeriodUs=FRAME_US;
  if(!service.checkPackage(path,packageError,&targetFps))return false;
  framePeriodUs=(1000000+targetFps-1)/targetFps;
  host.packageFile=LittleFS.open(path,"r");
  if(!host.package.read(host.packageFile,limits))return runtime.load(nullptr,0);
  for(uint32_t i=0;i<host.package.count;i++)host.cachedResource(i);
  return runtime.load([](void* context,size_t offset,char* out,size_t count)->size_t{
    auto& h=*(DmaHost*)context;if(!h.packageFile.seek(h.package.sourceOffset+offset))return 0;return h.packageFile.read((uint8_t*)out,count);
  },&host,host.package.sourceSize);
}
static bool loadTheme(int index){
  host.level=host.area=host.deaths=host.coins=0;
  if(index>=0)return loadScript(service.appPath(index).c_str());
  runtime.stop();host.releasePackage();
  packageError="";targetFps=60;framePeriodUs=FRAME_US;
  return runtime.load(BASIC_CLOCK,strlen(BASIC_CLOCK));
}
static void timerTick(void*){xTaskNotifyGive(renderHandle);}
static bool showBoot(){
  const uint32_t now=millis();
  const bool connected=WiFi.status()==WL_CONNECTED;
  const IPAddress ip=connected?WiFi.localIP():WiFi.softAPIP();
  const bool ready=ip!=IPAddress(0,0,0,0);
  if(ready&&!bootReady){bootReady=true;bootReadyAt=now;}
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
    snprintf(text,sizeof(text),"START IN %u",unsigned(5-(now-bootReadyAt)/1000));line(53,text,0xffff);
  }else line(36,"PLEASE WAIT",0xffe0);
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
        metrics.theme=snapshot.theme;metrics.generation=snapshot.generation;metrics.sampleMs=esp_timer_get_time()/1000;
        metrics.heap=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);xQueueOverwrite(metricsQueue,&metrics);reportAt=esp_timer_get_time();
      }
      xSemaphoreGive(renderGate);continue;
    }
#endif
    bool rotated=false;
    if(panelGraphics()->getRotation()!=snapshot.rotation){panelGraphics()->setRotation(snapshot.rotation);host.valid[0]=host.valid[1]=false;rotated=true;}
    static int brightness=-1;if(snapshot.brightness!=brightness){brightness=snapshot.brightness;panelBrightness(brightness);host.valid[0]=host.valid[1]=false;}
    if(snapshot.panelTest){if(!testing||rotated)drawPanelTest();testing=true;xSemaphoreGive(renderGate);continue;}
    if(testing){testing=false;previous=esp_timer_get_time()/1000;nextFrameAt=esp_timer_get_time();}
    host.blitUs=0;
    const int64_t start=esp_timer_get_time();
    if(snapshot.theme!=current||snapshot.generation!=generation){
      current=snapshot.theme;generation=snapshot.generation;
      healthy=loadTheme(current);
      metrics={};metrics.theme=current;metrics.generation=generation;metrics.targetFps=targetFps;previous=esp_timer_get_time()/1000;nextFrameAt=esp_timer_get_time()+framePeriodUs;
      // Initialization is reported separately from the frame budget.
      if(!healthy){snprintf(metrics.error,sizeof(metrics.error),"%s",packageError.length()?packageError.c_str():runtime.error());runtime.stop();}
    }else if(healthy){
      if(snapshot.a&&!oldA)healthy=runtime.button("A");
      if(healthy&&snapshot.b&&!oldB)healthy=runtime.button("B");
      if(healthy&&(start>=nextFrameAt||rotated||(snapshot.a&&!oldA)||(snapshot.b&&!oldB))){
        const uint32_t dt=std::min<int64_t>(std::max(uint32_t(100),(framePeriodUs+999)/1000),start/1000-previous);previous=start/1000;
        healthy=runtime.update(dt)&&runtime.draw();
        const uint32_t elapsed=esp_timer_get_time()-start;
        if(healthy)metrics.frames++;metrics.computeUs=elapsed-host.blitUs;metrics.blitUs=host.blitUs;
        metrics.maxUs=std::max(metrics.maxUs,elapsed);if(elapsed>framePeriodUs)metrics.late++;
        while(nextFrameAt<=start)nextFrameAt+=framePeriodUs;
      }
      if(!healthy){snprintf(metrics.error,sizeof(metrics.error),"%s",runtime.error());runtime.stop();}
    }
    if(esp_timer_get_time()-reportAt>=1000000){
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
  orientation.configure(panelOrientation());
#ifdef HUB75_S3_BOARD
  delay(1500);
  Serial.printf("[BOARD] HUB75-S3 20261101-1 chip=%s flash=%u psram=%u\n",ESP.getChipModel(),ESP.getFlashChipSize(),ESP.getPsramSize());
  sensors.begin();
  service.sensorStatus=[](){return sensors.json();};
#else
  service.sensorStatus=[](){return String("{\"ahtPresent\":false,\"imuPresent\":false,\"imuSupported\":false,\"temperatureValid\":false,\"humidityValid\":false,\"imuValid\":false,\"ahtHealthy\":false,\"imuHealthy\":false,\"temperature\":null,\"humidity\":null,\"accelG\":null,\"gyroDps\":null}");};
#endif
  service.orientationChanged=[](const PanelOrientation& value){orientation.configure(value);};
  service.orientationStatus=[](){
#ifdef HUB75_S3_BOARD
    const bool supported=true,valid=sensors.imuValid();
#else
    const bool supported=false,valid=false;
#endif
    char gravity[96]="null";
    if(orientation.gravityValid()){float x,y,z;orientation.screenGravity(x,y,z);snprintf(gravity,sizeof(gravity),"[%.5f,%.5f,%.5f]",x,y,z);}
    const auto& config=orientation.configuration();
    return "{\"manualRotation\":"+String(config.rotation*90)+",\"rotationDegrees\":"+String(orientation.rotation()*90)+",\"autoRotate\":"+(config.autoRotate?"true":"false")+",\"autoRotateSupported\":"+(supported?"true":"false")+",\"imuValid\":"+(valid?"true":"false")+",\"source\":\""+(orientation.automatic()?"gravity":config.autoRotate?"sensor-unavailable":"manual")+"\",\"gravityG\":"+gravity+",\"coordinates\":\"screen +x right,+y down; -accel in g; IMU X/Y basis at rotation 0\"}";
  };
  HUB75_I2S_CFG cfg(64,64,1);
#ifdef HUB75_S3_BOARD
  cfg.gpio={4,14,5,6,13,7,15,11,16,10,12,9,18,17};
#else
  cfg.gpio={25,26,27,14,12,13,23,19,5,17,18,4,15,16};
#endif
  applyPanelColorOrder(cfg.gpio,panelColorOrder());
  const PanelTiming timing=panelTiming();
  selectedPanelDriver=timing.driver;
  cfg.driver=static_cast<HUB75_I2S_CFG::shift_driver>(timing.driver<=6?timing.driver:0);
  cfg.i2sspeed=static_cast<HUB75_I2S_CFG::clk_speed>(timing.clockMHz*1000000);
  cfg.clkphase=timing.clockPhase;cfg.latch_blanking=timing.latchBlanking;
  cfg.double_buff=true;cfg.min_refresh_rate=120;
  cfg.setPixelColorDepthBits(6);
  if(timing.driver==8){cfg.double_buff=false;cfg.min_refresh_rate=60;cfg.setPixelColorDepthBits(8);}
  Serial.printf("[PANEL] profile=%d %s 64x64 1/32 clock=%dMHz phase=%d blank=%d\n",timing.driver,panelDriverName(timing.driver),timing.clockMHz,timing.clockPhase,timing.latchBlanking);
#ifdef HUB75_S3_BOARD
  if(timing.driver==7)panelInitialized=gpioProbe.begin(cfg.gpio);
  else if(timing.driver==9)panelInitialized=protomatterProbe.begin(cfg.gpio);
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
    if(selectedPanelDriver==9)result+=",\"colorDepth\":4,\"doubleBuffer\":false";
    if(matrix&&panelInitialized){const auto& active=matrix->getCfg();result+=",\"driver\":"+String(int(active.driver))+",\"clockPhase\":"+String(active.clkphase?1:0)+",\"colorDepth\":"+String(active.getPixelColorDepthBits())+",\"doubleBuffer\":"+(active.double_buff?"true":"false")+",\"minRefreshHz\":"+String(active.min_refresh_rate);
#ifdef HUB75_S3_BOARD
      const unsigned divider=LCD_CAM.lcd_clock.lcd_clkm_div_num;result+=",\"clockHz\":"+String(divider?160000000U/divider:0);
#endif
    }return result+"}";
  };
  limits=DeviceLimits::detect();
  runtime.configureLimits(limits.luaHeapBytes,limits.luaWorkingBytes,limits.sourceBytes,limits.instructions,limits.psramBytes>=1024*1024);
  renderGate=xSemaphoreCreateMutex();
  snapshotQueue=xQueueCreate(1,sizeof(Snapshot));metricsQueue=xQueueCreate(1,sizeof(Metrics));
  if(!snapshotQueue||!metricsQueue||!renderGate){Serial.println("Queue allocation failed");return;}
  service.renderLock=[](bool lock){if(lock)return xSemaphoreTake(renderGate,pdMS_TO_TICKS(2000))==pdTRUE;xSemaphoreGive(renderGate);return true;};
  service.stopScript=[](){runtime.stop();host.releasePackage();};
  service.framePixels=[](){return runtime.pixels();};
  service.applyScript=[](const char* path,String& error){
    if(!panelInitialized||diagnosticPanel()){error="当前为屏幕驱动自检模式，请切回普通驱动再运行主题";return false;}
    if(loadScript(path))return true;
    error=packageError.length()?packageError:runtime.error();
    if(error.indexOf("memory")>=0){
      char detail[112];snprintf(detail,sizeof(detail)," (Lua %u/%u B, free %u B, largest %u B)",(unsigned)runtime.memoryUsed(),(unsigned)runtime.peakMemoryUsed(),heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT),heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));error+=detail;
    }
    runtime.stop();return false;
  };
  service.begin(THEME_COUNT,THEME_TITLES);
  service.lastError=panelInitError;
  Serial.printf("64x64, 240 MHz, render core 1, service core 0; %d catalog apps. Serial: {\"cmd\":\"help\"}\n",THEME_COUNT);
}
void loop(){
#ifdef HUB75_S3_BOARD
  sensors.poll();
#endif
  if(!snapshotQueue||!metricsQueue){delay(100);return;}
  service.poll();
  static Snapshot next;next.a=service.buttonAUntil&&int32_t(service.buttonAUntil-millis())>0;next.b=service.buttonBUntil&&int32_t(service.buttonBUntil-millis())>0;next.theme=service.theme;next.generation=service.generation;next.brightness=service.brightness;
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
  if(!renderHandle){
    if(panelInitialized&&!diagnosticPanel()&&showBoot()){delay(10);return;}
    if(xTaskCreatePinnedToCore(renderTask,"theme-render",12288,nullptr,3,&renderHandle,1)!=pdPASS){Serial.println("Render task allocation failed");delay(100);return;}
    esp_timer_create_args_t args={};args.callback=timerTick;args.name="theme-frame";
    if(esp_timer_create(&args,&frameTimer)!=ESP_OK||esp_timer_start_periodic(frameTimer,FRAME_US)!=ESP_OK){Serial.println("Frame timer failed");return;}
  }
  Metrics m;
  if(xQueueReceive(metricsQueue,&m,0)==pdTRUE){
    service.lastError=m.error;
    snprintf(service.metrics,sizeof(service.metrics),"{\"theme\":%d,\"generation\":%u,\"frames\":%u,\"late\":%u,\"missed\":%u,\"computeUs\":%u,\"dmaUs\":%u,\"maxUs\":%u,\"luaBytes\":%u,\"luaUsed\":%u,\"graphicsBytes\":%u,\"sampleMs\":%u,\"targetFps\":%d,\"level\":%d,\"area\":%d,\"deaths\":%d,\"coins\":%d}",m.theme,m.generation,m.frames,m.late,m.missed,m.computeUs,m.blitUs,m.maxUs,m.lua,m.luaUsed,m.graphics,m.sampleMs,m.targetFps,m.level,m.area,m.deaths,m.coins);
    Serial.printf("frames=%u late=%u missed=%u cpu_us=%u dma_us=%u max_us=%u free_internal=%u lua_peak=%u graphics=%u level=%d area=%d deaths=%d coins=%d error=%s\n",m.frames,m.late,m.missed,m.computeUs,m.blitUs,m.maxUs,m.heap,m.lua,m.graphics,m.level,m.area,m.deaths,m.coins,m.error);
  }
  delay(10);
}
