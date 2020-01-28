#ifndef __LFSRGEN64_H
#define __LFSRGEN64_H

#include <cstdint>
#include <cstring>
#include <cinttypes>

template<typename returnType>     // returnType maybe struct, so can be arbitrarily long
class LFSRGen    // gen as Linear-feedback shift register
{
public:
  uint32_t maxvalue;
  uint32_t currentid;
  uint64_t regArray[(sizeof(returnType) + 7) / 8];
  uint64_t reg0[(sizeof(returnType) + 7) / 8];
  uint32_t prest;
  uint32_t regCountPerWord;
  uint32_t returnLength;
  uint32_t tailLength;
  
  LFSRGen(const LFSRGen &other) {
    maxvalue = other.maxvalue;
    currentid = other.currentid;
    
    for (int i = 0; i < (sizeof(returnType) + 7) / 8; i++) {
      regArray[i] = other.regArray[i];
      reg0[i] = other.reg0[i];
    }
    
    prest = other.prest;
    regCountPerWord = other.regCountPerWord;
    returnLength = other.returnLength;
    tailLength = other.tailLength;
  }
  
  LFSRGen(uint64_t _seed, uint32_t _max, uint32_t prestart) {
    maxvalue = _max;
    returnLength = ((sizeof(returnType) + 7) / 8);
    
    for (int i = 0; i < returnLength || i == 0; i++) {
      reg0[i] = _seed;
      unsigned __int128 sv = _seed;
      sv *= (0xA1B2C3D4 ^ sv);
      sv >>= 32;
      _seed = sv + _seed;
    }
    
    tailLength = sizeof(returnType) % 8;
    
    if (tailLength) {
      reg0[returnLength] &= ((1ULL << (tailLength * 8)) - 1);
      regArray[returnLength] &= ((1ULL << (tailLength * 8)) - 1);
    }
    
    prest = prestart;
    reset();
    
    while (currentid < prest) {
      shift();
    }
  }
  
  inline void gen(returnType *ret) {
    memcpy((void *) ret, (void *) regArray, sizeof(returnType));
    shift();
  }
  
  void reset() {
    currentid = 0;
    
    for (int i = 0; i < returnLength; i++) {
      regArray[i] = reg0[i];
    }
  }

private:
  inline void shift() {
    for (int i = 0; i < returnLength; i++) {
      uint64_t lsb = regArray[i] & 1LL;
      regArray[i] >>= 1;
      
      if ((tailLength) && (i == returnLength - 1)) {
        if (lsb) {
          uint64_t taps;
          switch (tailLength) {
            case 1:
              taps = 0xB8;
              break;
            case 2:
              taps = 0xB400;
              break;
            case 3:
              taps = 0xD80000;
              break;
            case 4:
              taps = 0x80200003;
              break;
            case 5:
              taps = 0x9C00000000ULL;
              break;
            case 6:
              taps = (1ULL << 47) + (1ULL << 43) + (1ULL << 40) + (1ULL << 38);
              break;
            case 7:
              taps = (1ULL << 55) + (1ULL << 53) + (1ULL << 51) + (1ULL << 48);
              break;
            default:
              throw runtime_error("impossible");
          }
          
          regArray[i] ^= taps;
        }
      } else {
        regArray[i] ^=
          (-lsb) & 0xD800000000000000ULL;  // every element is an independent Linear-feedback shift register
      }
    }
    
    currentid++;
    
    if (currentid == maxvalue) {    // force wind
      currentid = 0;
      
      for (int i = 0; i < returnLength; i++) {
        regArray[i] = reg0[i];
      }
    }
  }
};

#endif
