//
// Created by ssqstone on 2018/7/19.
//

#pragma once

#include "common.h"

using namespace std;

#pragma pack(push, 1)

class MAC {
public:
  uint8_t addr[6];
  
  inline MAC() {}
  
  inline explicit MAC(uint32_t seed) {
    uint8_t *s = reinterpret_cast<uint8_t *>(&seed);
    
    memcpy(addr, s, 4);
    addr[4] = s[0] ^ s[1];
    addr[5] = s[2] ^ s[3];
  }
  
  inline static MAC sample() {
    return MAC(InputBase::rand());
  }
  
  inline static MAC enumerate(uint32_t seed) {
    return MAC(seed);
  }
  
  inline bool operator!=(const MAC &other) const {
    return !(*this == other);
  }
  
  inline bool operator==(const MAC &other) const {
    for (int i = 0; i < 6; ++i) {
      if (addr[i] != other.addr[i]) return false;
    }
    return true;
  }
};

inline ostream &operator<<(ostream &os, const MAC &mac) {
  os << hex;
  for (int i = 0; i < 6; ++i) {
    os << mac.addr[i];
  }
  os << dec;
  return os;
}

// 5cd50dc8-69fd-40ee-adb9-523f62f67e6a_video_30
class ID {
public:
  static inline string sample() {
    return enumerate(InputBase::rand());
  }
  
  inline static string enumerate(uint32_t seed) {
    string id(45, '-');
    
    uint32_t s1 = seed;
    uint32_t s2 = ~seed;
    uint32_t s3 = s1 * s2;
    uint32_t s4 = ~s3;
    
    uint32_t s[] = {s1, s2, s3, s4};
    
    for (int i = 0; i < 32; ++i) {
      int realI = i;
      if (i >= 8) realI += 1 + (i - 8) / 4;
      
      uint8_t hex = static_cast<uint8_t>(s[i / 8] >> ((i % 8) * 4)) & 15;
      
      id[realI] = hex < 10 ? '0' + hex : 'a' + hex - 10;
    }
    
    return id;
  }
};

class URL {
public:
  static inline string sample() {
    return "https://d2lkq7nlcrdi7q.cloudfront.net/dm/2$KNA2j5deIAM-rbspbnnKxFBPg4M~/"
           "4bd5/ac42/8a57/485b-8fe3-3f823e0db83c/" + ID::sample();
  }
  
  inline static string enumerate(uint32_t seed) {
    return "https://d2lkq7nlcrdi7q.cloudfront.net/dm/2$KNA2j5deIAM-rbspbnnKxFBPg4M~/"
           "4bd5/ac42/8a57/485b-8fe3-3f823e0db83c/" + ID::enumerate(seed);
  }
};

class IPv4 {
public:
  static inline uint32_t sample() {
    return enumerate(InputBase::rand());
  }
  
  inline static uint32_t enumerate(uint32_t seed) {
    return seed;
  }
};

class IPv6 {
public:
  static inline unsigned __int128 sample() {
    return enumerate(InputBase::rand());
  }
  
  inline static unsigned __int128 enumerate(uint32_t seed) {
    return ((unsigned __int128) seed << 96) + ((unsigned __int128) seed << 64) + ((unsigned __int128) seed << 32) +
           seed;
  }
};

inline ostream &operator<<(ostream &os, const unsigned __int128 &i) {
  os << std::hex << (i >> 64) << (uint64_t) i << std::dec;
  return os;
}

class Addr_Port {  // 6B
public:
  uint32_t addr;
  uint16_t port;
  
  inline Addr_Port() {}
  
  inline Addr_Port(uint32_t addr, uint16_t port) : addr(addr), port(port) {}
  
  inline bool operator==(const Addr_Port &another) const {
    return std::tie(addr, port) == std::tie(another.addr, another.port);
  }
  
  inline bool operator<(const Addr_Port &another) const {
    return std::tie(addr, port) < std::tie(another.addr, another.port);
  }
  
  static inline Addr_Port sample() {
    return enumerate(InputBase::rand());
  }
  
  inline static Addr_Port enumerate(uint32_t seed) {
    return {seed, (uint16_t) seed};
  }
};

inline ostream &operator<<(ostream &os, const Addr_Port &addr) {
  os << std::hex << addr.addr << ":" << addr.port << std::dec;
  return os;
}

class Tuple5 {  // 13B
public:
  Addr_Port dst, src;
  uint16_t protocol = 6;
  
  inline Tuple5() {}
  
  inline Tuple5(Addr_Port dst, Addr_Port src, uint8_t protocol) : dst(dst), src(src), protocol(protocol) {}
  
  inline bool operator==(const Tuple5 &another) const {
    return std::tie(dst, src, protocol) == std::tie(another.dst, another.src, another.protocol);
  }
  
  inline bool operator<(const Tuple5 &another) const {
    return std::tie(dst, src, protocol) < std::tie(another.dst, another.src, another.protocol);
  }
  
  static inline Tuple5 sample() {
    return enumerate(InputBase::rand());
  }
  
  inline static Tuple5 enumerate(uint32_t seed) {
    return {Addr_Port::enumerate(seed), Addr_Port::enumerate(~seed), uint8_t((seed & 1) ? 6 : 17)};
  }
};

inline ostream &operator<<(ostream &os, const Tuple5 &tuple5) {
  os << tuple5.src << "->" << tuple5.dst << "@" << tuple5.protocol;
  return os;
}

#pragma pack(pop)
