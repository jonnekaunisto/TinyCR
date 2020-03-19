//
// Created by ssqstone on 2018/7/17.
//
#include "../utils/common.h"

#pragma once

template<class Key, int L = 1>
class BloomFilter {
public:
  static_assert(L <= 64, "specified L is too long for a counting Bloom filter! Maybe 4 is enough! ");
  vector<Hasher32<Key>> h;   // the last h is the digest function used in associated data plane
  uint32_t capacity;
  uint32_t m;
  vector<uint64_t> mem;
  int k;  // real k
  
  const static uint64_t MASK = uint64_t(-1) >> (64 - L);
  
  inline uint32_t multiply_high_u32(uint32_t x, uint32_t y) const {
    return (uint32_t) (((uint64_t) x * (uint64_t) y) >> 32);
  }
  
  inline uint32_t fast_map_to_buckets(uint32_t x) const {
    return multiply_high_u32(x, m);
  }
  
  inline void increaseAt(uint32_t index) {
    memSet(index, 1);
    // uint64_t result = getAt(index);
    
    // if (result < (1 << L) - 1) {
    //   result += 1;
    //   memSet(index, result);
    // }
  }
  
  //! return if the corresponding bloom filter slot should be emptied
  inline bool decreaseAt(uint32_t index) {
    uint64_t result = getAt(index);
    
    if (result > 0) {
      result -= 1;
      memSet(index, result);
    }
    
    return result == 0;
  }
  
  inline void memSet(uint32_t index, uint64_t value) {
    uint64_t v = uint64_t(value) & MASK;
    
    uint32_t start = index * L / 64;
    uint8_t offset = uint8_t(index * L % 64);
    char left = char(offset + L - 64);
    
    uint64_t mask = ~(MASK << offset); // [offset, offset + L) should be 0, and others are 1
    
    mem[start] &= mask;
    mem[start] |= v << offset;
    
    if (L > 1 && left > 0) {
      mask = uint64_t(-1) << left;     // lower left bits should be 0, and others are 1
      mem[start + 1] &= mask;
      mem[start + 1] |= v >> (L - left);
    }
  }
  
  inline uint64_t getAt(uint32_t index) const {
    uint32_t start = index * L / 64;
    uint8_t offset = uint8_t(index * L % 64);
    
    char left = char(L > 1 ? char(offset + L - 64) : 0);
    left = char(left < 0 ? 0 : left);
    
    uint64_t mask = ~(uint64_t(-1) << (L - left));     // lower L-left bits should be 1, and others are 0
    uint64_t result = (mem[start] >> offset) & mask;
    
    if (L > 1 && left > 0) {
      mask = ~(uint64_t(-1) << left);     // lower left bits should be 1, and others are 0
      result |= (mem[start + 1] & mask) << (L - left);
    }
    
    return result;
  }

public:
  /// if numberOfHashes = -1, let the algorithm find out the best k
  explicit BloomFilter(uint32_t capacity = 4096, float FP = 0.001, int8_t numberOfHashes = -1) : capacity(capacity) {
    if (numberOfHashes == -1) {
      int tmpM = m = capacity;
      int tmpK = k = 4;
      
      do {
        tmpM = m;
        tmpK = k;
        m = static_cast<uint32_t>(capacity * k / -log(1 - pow(FP, 1.0 / k)));
        k = (int) round(log(2.0) * m / capacity);
      } while (tmpK != k || tmpM != m);
    } else {
      k = numberOfHashes;
      m = static_cast<uint32_t>(capacity * k / -log(1 - pow(FP, 1.0 / k)));
    }
    

    double fp = pow(1 - exp(-1.0 * k * capacity / m), k);
//    cout << "decided m, K, fp: " << m << ", " << K << ", " << fp << endl;
    
    mem.resize((m * L + 63) / 64);

    for (int i = 0; i < k; ++i) {
      h.push_back(Hasher32<Key>(uint32_t(InputBase::rand())));
    }
  }

  BloomFilter(vector<Hasher32<Key>> &_h, uint32_t _capacity, uint32_t _m, vector<uint64_t> &_mem)
  {
    h = _h;
    capacity = _capacity;
    m = _m;
    mem = _mem;
  }
  
  void insert(const Key &key) {
    for (auto &hash : h) {
      increaseAt(fast_map_to_buckets(hash(key)));
    }
  }
  
  //! return the bit pattern to erase: if the i-th is 1, then erase the i-th mapped bit for the Bloom filter
  uint64_t erase(const Key &key) {
    uint64_t toErase = 0;
    uint64_t bit = 1;
    for (auto &hash : h) {
      if (decreaseAt(fast_map_to_buckets(hash(key)))) {
        toErase |= bit;
      }
      bit <<= 1;
    }
    
    return toErase;
  }
  
  bool isMember(const Key &key) const {
    for (auto &hash : h) {
      if (!getAt(fast_map_to_buckets(hash(key)))) return false;
    }
    
    return true;
  }
  
  void mask(const Key &k, uint64_t toErase) {
    assert(L == 1);
    for (int i = 0; toErase; i++, toErase >>= 1) {
      if (toErase & 1)
        memSet(fast_map_to_buckets(h[i](k)), 0);
    }
  }
  
  uint32_t getCapacity() const {
    return capacity;
  }
  
  uint64_t getMemoryCost() const {
	  //cout<<mem.size()<<" "<<sizeof(mem[0])<<endl;
    return m * L / 8;
  }
  uint32_t getK() const {
    return k;
  }
};
