#pragma once
#include <Arduino.h>
#include <WiFiUdp.h>
#include <atomic>
#include <lwip/dns.h>

// DeviceService owns this for the lifetime of the firmware; DNS callbacks keep
// stable slot addresses, including when a synchronization round is cancelled.
class NtpTransport {
public:
  bool begin(const String& preferred);
  bool poll(int64_t& unixUs,String& source);
  void stop(){udp_.stop();}
  String servers()const;
private:
  struct Peer {
    char name[254]={};
    std::atomic<int> state{0}; // 1: DNS pending, 2: resolved, 3: failed
    uint32_t address=0;
    bool enabled=false;
    uint8_t token[8]={};
    int64_t sentAt=-1;
    uint8_t sends=0;
  } peers_[4];
  WiFiUDP udp_;
  static void resolve(void* arg);
  static void resolved(const char*,const ip_addr_t*,void* arg);
};
