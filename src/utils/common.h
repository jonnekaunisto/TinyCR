/*
 * config.h
 *
 *  Created on: Nov 5, 2017
 *      Author: ssqstone
 */

#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>
#include <cassert>
#include <cinttypes>
#include <sys/time.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

#include <functional>
#include <algorithm>
#include <iterator>
#include <utility>
#include <random>

#include <queue>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <tuple>
#include <vector>
#include <array>
#include <queue>
#include <set>
#include <list>
#include <forward_list>

#include "disjointset.h"
#include "hash.h"
#include "lfsr64.h"
#include "debugbreak.h"
#include "json.hpp"

using json = nlohmann::json;

//int VIP_NUM = 128;                       // must be power of 2
//int VIP_MASK = (VIP_NUM - 1);
//int CONN_NUM = (16 * 1024 * VIP_NUM);    // must be multiple of VIP_NUM
//int DIP_NUM = (VIP_NUM * 128);
//int LOG_INTERVAL = (50 * 1000000);       // must be multiple of 1E6
//int HT_SIZE = 4096;                    // must be power of 2
//int STO_NUM = (CONN_NUM);                // simulate control plane

//template<int coreId = 0>
//inline void getVip(Addr_Port *vip) {
//  static int addr = 0x0a800000 + coreId * 10;
//  vip->addr = addr++;
//  vip->port = 0;
//  if (addr >= 0x0a800000 + VIP_NUM) addr = 0x0a800000;
//}
//
//template<int coreId = 0>
//inline void getTuple3(Tuple3 *tuple3) {
//  static LFSRGen<Tuple3> tuple3Gen(0xe2211, CONN_NUM, coreId * 10);
//  tuple3Gen.gen(tuple3);
//}
//
//template<int coreId = 0>
//inline void get(Tuple3 *tuple, Addr_Port *vip) {
//  getTuple3<coreId>(tuple); // this is why we assume CONN_NUM is multiple of VIP_NUM
//  getVip<coreId>(vip);
//  if (tuple->protocol & 1) tuple->protocol = 17;
//  else tuple->protocol = 6;
//}

inline uint64_t diff_us(timeval t1, timeval t2) {
  return ((t1.tv_sec - t2.tv_sec) * 1000000ULL + (t1.tv_usec - t2.tv_usec));
}

inline uint64_t diff_ms(timeval t1, timeval t2) {
  return diff_us(t1, t2) / 1000ULL;
}

std::string human(uint64_t word);

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <sched.h>
#include <unistd.h>
#include <execinfo.h>

int stick_this_thread_to_core(int core_id);

void sync_printf(const char *format, ...);

void commonInit();

enum Distribution {
  exponential,
  uniform
};

/** Zipf-like random distribution.
 *
 * "Rejection-inversion to generate variates from monotone discrete
 * distributions", Wolfgang Hörmann and Gerhard Derflinger
 * ACM TOMACS 6.3 (1996): 169-184
 */
template<class IntType = unsigned long, class RealType = double>
class zipf_distribution {
public:
  typedef RealType input_type;
  typedef IntType result_type;
  
  static_assert(std::numeric_limits<IntType>::is_integer, "");
  static_assert(!std::numeric_limits<RealType>::is_integer, "");
  
  zipf_distribution(const IntType n = std::numeric_limits<IntType>::max(),
                    const RealType q = 1.0)
    : n(n), q(q), H_x1(H(1.5) - 1.0), H_n(H(n + 0.5)), dist(H_x1, H_n) {}
  
  IntType operator()(std::default_random_engine &rng) {
    while (true) {
      const RealType u = dist(rng);
      const RealType x = H_inv(u);
      const IntType k = clamp<IntType>(std::round(x), 1, n);
      if (u >= H(k + 0.5) - h(k)) {
        return k;
      }
    }
  }

private:
  /** Clamp x to [min, max]. */
  template<typename T>
  static constexpr T clamp(const T x, const T min, const T max) {
    return std::max(min, std::min(max, x));
  }
  
  /** exp(x) - 1 / x */
  static double
  expxm1bx(const double x) {
    return (std::abs(x) > epsilon)
           ? std::expm1(x) / x
           : (1.0 + x / 2.0 * (1.0 + x / 3.0 * (1.0 + x / 4.0)));
  }
  
  /** H(x) = log(x) if q == 1, (x^(1-q) - 1)/(1 - q) otherwise.
   * H(x) is an integral of h(x).
   *
   * Note the numerator is one less than in the paper order to work with all
   * positive q.
   */
  const RealType H(const RealType x) {
    const RealType log_x = std::log(x);
    return expxm1bx((1.0 - q) * log_x) * log_x;
  }
  
  /** log(1 + x) / x */
  static RealType
  log1pxbx(const RealType x) {
    return (std::abs(x) > epsilon)
           ? std::log1p(x) / x
           : 1.0 - x * ((1 / 2.0) - x * ((1 / 3.0) - x * (1 / 4.0)));
  }
  
  /** The inverse function of H(x) */
  const RealType H_inv(const RealType x) {
    const RealType t = std::max(-1.0, x * (1.0 - q));
    return std::exp(log1pxbx(t) * x);
  }
  
  /** That hat function h(x) = 1 / (x ^ q) */
  const RealType h(const RealType x) {
    return std::exp(-q * std::log(x));
  }
  
  static constexpr RealType epsilon = 1e-8;
  
  IntType n;     ///< Number of elements
  RealType q;     ///< Exponent
  RealType H_x1;  ///< H(x_1)
  RealType H_n;   ///< H(n)
  std::uniform_real_distribution<RealType> dist;  ///< [H(x_1), H(n)]
};

class InputBase {
public:
  static Distribution distribution;
  static std::default_random_engine generator;
  static int bound;
  static zipf_distribution<int, double> expo;
  static std::uniform_int_distribution<int> unif;
  
  inline static void setSeed(uint32_t seed) {
    generator.seed(seed);
  }
  
  inline static uint32_t rand() {
    if (distribution == uniform) {
      return static_cast<uint32_t>(unif(generator)) % bound;
    } else {
      return static_cast<uint32_t>(expo(generator)) % bound;
    }
  }
};

template<typename InType,
  template<typename U, typename alloc = allocator<U>> class InContainer,
  typename OutType = InType,
  template<typename V, typename alloc = allocator<V>> class OutContainer = InContainer>
OutContainer<OutType> mapf(const InContainer<InType> &input, function<OutType(const InType &)> func) {
  OutContainer<OutType> output;
  output.resize(input.size());
  transform(input.begin(), input.end(), output.begin(), func);
  return output;
}

#include <chrono>

class TeeOstream {
public:
  TeeOstream(string name = "/dev/null") : my_fstream(name) {}
  
  // for regular output of variables and stuff
  template<typename T>
  TeeOstream &operator<<(const T &something) {
    std::cout << something;
    my_fstream << something;
    return *this;
  }
  
  // for manipulators like std::endl
  typedef std::ostream &(*stream_function)(std::ostream &);
  
  TeeOstream &operator<<(stream_function func) {
    func(std::cout);
    func(my_fstream);
    return *this;
  }
  
  void flush() {
    my_fstream.flush();
    std::cout.flush();
  }
  
private:
  std::ofstream my_fstream;
};

/// for runtime statistics collection
class Counter {
public:
  unordered_map<string, unordered_map<string, double>> mem;
  static list<Counter> counters;
  TeeOstream &os;
  
  explicit Counter(TeeOstream &os) : os(os) {}
  
  static inline void count(const string &solution, const string &type, double acc = 1) {
    #ifdef PROFILE
    auto it = counters.back().mem.find(solution);
    if (it == counters.back().mem.end()) {
      counters.back().mem.insert(make_pair(solution, unordered_map<string, double>()));
      it = counters.back().mem.find(solution);
    }
    
    unordered_map<string, double> &typeToCount = it->second;
    
    if (typeToCount.find(type) == typeToCount.end())
      typeToCount.insert(make_pair(type, uint64_t(0)));
    
    counters.back().mem[solution][type] += acc;
    #endif
  }
  
  ~Counter() {
    lap();
  }
  
  string pad() const;
  
  void lap() {
    if (mem.empty()) return;
    
    #ifdef PROFILE
    
    for (auto it = mem.begin(); it != mem.end(); ++it) {
      const string &solution = it->first;
      
      for (auto iit = it->second.begin(); iit != it->second.end(); ++iit) {
        const string type = iit->first;
        os << pad() << "->" << "[" << solution << "] [" << type << "] " << iit->second << endl;
      }
    }
    
    mem.clear();
    #endif
  }
};

class Clocker {
  int level;
  struct timeval start;
  string name;
  bool stopped = false;
  
  int laps = 0;
  uint64_t us = 0;
  
  TeeOstream &os;

public:
  explicit Clocker(const string &name, TeeOstream *os = nullptr)
    : name(name), level(currentLevel++), os(os ? *os : Counter::counters.back().os) {
    if(Counter::counters.size()) Counter::counters.back().os.flush();
    
    for (int i = 0; i < level; ++i) this->os << "| ";
    this->os << "++";
    
    gettimeofday(&start, nullptr);
    this->os << " [" << name << "]" << endl;
    
    Counter::counters.emplace_back(this->os);
  }
  
  void lap() {
    timeval end;
    gettimeofday(&end, nullptr);
    us += diff_us(end, start);
    
    output();
    
    laps++;
  }
  
  void resume() {
    gettimeofday(&start, nullptr);
  }
  
  void stop() {
    Counter::counters.back().lap();
    Counter::counters.pop_back();
    
    lap();
    stopped = true;
    currentLevel--;
  }
  
  ~Clocker() {
    if (!stopped)
      stop();
  }
  
  void output() const {
    for (int i = 0; i < level; ++i) os << "| ";
    os << "--";
    os << " [" << name << "]" << (laps ? "@" + to_string(laps) : "") << ": "
       << us / 1000 << "ms or " << us << "us"
       << endl;
  }
  
  static int currentLevel;
};
