#include "NtpTransport.h"
#include "NtpPacket.h"
#include <esp_timer.h>
#include <esp_system.h>
#include <lwip/tcpip.h>

void NtpTransport::resolved(const char*,const ip_addr_t* ip,void* arg){
  auto& p=*static_cast<Peer*>(arg);
  p.address=ip&&IP_IS_V4(ip)?ip4_addr_get_u32(ip_2_ip4(ip)):0;
  p.state.store(p.address?2:3,std::memory_order_release);
}
void NtpTransport::resolve(void* arg){
  auto& p=*static_cast<Peer*>(arg);ip_addr_t ip;
  const err_t result=dns_gethostbyname_addrtype(p.name,&ip,resolved,arg,LWIP_DNS_ADDRTYPE_IPV4);
  if(result==ERR_OK)resolved(nullptr,&ip,arg);
  else if(result!=ERR_INPROGRESS)resolved(nullptr,nullptr,arg);
}
bool NtpTransport::begin(const String& preferred){
  stop();if(!udp_.begin(0))return false;
  const char* defaults[]={"ntp.cloud.aliyuncs.com","ntp.tencent.com","time.cloudflare.com","pool.ntp.org"};
  int fallback=0;
  for(int i=0;i<4;i++){
    String name=preferred;
    if(i){while(preferred==defaults[fallback])fallback++;name=defaults[fallback++];}
    auto& p=peers_[i];p.sentAt=-1;p.sends=0;
    if(p.state.load(std::memory_order_acquire)==1){p.enabled=name==p.name;continue;}
    p.enabled=true;strcpy(p.name,name.c_str());p.state.store(1,std::memory_order_release);
    if(tcpip_callback(resolve,&p)!=ERR_OK)p.state.store(3,std::memory_order_release);
  }
  return true;
}
String NtpTransport::servers()const{
  String names;for(const auto& p:peers_)if(p.enabled){if(names.length())names+=", ";names+=p.name;}return names;
}
bool NtpTransport::poll(int64_t& unixUs,String& source){
  const int64_t now=esp_timer_get_time();
  for(int n=0;n<4;n++){
    const int size=udp_.parsePacket();if(!size)break;
    uint8_t packet[48]={};const IPAddress ip=udp_.remoteIP();const uint16_t port=udp_.remotePort();
    const int read=udp_.read(packet,sizeof(packet));udp_.flush();
    if(size<48||read!=48||port!=123)continue;
    for(auto& p:peers_)if(p.enabled&&p.sends&&p.state.load(std::memory_order_acquire)==2&&ip==IPAddress(p.address)&&NtpPacket::decode(packet,48,p.token,now-p.sentAt,unixUs)){
      source=p.name;return true;
    }
  }
  for(auto& p:peers_)if(p.enabled&&p.state.load(std::memory_order_acquire)==2&&p.sends<3&&(p.sentAt<0||now-p.sentAt>=2000000)){
    uint8_t packet[48]={};packet[0]=0x23;
    uint32_t a=esp_random(),b=esp_random();memcpy(p.token,&a,4);memcpy(p.token+4,&b,4);memcpy(packet+40,p.token,8);
    p.sentAt=now;p.sends++;
    if(udp_.beginPacket(IPAddress(p.address),123)){udp_.write(packet,sizeof(packet));udp_.endPacket();}
  }
  return false;
}
