"""Deterministic DNS/UDP fixtures for the real parallel NTP transport."""
files = {
'WiFiUdp.h': r'''
#pragma once
#include <vector>
#include <deque>
struct IPAddress {uint32_t value;IPAddress(uint32_t n=0):value(n){}bool operator==(const IPAddress& o)const{return value==o.value;}};
struct Datagram {IPAddress ip;uint16_t port=123;std::vector<uint8_t> data;};
inline std::vector<Datagram> sent;inline std::deque<Datagram> received;
class WiFiUDP {
 Datagram outgoing,current;
public:
 int begin(int){return 1;}void stop(){received.clear();}
 int parsePacket(){if(received.empty())return 0;current=received.front();received.pop_front();return int(current.data.size());}
 IPAddress remoteIP(){return current.ip;}uint16_t remotePort(){return current.port;}
 int read(uint8_t* p,size_t n){n=n<current.data.size()?n:current.data.size();memcpy(p,current.data.data(),n);return int(n);}
 void flush(){}int beginPacket(IPAddress ip,int port){outgoing={ip,uint16_t(port),{}};return 1;}
 void write(uint8_t* p,size_t n){outgoing.data.assign(p,p+n);}void endPacket(){sent.push_back(outgoing);}
};
''',
'esp_system.h': '#pragma once\ninline uint32_t esp_random(){static uint32_t n=13;return n=n*1664525+1013904223;}\n',
'lwip/dns.h': r'''
#pragma once
#include "Arduino.h"
using err_t=int;enum {ERR_OK=0,ERR_INPROGRESS=-5,LWIP_DNS_ADDRTYPE_IPV4=0};
struct ip_addr_t{uint32_t value;};
#define IP_IS_V4(p) true
#define ip_2_ip4(p) p
inline uint32_t ip4_addr_get_u32(const ip_addr_t* p){return p->value;}
using Callback=void(*)(const char*,const ip_addr_t*,void*);
inline Callback pending=nullptr;inline void* pendingArg;inline const char* pendingName;
inline int dns_gethostbyname_addrtype(const char* name,ip_addr_t* ip,Callback callback,void* arg,int){
 if(!strcmp(name,"stall.example")){pending=callback;pendingArg=arg;pendingName=name;return ERR_INPROGRESS;}
 ip->value=1;for(auto c:std::string(name))ip->value=ip->value*31+c;return ERR_OK;
}
''',
'lwip/tcpip.h':'#pragma once\ninline int tcpip_callback(void(*fn)(void*),void* arg){fn(arg);return 0;}\n',
'test.cpp': r'''
#include "Arduino.h"
#include <cassert>
int64_t clockUs=0;time_t epoch=1762300800;bool enabled=false;
int syncStatus=0,starts=0;String hostname;uint32_t interval=0;
time_t fakeTime(time_t* target){if(target)*target=epoch;return epoch;}
#define time fakeTime
#include "TimeSync.cpp"
#undef time
#include "NtpTransport.cpp"
static bool has(TimeSync& clock,const char* text){return clock.json().find(text)!=std::string::npos;}
static Datagram response(size_t i){auto d=sent.at(i);d.data.assign(48,0);d.data[0]=0x24;d.data[1]=2;memcpy(d.data.data()+24,sent[i].data.data()+40,8);uint32_t t=uint32_t(1762300800ULL+2208988800ULL);for(int j=0;j<4;j++)d.data[40+j]=uint8_t(t>>(24-j*8));return d;}
int main(){
 Preferences prefs;TimeSync clock;String error;clock.begin(prefs);
 clock.poll(false);assert(sent.empty()&&has(clock,"waiting-network"));
 clockUs=1000000;clock.poll(true);clock.poll(true);assert(sent.size()==4&&has(clock,"ntp.tencent.com")&&has(clock,"pool.ntp.org"));
 received.push_back(response(2));clockUs+=100000;clock.poll(true);assert(has(clock,"synced")&&has(clock,"time.cloudflare.com"));
 size_t count=sent.size();clockUs+=86400000000LL-1;clock.poll(true);assert(sent.size()==count);
 clockUs++;clock.poll(true);clock.poll(true);assert(sent.size()==count+4);
 for(int kind=0;kind<7;kind++){auto d=response(count);if(kind==0)d.data[0]=0x23;if(kind==1)d.data[0]=0xe4;if(kind==2)d.data[1]=0;if(kind==3)d.data[24]^=1;if(kind==4)d.ip.value++;if(kind==5)d.port=124;if(kind==6)d.data.resize(12);received.push_back(d);clock.poll(true);assert(has(clock,"syncing"));}
 clockUs+=8000000;clock.poll(true);assert(has(clock,"retry"));count=sent.size();
 clockUs+=4999999;clock.poll(true);assert(sent.size()==count);clockUs++;clock.poll(true);clock.poll(true);assert(sent.size()==count+4);
 received.push_back(response(count));clock.poll(false);assert(has(clock,"waiting-network"));clock.poll(true);clock.poll(true);assert(has(clock,"syncing"));
 assert(!clock.manual("2026-11-05T12:34:56","",error));
 assert(!clock.configure(true,"https://ntp.example",error));assert(!clock.configure(true,"a..example",error));
 assert(!clock.configure(false,"192.168.1.1",error,"bad\"zone"));
 assert(clock.configure(false,"192.168.1.1",error,"UTC0"));count=sent.size();clockUs+=2*86400000000LL;clock.poll(true);assert(sent.size()==count&&!clock.sync(error));
 assert(!clock.manual("2026-02-30T12:00:00","",error));assert(clock.manual("2026-11-05T12:34:56","",error));assert(has(clock,"2026-11-05 12:34:56"));
 assert(clock.manual("","1762300800",error)&&epoch==1762300800);assert(!clock.manual("","99999999999999999999999",error));
 TimeSync reboot;reboot.begin(prefs);reboot.poll(true);assert(sent.size()==count&&has(reboot,"UTC0"));
 assert(reboot.configure(true,"stall.example",error));reboot.poll(true);reboot.poll(true);assert(sent.size()==count+3&&pending);
 assert(reboot.configure(true,"changed.example",error));reboot.poll(true);reboot.poll(true);assert(!strcmp(pendingName,"stall.example"));
 ip_addr_t ip{123};pending(nullptr,&ip,pendingArg);received.push_back(response(sent.size()-1));reboot.poll(true);assert(has(reboot,"synced"));
 int64_t parsed=0;auto d=response(0);assert(!NtpPacket::decode(d.data.data(),0,sent[0].data.data()+40,1,parsed));assert(!NtpPacket::decode(d.data.data(),48,sent[0].data.data()+40,3000001,parsed));
 uint32_t seconds=uint32_t(2114380800ULL+2208988800ULL);for(int j=0;j<4;j++)d.data[40+j]=uint8_t(seconds>>(24-j*8));d.data[44]=128;
 assert(NtpPacket::decode(d.data.data(),48,sent[0].data.data()+40,100000,parsed)&&parsed==2114380800550000LL);
 puts("PASS: four parallel peers, first valid response, rejected packets, 24h/offline/retry, timezone persistence, cancelled DNS callback, NTP era");
}
'''
}
