#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace NtpPacket {
inline uint32_t word(const uint8_t* p){return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|p[3];}
inline bool decode(const uint8_t* p,size_t size,const uint8_t* token,int64_t rttUs,int64_t& unixUs){
  if(size<48)return false;
  const int version=(p[0]>>3)&7;
  if(size<48||(p[0]&7)!=4||(p[0]>>6)==3||version<3||version>4||!p[1]||p[1]>15||memcmp(p+24,token,8)||rttUs<0||rttUs>3000000)return false;
  uint64_t seconds=word(p+40);if(seconds<2208988800ULL)seconds+=4294967296ULL;
  seconds-=2208988800ULL;
  if(seconds<1704067200ULL||seconds>2145916799ULL)return false;
  unixUs=int64_t(seconds)*1000000+(uint64_t(word(p+44))*1000000>>32)+rttUs/2;
  return true;
}
}
