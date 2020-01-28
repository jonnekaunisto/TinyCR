/*!
 \file hash.h
 Describes hash functions used in this project.
 */

#pragma once

#include <functional>
#include <type_traits>
#include <cinttypes>
#include <string>
#include <iostream>
#include "../include/farmhash.h"

//! \brief A hash function that hashes keyType to uint32_t. When SSE4.2 support is found, use sse4.2 instructions, otherwise use default hash function  std::hash.
template<class K>
class Hasher32 {
public:
  uint32_t s;    //!< hash s.
  
  Hasher32()
    : s(0xe2211) {
  }
  
  explicit Hasher32(uint32_t _s)
    : s(_s) {
  }
  
  //! set bitmask and s
  void setSeed(uint32_t _s) {
    s = _s;
  }
  
  template<class K1>
  inline typename std::enable_if<!std::is_same<K1, std::string>::value, uint64_t *>::type
  getBase(const K &k0) const {
    uint64_t *base;
    return (uint64_t *) &k0;
  }
  
  template<class K1>
  inline typename std::enable_if<std::is_same<K1, std::string>::value, uint64_t *>::type
  getBase(const K &k0) const {
    uint64_t *base;
    return (uint64_t *) &k0[0];
  }
  
  template<class K1>
  inline typename std::enable_if<!std::is_same<K1, std::string>::value, uint16_t>::type
  getKeyByteLength(const K &k0) const {
    return sizeof(K);
  }
  
  template<class K1>
  inline typename std::enable_if<std::is_same<K1, std::string>::value, uint16_t>::type
  getKeyByteLength(const K &k0) const {
    return k0.length();
  }
  
  inline uint32_t operator()(const K &k0) const {
    static_assert(sizeof(K) <= 32, "K length should be 32/64/96/128/160/192/224/256 bits");

    uint64_t *base = getBase<K>(k0);
    const uint16_t keyByteLength = getKeyByteLength<K>(k0);
    return farmhash::Hash32WithSeed((char *) base, (size_t) keyByteLength, s);
  }
};

//! \brief A hash function that hashes keyType to uint32_t. When SSE4.2 support is found, use sse4.2 instructions, otherwise use default hash function  std::hash.
template<class K>
class Hasher64 {
public:
  uint64_t s;    //!< hash s.
  
  Hasher64()
    : s(0xe2211e2211) {
  }
  
  explicit Hasher64(uint64_t _s)
    : s(_s) {
  }
  
  //! set bitmask and s
  void setSeed(uint64_t _s) {
    s = _s;
  }
  
  template<class K1>
  inline typename std::enable_if<!std::is_same<K1, std::string>::value, uint64_t *>::type
  getBase(const K &k0) const {
    uint64_t *base;
    return (uint64_t *) &k0;
  }
  
  template<class K1>
  inline typename std::enable_if<std::is_same<K1, std::string>::value, uint64_t *>::type
  getBase(const K &k0) const {
    uint64_t *base;
    return (uint64_t *) &k0[0];
  }
  
  template<class K1>
  inline typename std::enable_if<!std::is_same<K1, std::string>::value, uint16_t>::type
  getKeyByteLength(const K &k0) const {
    return sizeof(K);
  }
  
  template<class K1>
  inline typename std::enable_if<std::is_same<K1, std::string>::value, uint16_t>::type
  getKeyByteLength(const K &k0) const {
    return k0.length();
  }
  
  inline uint64_t operator()(const K &k0) const {
    static_assert(sizeof(K) <= 32, "K length should be 32/64/96/128/160/192/224/256 bits");

    uint64_t *base = getBase<K>(k0);
    const uint16_t keyByteLength = getKeyByteLength<K>(k0);
    return farmhash::Hash64WithSeed((char *) base, (size_t) keyByteLength, s);
  }
};
