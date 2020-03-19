#pragma once

#include <vector>
#include <cinttypes>

using namespace std;
/*! \file disjointset.h
 *  Disjoint Set data structure.
 */
/*!
 * \brief Disjoint Set data structure. Helps to test the acyclicity of the graph during construction.
 * */
class DisjointSet {
  vector<int> mem;
public:
  inline void __set(int i, int parent) {
    mem[i] = parent;
  }
  
  int representative(int i) {
    if (mem[i] < 0) {
      return mem[i] = i;
    } else if (mem[i] != i) {
      return mem[i] = representative(mem[i]);
    } else {
      return i;
    }
  }
  
  inline void merge(int a, int b) {
    mem[representative(b)] = representative(a);
  }
  
  inline bool sameSet(int a, int b) {
    return representative(a) == representative(b);
  }
  
  inline bool isRoot(int a) const {
    return (mem[a] == a);
  }
  
  //! re-initilize the disjoint sets.
  inline void reset() {
    for (int &a : mem)
      a = -1;
  }
  
  //! add new keys, so that the total number of elements equal to n.
  inline void resize(size_t n) {
    mem.resize(n, -1);
  }
  
  DisjointSet() {}
  
  explicit DisjointSet(unsigned long capacity) {
    resize(capacity);
  }
};
