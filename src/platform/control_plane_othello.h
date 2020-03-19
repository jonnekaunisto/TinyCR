#pragma once

#include "../utils/common.h"

using namespace std;

template<class K, class V, uint8_t L, uint8_t DL>
class DataPlaneOthello;

template<class K, bool l2, uint8_t DL>
class OthelloFilterControlPlane;

struct OthelloCPCell {
  uint32_t keyId;
  uint32_t nodeId;
};

/**
 * Control plane Othello can track connections (Add [amortized], Delete, Membership Judgment) in O(1) time,
 * and can iterate on the keys in exactly n elements.
 *
 * Implementation: just add an array indMem to be maintained. always ensure that registered keys can
 * be queried to get the index of it in the keys array
 *
 * How to ensure:
 * add to tail when add, and store the value as well as the index to othello
 * when delete, move key-value and update corresponding index
 *
 * @note
 *  The valueType must be compatible with all int operations
 *
 *  If you wish to export the control plane to a data plane query structure at a fast speed and at any time, then
 *  set willExport to true. Additional computation and memory overheads will apply on insert, while lookups will be faster.
 *
 *  If you wish to maintain the disjoint set, the insertion will become faster but the deletion is slower, in the sense that
 *  memory accesses are more expensive than computation
 */
template<class K, class V, uint8_t L = sizeof(V) * 8, uint8_t DL = 0,
  bool maintainDP = true, bool maintainDisjointSet = true, bool randomized = false>
class ControlPlaneOthello {
  template<class K1, class V1, uint8_t L1, uint8_t DL1> friend
  class DataPlaneOthello;
  
  template<class K1, bool l2, uint8_t DL1> friend
  class OthelloFilterControlPlane;

public:
  //*******builtin values
  const static int MAX_REHASH = 50; //!< Maximum number of rehash tries before report an error. If this limit is reached, Othello build fails.
  const static int VDL = L + DL;
  static_assert(VDL <= 60, "Value is too long. You should consider another solution to avoid memory waste. ");
  static_assert(L <= sizeof(V) * 8, "Value is too long. ");
  const static uint64_t VDEMASK = ~(uint64_t(-1) << VDL);   // lower VDL bits are 1, others are 0
  const static uint64_t DEMASK = ~(uint64_t(-1) << DL);   // lower DL bits are 1, others are 0
  const static uint64_t VMASK = ~(uint64_t(-1) << L);   // lower L bits are 1, others are 0
  const static uint64_t VDMASK = (VDEMASK << 1) & VDEMASK; // [1, VDL) bits are 1
  //****************************************
  //*************DATA Plane
  //****************************************
//private:
  vector<uint64_t> mem{};        // memory space for array A and array B. All elements are stored compactly into consecutive uint64_t
  uint32_t ma = 0;               // number of elements of array A
  uint32_t mb = 0;               // number of elements of array B
  Hasher64<K> hab = Hasher64<K>((uint64_t(InputBase::rand()) << 32) + InputBase::rand());          // hash function Ha
  Hasher32<K> hd = Hasher32<K>(uint32_t(InputBase::rand()));
  
  bool maintainingDP = maintainDP;
  
  void setSeed(int seed) {
    seed = (seed != -1) ? seed : InputBase::rand();
    hd.setSeed(seed);
  }
  
  void changeSeed() { setSeed(-1); }
  
  inline uint32_t multiply_high_u32(uint32_t x, uint32_t y) const {
    return (uint32_t) (((uint64_t) x * (uint64_t) y) >> 32);
  }
  
  inline uint32_t fast_map_to_A(uint32_t x) const {
    // Map x (uniform in 2^64) to the range [0, num_buckets_ -1]
    // using Lemire's alternative to modulo reduction:
    // http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
    // Instead of x % N, use (x * N) >> 64.
    return multiply_high_u32(x, ma);
  }
  
  inline uint32_t fast_map_to_B(uint32_t x) const {
    return multiply_high_u32(x, mb);
  }
  
  /// \param k
  /// \param aInd, bInd return indices of k into array A&B
  inline void getIndices(const K &k, uint32_t &aInd, uint32_t &bInd) const {
    uint64_t hash = hab(k);
    bInd = fast_map_to_B(hash >> 32) + ma;
    aInd = fast_map_to_A(hash);
  }
  
  /// \return the number of uint64_t elements to hold ma + mb valueType elements
  inline void memResize() {
    if (!maintainingDP) return;
    mem.resize(((ma + mb) * VDL + 63) / 64);
  }
  
  /// Set the index-th element to be value. if the index > ma, it is the (index - ma)-th element in array B
  /// \param index in array A or array B
  /// \param value
  inline void memSet(uint32_t index, uint64_t value) {
    if (VDL == 0) return;
    
    uint64_t v = uint64_t(value) & VDEMASK;
    
    uint32_t start = index * VDL / 64;
    uint8_t offset = uint8_t(index * VDL % 64);
    char left = char(offset + VDL - 64);
    
    uint64_t mask = ~(VDEMASK << offset); // [offset, offset + VDL) should be 0, and others are 1
    
    mem[start] &= mask;
    mem[start] |= v << offset;
    
    if (left > 0) {
      mask = uint64_t(-1) << left;     // lower left bits should be 0, and others are 1
      mem[start + 1] &= mask;
      mem[start + 1] |= v >> (VDL - left);
    }

  }
  
  /// \param index in array A or array B
  /// \return the index-th element. if the index > ma, it is the (index - ma)-th element in array B
  inline uint64_t memGet(uint32_t index) const {
    if (VDL == 0) return 0;

    uint32_t start = index * VDL / 64;
    uint8_t offset = uint8_t(index * VDL % 64);
    
    char left = char(offset + VDL - 64);
    left = char(left < 0 ? 0 : left);
    
    uint64_t mask = ~(uint64_t(-1) << (VDL - left));     // lower VDL-left bits should be 1, and others are 0
    uint64_t result = (mem[start] >> offset) & mask;
    
    if (left > 0) {
      mask = ~(uint64_t(-1) << left);     // lower left bits should be 1, and others are 0
      result |= (mem[start + 1] & mask) << (VDL - left);
    }
    
    return result;
  }
  
  inline void memValueSet(uint32_t index, uint64_t value) {
    if (L == 0) return;
    
    uint64_t v = uint64_t(value) & VMASK;
    
    uint32_t start = (index * VDL + DL) / 64;
    uint8_t offset = uint8_t((index * VDL + DL) % 64);
    char left = char(offset + L - 64);
    
    uint64_t mask = ~(VMASK << offset); // [offset, offset + L) should be 0, and others are 1
    
    mem[start] &= mask;
    mem[start] |= v << offset;
    
    if (left > 0) {
      mask = uint64_t(-1) << left;     // lower left bits should be 0, and others are 1
      mem[start + 1] &= mask;
      mem[start + 1] |= v >> (L - left);
    }
  }
  
  inline uint64_t memValueGet(uint32_t index) const {
    if (L == 0) return 0;
    
    uint32_t start = (index * VDL + DL) / 64;
    uint8_t offset = uint8_t((index * VDL + DL) % 64);
    char left = char(offset + L - 64);
    left = char(left < 0 ? 0 : left);
    
    uint64_t mask = ~(uint64_t(-1) << (L - left));     // lower L-left bits should be 1, and others are 0
    uint64_t result = (mem[start] >> offset) & mask;
    
    if (left > 0) {
      mask = ~(uint64_t(-1) << left);     // lower left bits should be 1, and others are 0
      result |= (mem[start + 1] & mask) << (L - left);
    }
    
    return result;
  }

public:
  inline uint32_t getMa() const {
    return ma;
  }
  
  inline uint32_t getMb() const {
    return mb;
  }
  
  inline Hasher64<K> getH() const {
    return hab;
  }
  
  inline Hasher32<K> getHd() const {
    return hd;
  }
  
  /// \param k
  /// \param v the lookup value for k
  /// \return the lookup action is successful, but it does not mean the key is really a member
  /// \note No membership is checked. Use isMember to check the membership
  inline bool query(const K &k, V &out) const {
    if (maintainDP) {
      uint32_t ha, hb;
      getIndices(k, ha, hb);
      V aa = memGet(ha);
      V bb = memGet(hb);
      ////printf("%llx   [%x] %x ^ [%x] %x = %x\n", k,ha,aa&LMASK,hb,bb&LMASK,(aa^bb)&LMASK);
      uint64_t vd = aa ^bb;
      out = vd >> DL;
    } else {

      uint32_t index = queryIndex(k);
      if (index >= values.size()) return false;// throw runtime_error("Index out of bound. Maybe not a member");
      out = values[index];
    }
    return true;
  }

public:
  explicit ControlPlaneOthello(uint32_t keyCapacity = 256, float o_ratio = 1) {
    // for (minimalKeyCapacity = 256; minimalKeyCapacity < keyCapacity; minimalKeyCapacity <<= 1);
   minimalKeyCapacity = (uint32_t)(keyCapacity * o_ratio) + 1;
    resizeKey(0,true);
    
    resetBuildState();
    
    build();
  }
  
  /// Resize key and value related memory for the Othello to be able to hold keyCount keys
  /// \param targetCapacity the target capacity
  /// \note Side effect: will change keyCnt, and if hash size is changed, a rebuild is performed
  void resizeKey(uint32_t targetCapacity, bool compact = false) {
    // compact = true;
    targetCapacity = max(targetCapacity, minimalKeyCapacity);
    
    if (targetCapacity < this->size()) {
      throw runtime_error("The specified capacity is less than current key size! ");
    }
    
    uint32_t nextMb;
    
    if (compact) {
      nextMb = targetCapacity;
    } else {
      nextMb = minimalKeyCapacity;
      while (nextMb < targetCapacity)
        nextMb <<= 1;
    }
    uint32_t nextMa = static_cast<uint32_t>(1.33334 * nextMb);
    
    if (targetCapacity > keys.capacity()) {
      uint32_t keyCntReserve = max(256U, targetCapacity * 2U);
      keys.resize(keyCntReserve);
      values.resize(keyCntReserve);
      nextAtA.resize(keyCntReserve);
      nextAtB.resize(keyCntReserve);
    }
    
    if (nextMa > ma || nextMa < 0.8 * ma) {
      ma = nextMa;
      mb = nextMb;
      
      memResize();
      
      indMem.resize(ma + mb);
      head.resize(ma + mb);
      connectivityForest.resize(ma + mb);
      
      build();
    }
//    cout << human(keyCnt) << " Keys, ma/mb = " << human(ma) << "/" << human(mb) << endl;
  }
  
  //****************************************
  //*************CONTROL plane
  //****************************************
//private:
  uint32_t keyCnt = 0, minimalKeyCapacity = 0;
public:
  void setMinimalKeyCapacity(uint32_t minimalKeyCapacity) {
    this->minimalKeyCapacity = minimalKeyCapacity;
    resizeKey(0);
  }
  
  void clear() {
    keyCnt = 0;
    setMinimalKeyCapacity(0);
  }
  
  void compose(const unordered_map<V, V> &migration) {
    for (int i = 0; i < size(); ++i) {
      uint16_t &val = values[i];
      
      auto it = migration.find(val);
      if (it != migration.end()) {
        uint16_t dst = it->second;
        if (dst == (uint16_t) -1) {
          eraseAt(i);
          --i;
        } else {
          val = dst;
        }
      }
    }
    
    if (maintainingDP)
      fillValue<true>();
  }
  
  void prepareDP() {
    if (maintainDP) return;
    maintainingDP = true;
    
    memResize();
    fillValue();
    
    maintainingDP = false;
  }

//private:
  // ******input of control plane
  vector<K> keys{};
  vector<V> values{};
  vector<uint32_t> indMem{};       // memory space for indices
  
  inline V randVal(int i = 0) const {
    V v = InputBase::rand();
    
    if (sizeof(V) > 4) {
      *(((int *) &v) + 1) = InputBase::rand();
    }
    return v;
  }
  
  /// Forget all previous build states and get prepared for a new build
  void resetBuildState() {
    for (uint32_t i = 0; i < ma + mb; ++i) {
      if (maintainingDP) memSet(i, randomized ? (randVal(i) & VDMASK) : 0);
    }
    
    fill(head.begin(), head.end(), OthelloCPCell({uint32_t(-1), uint32_t(-1)}));
    fill(nextAtA.begin(), nextAtA.end(), OthelloCPCell({uint32_t(-1), uint32_t(-1)}));
    fill(nextAtB.begin(), nextAtB.end(), OthelloCPCell({uint32_t(-1), uint32_t(-1)}));
    connectivityForest.reset();
  }
  
  /*! multiple keys may share a same end (hash value)
   first and next1, next2 maintain linked lists,
   each containing all keys with the same hash in either of their ends
   */
  vector<OthelloCPCell> head{};         //!< subscript: hashValue, value: keyIndex
  vector<OthelloCPCell> nextAtA{};         //!< subscript: keyIndex, value: keyIndex
  vector<OthelloCPCell> nextAtB{};         //! h2(keys[i]) = h2(keys[next2[i]]);
  
  DisjointSet connectivityForest;                     //!< store the hash values that are connected by key edges
  
  /// update the disjoint set and the connected forest so that
  /// include all the old keys and the newly inserted key
  /// \note this method won't change the node value
  inline void addEdge(uint32_t key, uint32_t ha, uint32_t hb) {
    nextAtA[key] = head[ha];
    head[ha] = {key, hb};
    nextAtB[key] = head[hb];
    head[hb] = {key, ha};
    
    connectivityForest.merge(ha, hb);
  }
  
  #ifdef FULL_DEBUG
  
  void checkLoop(int ha, int hb, int keyId) {
    const K &k = keys[keyId];
    vector<pair<uint32_t, uint32_t>> stack;
    stack.emplace_back(uint32_t(keyId), ha);
    checkLoop(hb, keyId, stack);
  }
  
  void checkLoop(int loopEnd, int excludeId, vector<pair<uint32_t, uint32_t>> &stack) {
    uint32_t prev = stack.back().first;
    uint32_t nid = stack.back().second;
    
    bool isAtoB = nid < ma;
    const vector<OthelloCPCell> &nextKeyOfThisKey = isAtoB ? nextAtA : nextAtB;
    
    for (OthelloCPCell cell = head[nid]; cell.keyId != uint32_t(-1); cell = nextKeyOfThisKey[cell.keyId]) {
      if (cell.keyId == prev || cell.keyId == excludeId) continue;
      
      uint32_t nextNode = cell.nodeId;
      
      stack.emplace_back(uint32_t(cell.keyId), nextNode);
      if (nextNode == loopEnd) {
        // cout << "Loop found! " << endl;
        
        for (pair<uint32_t, uint32_t> &pair: stack) {
          uint32_t keyId = pair.first;
          const K &k = keys[keyId];
          
          uint32_t ha, hb;
          getIndices(k, ha, hb);
          
          // cout << k << ": (" << ha << ", " << hb << ")" << endl;
        }
        
        return;
      }
      
      checkLoop(loopEnd, excludeId, stack);
      stack.pop_back();
    }
  }
  
  #endif
  
  /// test if this hash pair is acyclic, and build:
  /// the connected forest and the disjoint set of connected relation
  /// the disjoint set will be only useful to determine the root of a connected component
  ///
  /// Assume: all build related memory are cleared before
  /// Side effect: the disjoint set and the connected forest are changed
  bool testHash() {
    uint32_t ha, hb;
    // cout << "********\ntesting hash" << endl;
    for (int i = 0; i < keyCnt; i++) {
      const K &k = keys[i];
      uint32_t ha, hb;
      getIndices(k, ha, hb);
      
      // cout << i << "th key: " << keys[i] << ", ha: " << ha << ", hb: " << hb << endl;
      
      // two indices are in the same disjoint set, which means the current key will incur a circle.
      if (connectivityForest.sameSet(ha, hb)) {
        //printf("Conflict key %d: %llx\n", i, *(unsigned long long*) &(keys[i]));
        
        #ifdef FULL_DEBUG
        checkLoop(ha, hb, i);
        #endif
        
        return false;
      }
      addEdge(i, ha, hb);
    }
    return true;
  }
  
  /// Fill the values of a connected tree starting at the root node and avoid searching keyId
  /// Assume:
  /// 1. the value of root is properly set before the function call
  /// 2. the values are in the value array
  /// 3. the root is always from array A
  /// Side effect: all node in this tree is set and if updateToFilled
  template<bool fillValue, bool fillIndex, bool keepDigest = false>
  void fillTreeDFS(uint32_t root) {
    assert(root < ma);
    
    stack<pair<uint32_t, uint32_t>> stack;  // previous key id, this node
    stack.push(make_pair(uint32_t(-1), root));
    
    do {
      Counter::count("Othello", "fillTreeDFS step");
      uint32_t prev = stack.top().first;
      uint32_t nid = stack.top().second;
      stack.pop();
      
      bool isAtoB = nid < ma;
      
      // // find all the opposite side node to be filled
      // search all the edges of this node, to fill and enqueue the opposite side, and record the fill
      
      const vector<OthelloCPCell> &nextKeyOfThisKey = isAtoB ? nextAtA : nextAtB;
      
      for (OthelloCPCell cell = head[nid]; cell.keyId != uint32_t(-1); cell = nextKeyOfThisKey[cell.keyId]) {
        // now the opposite side node needs to be filled
        // fill and enqueue all next element of it
        if (cell.keyId == prev) continue;
        
        uint32_t nextNode = cell.nodeId;
        
        fillSingle<fillValue, fillIndex, keepDigest>(cell.keyId, nextNode, nid);
        
        stack.push(make_pair(uint32_t(cell.keyId), nextNode));
      }
    } while (!stack.empty());
  }
  
  template<bool fillValue, bool fillIndex, bool keepDigest = false>
  inline void fillSingle(uint32_t keyId, uint32_t nodeToFill, uint32_t oppositeNode) {
    if (fillValue && maintainingDP) {
      uint64_t valueToFill;
      if (keepDigest) {
        uint64_t v = values[keyId];
        valueToFill = v ^ memValueGet(oppositeNode);
        memValueSet(nodeToFill, valueToFill);
      } else {
        if (DL) {
          uint64_t digest = hd(keys[keyId]) & DEMASK;
          uint64_t vd = (values[keyId] << DL) | digest;
          valueToFill = (vd ^ memGet(oppositeNode)) | 1ULL;
        } else {
          uint64_t v = values[keyId];
          valueToFill = v ^ memGet(oppositeNode);
        }
        
        memSet(nodeToFill, valueToFill);
      }
    }
    
    if (fillIndex) {
      uint32_t indexToFill = keyId ^indMem[oppositeNode];
      indMem[nodeToFill] = indexToFill;
    }
  }
  
  template<bool fillValue, bool fillIndex, bool keepDigest = false>
  /// fix the value and index at single node by xoring x
  /// \param x the xor'ed number
  inline void fixSingle(uint32_t nodeToFix, uint64_t x, uint32_t ix) {
    if (fillValue && maintainDP) {
      if (keepDigest) {
        uint64_t valueToFill = x ^memValueGet(nodeToFix);
        memValueSet(nodeToFix, valueToFill);
      } else {
        uint64_t valueToFill = x ^memGet(nodeToFix);
        // if(x)
        // {
        //   cout<<"valueToFill: "<<valueToFill<<" memGet(nodeToFix): "<<memGet(nodeToFix)<<" nodeToFix: "<<nodeToFix<<endl;
        // }
        // else
        // {
        //   // cout<<"valueToFill: "<<valueToFill<<" memGet(nodeToFix): "<<memGet(nodeToFix)<<" nodeToFix: "<<nodeToFix<<endl;
        // }
        
        memSet(nodeToFix, valueToFill);
      }
    }
    
    if (fillIndex) {
      uint32_t indexToFill = ix ^indMem[nodeToFix];
      indMem[nodeToFix] = indexToFill;
    }

  }
  
  /// Fix the values of a connected tree starting at the root node and avoid searching keyId
  /// Assume:
  /// 1. the value of root is not properly set before the function call
  /// 2. the values are in the value array
  /// 3. the root is always from array A
  /// Side effect: all node in this tree is set and if updateToFilled
  /// @return  the xor template:
  ///           > 0 starting from the A node
  ///           < 0 from the B node
  ///           == 0 doesn't matter
  template<bool fillValue, bool fillIndex, bool keepDigest = false>
  int64_t fixHalfTreeDFS(uint32_t keyId, uint32_t root, uint32_t hb) {
    assert(root < ma && keyId != uint32_t(-1));
    
    bool swapped = false;
    
    if (fillValue) {
      uint64_t rootVal = memGet(root);
      uint64_t hbVal = memGet(hb);
  
      if (DL && (hbVal & 1) == 0) {
        hbVal |= 1;
        memSet(hb, hbVal);
    
        if ((rootVal & 1) == 1) {
          swap(root, hb);
          swap(rootVal, hbVal);
          swapped = true;
        }
      }
    }
    
    uint64_t x = fillValue ? (keepDigest ? memValueGet(root) : memGet(root)) : 0;
    uint32_t ix = fillIndex ? indMem[root] : 0;
    
    fillSingle<fillValue, fillIndex, keepDigest>(keyId, root, hb);
    
    x = fillValue ? (x ^ (keepDigest ? memValueGet(root) : memGet(root)))
                  : 0;  // the xor'ed value field, including digests. E must be 1, because both ends are 1
    ix = fillIndex ? ix ^ indMem[root] : 0;  // the xor'ed index field
    
    stack<pair<uint32_t, uint32_t>> stack;  // previous key id, this node
    stack.push(make_pair(keyId, root));
    
    do {
      Counter::count("Othello", "fixHalfTreeDFS step");
      uint32_t prev = stack.top().first;
      uint32_t nid = stack.top().second;
      stack.pop();
      
      bool isAtoB = nid < ma;
      
      // // find all the opposite side node to be filled
      // search all the edges of this node, to fill and enqueue the opposite side, and record the fill
      const vector<OthelloCPCell> &nextKeyOfThisKey = isAtoB ? nextAtA : nextAtB;
      
      for (OthelloCPCell cell = head[nid]; cell.keyId != uint32_t(-1); cell = nextKeyOfThisKey[cell.keyId]) {
        // now the opposite side node needs to be filled
        // fill and enqueue all next element of it
        if (cell.keyId == prev) continue;
        
        uint32_t nextNode = cell.nodeId;
        // cout<<"nextNode "<<nextNode<<endl;
        
        fixSingle<fillValue, fillIndex, keepDigest>(nextNode, x, ix);
        
        stack.push(make_pair(uint32_t(cell.keyId), nextNode));
      }
    } while (!stack.empty());
    
    #ifdef FULL_DEBUG
    assert(checkIntegrity());
    #endif
    
    return swapped ? -x : x;
  }


  template<bool fillValue, bool fillIndex, bool keepDigest = false>
  int64_t fixHalfTreeDFS_on_demand(uint32_t keyId, uint32_t root, uint32_t hb, vector<uint32_t> &flipped_index) {
    assert(root < ma && keyId != uint32_t(-1));
    
    bool swapped = false;
    
    if (fillValue) {
      uint64_t rootVal = memGet(root);
      uint64_t hbVal = memGet(hb);
  
      if (DL && (hbVal & 1) == 0) {
        hbVal |= 1;
        memSet(hb, hbVal);
    
        if ((rootVal & 1) == 1) {
          swap(root, hb);
          swap(rootVal, hbVal);
          swapped = true;
        }
      }
    }
    
    uint64_t x = fillValue ? (keepDigest ? memValueGet(root) : memGet(root)) : 0;
    uint32_t ix = fillIndex ? indMem[root] : 0;
    
    uint64_t pre_root_value = memGet(root);
    fillSingle<fillValue, fillIndex, keepDigest>(keyId, root, hb);
    uint64_t post_root_value = memGet(root);
    // cout<<"pre: "<<pre_root_value<<" post: "<<post_root_value<<endl;
    if(pre_root_value != post_root_value)
    {
      flipped_index.push_back(root);
    }
    
    x = fillValue ? (x ^ (keepDigest ? memValueGet(root) : memGet(root)))
                  : 0;  // the xor'ed value field, including digests. E must be 1, because both ends are 1
    ix = fillIndex ? ix ^ indMem[root] : 0;  // the xor'ed index field
    
    stack<pair<uint32_t, uint32_t>> stack;  // previous key id, this node
    stack.push(make_pair(keyId, root));
    
    do {
      Counter::count("Othello", "fixHalfTreeDFS step");
      uint32_t prev = stack.top().first;
      uint32_t nid = stack.top().second;
      stack.pop();
      
      bool isAtoB = nid < ma;
      
      // // find all the opposite side node to be filled
      // search all the edges of this node, to fill and enqueue the opposite side, and record the fill
      const vector<OthelloCPCell> &nextKeyOfThisKey = isAtoB ? nextAtA : nextAtB;
      
      for (OthelloCPCell cell = head[nid]; cell.keyId != uint32_t(-1); cell = nextKeyOfThisKey[cell.keyId]) {
        // now the opposite side node needs to be filled
        // fill and enqueue all next element of it
        if (cell.keyId == prev) continue;
        
        uint32_t nextNode = cell.nodeId;
        // cout<<"nextNode "<<nextNode<<endl;
        
        fixSingle<fillValue, fillIndex, keepDigest>(nextNode, x, ix);
        if(x)
        {
          flipped_index.push_back(nextNode);
        }

        stack.push(make_pair(uint32_t(cell.keyId), nextNode));
      }
    } while (!stack.empty());
    
    #ifdef FULL_DEBUG
    assert(checkIntegrity());
    #endif
    
    return swapped ? -x : x;
  }
  
  vector<uint32_t> getConnectedComponent(uint32_t skip, uint32_t root) {
    vector<uint32_t> result;
    
    stack<pair<uint32_t, uint32_t>> stack;  // previous key id, this node
    stack.push(make_pair(skip, root));
    
    do {
      uint32_t prev = stack.top().first;
      uint32_t nid = stack.top().second;
      stack.pop();
      
      bool isAtoB = nid < ma;
      
      // // find all the opposite side node to be filled
      // search all the edges of this node, to fill and enqueue the opposite side, and record the fill
      const vector<OthelloCPCell> &nextKeyOfThisKey = isAtoB ? nextAtA : nextAtB;
      
      for (OthelloCPCell cell = head[nid]; cell.keyId != uint32_t(-1); cell = nextKeyOfThisKey[cell.keyId]) {
        // now the opposite side node needs to be filled
        // fill and enqueue all next element of it
        if (cell.keyId == prev) continue;
        
        uint32_t nextNode = cell.nodeId;
        result.push_back(nextNode);
        
        stack.push(make_pair(uint32_t(cell.keyId), nextNode));
      }
    } while (!stack.empty());
    
    return result;
  }
  
  /// test the two nodes are connected or not
  /// Assume the Othello is properly built
  /// \note cannot use disjoint set if because disjoint set cannot maintain valid after key deletion. So a traverse is performed
  /// \param ha0
  /// \param hb0
  /// \return true if connected
  bool isConnectedDFS(uint32_t ha0, uint32_t hb0) {
    if (maintainDisjointSet) return connectivityForest.representative(ha0) == connectivityForest.representative(hb0);
    
    if (ha0 == hb0) return true;
    
    stack<pair<uint32_t, uint32_t>> stack;  // previous key id, this node
    stack.push(make_pair(uint32_t(-1), ha0));
    
    do {
      uint32_t prev = stack.top().first;
      uint32_t nid = stack.top().second;
      stack.pop();
      
      bool isAtoB = nid < ma;
      
      const vector<OthelloCPCell> &nextKeyOfThisKey = isAtoB ? nextAtA : nextAtB;
      
      for (OthelloCPCell cell = head[nid]; cell.keyId != uint32_t(-1); cell = nextKeyOfThisKey[cell.keyId]) {
        // now the opposite side node needs to be filled
        // fill and enqueue all next element of it
        if (cell.keyId == prev) continue;
        
        uint32_t nextNode = cell.nodeId;
        
        if (nextNode == hb0)
          return true;
        
        stack.push(make_pair(uint32_t(cell.keyId), nextNode));
      }
    } while (!stack.empty());
    
    return false;
  }
  
  /// Ensure the disjoint set is properly maintained after the construction.
  /// the workflow is: mark the representatives of all connected nodes as root
  /// \param node
  void connectBFS(uint32_t root) {
    
    
    stack<pair<uint32_t, uint32_t>> stack;  // previous key id, this node
    connectivityForest.__set(root, root);
    stack.push(make_pair(uint32_t(-1), root));
    
    if (head[root].keyId == uint32_t(-1)) {
      if (maintainingDP) memSet(root, memGet(root) & (uint64_t(-1) << 1));
      return;
    }
    
    

    do {
      uint32_t prev = stack.top().first;
      uint32_t nid = stack.top().second;
      stack.pop();
      
      bool isAtoB = nid < ma;
      
      const vector<OthelloCPCell> &nextKeyOfThisKey = isAtoB ? nextAtA : nextAtB;
      
      for (OthelloCPCell cell = head[nid]; cell.keyId != uint32_t(-1); cell = nextKeyOfThisKey[cell.keyId]) {
        // now the opposite side node needs to be filled
        // fill and enqueue all next element of it
        if (cell.keyId == prev) continue;
        
        uint32_t nextNode = cell.nodeId;
        
        connectivityForest.__set(nextNode, root);
        
        stack.push(make_pair(uint32_t(cell.keyId), nextNode));
      }
    } while (!stack.empty());

  }
  
  void connectBFS_on_demand(uint32_t root, vector<uint32_t> &flipped_index) {
    
    
    stack<pair<uint32_t, uint32_t>> stack;  // previous key id, this node
    connectivityForest.__set(root, root);
    stack.push(make_pair(uint32_t(-1), root));
    
    if (head[root].keyId == uint32_t(-1)) {
      uint32_t pre_root_value = memGet(root);
      if (maintainingDP) memSet(root, memGet(root) & (uint64_t(-1) << 1));
      uint32_t post_root_value = memGet(root);
      if(pre_root_value != post_root_value)
        flipped_index.push_back(root);
      return;
    }
    
    

    do {
      uint32_t prev = stack.top().first;
      uint32_t nid = stack.top().second;
      stack.pop();
      
      bool isAtoB = nid < ma;
      
      const vector<OthelloCPCell> &nextKeyOfThisKey = isAtoB ? nextAtA : nextAtB;
      
      for (OthelloCPCell cell = head[nid]; cell.keyId != uint32_t(-1); cell = nextKeyOfThisKey[cell.keyId]) {
        // now the opposite side node needs to be filled
        // fill and enqueue all next element of it
        if (cell.keyId == prev) continue;
        
        uint32_t nextNode = cell.nodeId;
        
        connectivityForest.__set(nextNode, root);
        
        stack.push(make_pair(uint32_t(cell.keyId), nextNode));
      }
    } while (!stack.empty());

  }

  /// Fill *Othello* so that the query returns values as defined
  ///
  /// Assume: edges and disjoint set are properly set up.
  /// Side effect: all values are properly set
  template<bool keepDigest = false>
  void fillValue() {
    for (uint32_t i = 0; i < ma + mb; i++) {
      if (connectivityForest.isRoot(i)) {  // we can only fix one end's value in a cc of keys, then fix the roots'
        if ((DL || randomized) && maintainingDP) {
          memSet(i, randomized ? randVal() | 1 : 1);
        }
        
        fillTreeDFS<true, true, keepDigest>(i);
      }
    }
  }
  
  inline void fillOnlyValue() {
    fillValue<true>();
  }
  
  /// Begin a new build
  /// Side effect: 1) discard all memory except keys and values. 2) build fail, or
  /// all the values and disjoint set are properly set
  bool tryBuild() {
    resetBuildState();
    
    if (keyCnt == 0) {
      return true;
    }
    
    #ifndef NDEBUG
    Clocker rebuild("rebuild");
    #else
    // cout << "rebuild" << endl;
    #endif
    bool succ = testHash();
    if (succ) {
      fillValue<false>();
    }
    
    return succ;
  }
  
  /// try really hard to build, until success or tryCount >= MAX_REHASH
  ///
  /// Side effect: 1) discard all memory except keys and values. 2) build fail, or
  /// all the values and disjoint set are properly set
  bool build() {
    int tryCount = 0;
    
    bool built = false;
    do {
      hab.setSeed((uint64_t(InputBase::rand()) << 32) | InputBase::rand());
      tryCount++;
      if (tryCount > 20 && !(tryCount & (tryCount - 1))) {
        // cout << "Another try: " << tryCount << " " << human(keyCnt) << " Keys, ma/mb = " << human(ma) << "/"
        //      << human(mb)    //
        //      << " keyT" << sizeof(K) * 8 << "b  valueT" << sizeof(V) * 8 << "b"     //
        //      << " Lvd=" << (int) VDL << endl;
      }
      built = tryBuild();
    } while ((!built) && (tryCount < MAX_REHASH));
    
    //printf("%08x %08x\n", Ha.s, Hb.s);
    if (built) {
      if (tryCount > 20) {
        // cout << "Succ " << human(keyCnt) << " Keys, ma/mb = " << human(ma) << "/" << human(mb)    //
        //      << " keyT" << sizeof(K) * 8 << "b  valueT" << sizeof(V) * 8 << "b"     //
        //      << " Lvd=" << (int) VDL << " After " << tryCount << "tries" << endl;
      }
    } else {
      // cout << "rebuild fail! " << endl;
      throw exception();
    }
    
    #ifdef FULL_DEBUG
    assert(checkIntegrity());
    #endif
    
    return built;
  }

public:
  /// \param k
  /// \return the index of k in the array of keys
  inline uint32_t queryIndex(const K &k) const {
    uint32_t ha, hb;
    getIndices(k, ha, hb);
    uint32_t aa = indMem[ha];
    uint32_t bb = indMem[hb];

    return aa ^ bb;
  }
  
  inline uint32_t queryIndex(const K &&k) const {
    throw runtime_error("cannot use move semantic");
  }
  
  inline bool isEmpty(int index) {
    return memGet(index) & 1;
  }
  
  /// Insert a key-value pair
  /// \param kv
  /// \return
  ///         cyclic add: 1 << VDL
  ///         acyclic add: the xor template:
  ///                                > 0 starting from the A node
  ///                                < 0 from the B node
  ///                                == 0 doesn't matter
  template<bool DoNotRebuild = false>
  inline int64_t insert(pair<K, V> &&kv) {
    // cout<<kv.first<<endl;
    assert(!isMember(kv.first));
    int lastIndex = keyCnt;
    
    if (keyCnt + 1 >= keys.size() || keyCnt >= mb)
      resizeKey(keyCnt + 1);
    keyCnt++;
    
    this->keys[lastIndex] = kv.first;
    this->values[lastIndex] = kv.second;
    
    uint32_t ha, hb;
    getIndices(kv.first, ha, hb);
    
    int64_t result = 1 << VDL;
    if (isConnectedDFS(ha, hb)) {
      if (DoNotRebuild) {
        keyCnt -= 1;
        throw runtime_error("Do not allow rebuild");
      }
      
      #ifndef NDEBUG
      Clocker rebuild("Othello cyclic add");
      #endif
      if (!build()) {
        keyCnt -= 1;
        throw runtime_error("Rebuild failed");
      }
    } else {  // acyclic, just add
      addEdge(lastIndex, ha, hb);
      result = fixHalfTreeDFS<maintainDP, true>(keyCnt - 1, ha, hb);
    }
    
    return result;
  }


/*insert a key with small table change
copy of insert() function, but returns a different value
the function returns whether a rebuilding is necessary: 0: rebulid; 1: no rebuild*/
  inline bool insert_on_demand(pair<K, V> kv, vector<uint32_t> & flipped_index) {
    // cout<<"insert on insert_on_demand"<<endl;
    // cout<<isMember(kv.first)<<" " << kv.first<<endl;
    
    assert(!isMember(kv.first));
    int lastIndex = keyCnt;
    keyCnt++;
    
    this->keys[lastIndex] = kv.first;
    this->values[lastIndex] = kv.second;
    
    uint32_t ha, hb;
    getIndices(kv.first, ha, hb);
    
    int64_t result = 1 << VDL;
    if (isConnectedDFS(ha, hb)) {
       return 0;
    } 
    else {  // acyclic, just add
      // cout<<"acyclic, just add"<<endl;
      // vector<uint64_t> mem_copy = mem;
      addEdge(lastIndex, ha, hb);
      result = fixHalfTreeDFS_on_demand<maintainDP, true>(keyCnt - 1, ha, hb, flipped_index);
      return 1;
    }
    
   
  }


/*flip the value of a stored key*/
    inline void valueFlip(pair<K, V> kv)
    {
      updateValueAt(queryIndex(kv.first), kv.second);
    }

    inline void valueFlip_on_demand(pair<K, V> kv, vector<uint32_t> &flipped_index)
    {
      updateValueAt_on_demand(queryIndex(kv.first), kv.second, flipped_index);
    }


  

  /// remove one key with the particular index keyId.
  /// \param uint32_t keyId.
  /// \note after this option, the number of keys, keyCnt decrease by 1.
  /// The key currently stored in keys[keyId] will be replaced by the last key of keys.
  /// \note remember to adjust the values array if necessary.
  inline void eraseAt(uint32_t keyId) {
    if (keyId >= keyCnt) throw exception();
    const K &k = keys[keyId];
    erase(k, keyId);
  }
  
  inline int64_t updateMapping(const K &k, V &val) {
    return updateValueAt(queryIndex(k), val);
  }
  
  inline int64_t updateMapping(const K &&k, V &&val) {
    return updateValueAt(queryIndex(k), val);
  }
  
  inline int64_t updateValueAt(uint32_t keyId, V val) {
    if (keyId >= keyCnt) throw exception();
    
    values[keyId] = val;
    
    if (maintainDP) {
      const K &k = keys[keyId];
      uint32_t ha, hb;
      getIndices(k, ha, hb);
      return fixHalfTreeDFS<true, false, true>(keyId, ha, hb);
    }
    
    return 0;
  }

  inline int64_t updateValueAt_on_demand(uint32_t keyId, V val, vector<uint32_t> &flipped_index) {
    if (keyId >= keyCnt) throw exception();
    
    values[keyId] = val;
    
    if (maintainDP) {
      const K &k = keys[keyId];
      uint32_t ha, hb;
      getIndices(k, ha, hb);
      return fixHalfTreeDFS_on_demand<true, false, true>(keyId, ha, hb, flipped_index);
    }
    
    return 0;
  }
  
  //****************************************
  //*********AS A SET
  //****************************************
public:
  inline const vector<K> &getKeys() const {
    return keys;
  }
  
  inline const vector<V> &getValues() const {
    return values;
  }
  
  inline vector<V> &getValues() {
    return values;
  }
  
  inline const vector<uint32_t> &getIndexMemory() const {
    return indMem;
  }
  
  inline uint32_t size() const {
    return keyCnt;
  }
  
  inline bool isMember(const K &x) const {
    uint32_t index = queryIndex(x);
    // cout<<(index < keyCnt)<<" "<<(keys[index] == x)<<endl;
    return (index < keyCnt && keys[index] == x);
  }
  
  inline void erase_on_demand(const K &k, vector<uint32_t> &flipped_index, uint32_t keyId = -1)
  {

    
    if (keyId == uint32_t(-1)) {
      keyId = queryIndex(k);
      if (keyId >= keyCnt || !(keys[keyId] == k)) return;
    }
    
    uint32_t ha, hb;
    getIndices(k, ha, hb);
    keyCnt--;
    
    // Delete the edge of keyId. By maintaining the linked lists on nodes ha and hb.
    OthelloCPCell headA = head[ha];
    if (headA.keyId == keyId) {
      head[ha] = nextAtA[keyId];
    } else {
      int t = headA.keyId;
      while (nextAtA[t].keyId != keyId)
        t = nextAtA[t].keyId;
      nextAtA[t] = nextAtA[keyId];
    }
    OthelloCPCell headB = head[hb];
    if (headB.keyId == keyId) {
      head[hb] = nextAtB[keyId];
    } else {
      int t = headB.keyId;
      while (nextAtB[t].keyId != keyId)
        t = nextAtB[t].keyId;
      nextAtB[t] = nextAtB[keyId];
    }
    

    // move the last to override current key-value
    if (keyId != keyCnt) {
      const K &lastKey = keys[keyCnt];
      keys[keyId] = lastKey;
      values[keyId] = values[keyCnt];
      
      uint32_t hal, hbl;
      getIndices(lastKey, hal, hbl);
      
      // repair the broken linked list because of key movement
      nextAtA[keyId] = nextAtA[keyCnt];
      if (head[hal].keyId == keyCnt) {
        head[hal] = {keyId, hbl};
      } else {
        int t = head[hal].keyId;
      //   cout<<"erase!!"<<endl;
      //   cout<<nextAtA.size()<<endl;
      // cout<<nextAtA[t].keyId<<" "<<t<<endl;
        while (nextAtA[t].keyId != keyCnt)
          t = nextAtA[t].keyId;
        nextAtA[t] = {keyId, hbl};
      }

      nextAtB[keyId] = nextAtB[keyCnt];
      if (head[hbl].keyId == keyCnt) {
        head[hbl] = {keyId, hal};
      } else {
        int t = head[hbl].keyId;
        while (nextAtB[t].keyId != keyCnt)
          t = nextAtB[t].keyId;
        nextAtB[t] = {keyId, hal};
      }


      // update the mapped index
      fixHalfTreeDFS_on_demand<false, true, true>(keyId, hal, hbl, flipped_index);

    }
    

    if (maintainDisjointSet) {
      // repair the disjoint set

      connectBFS_on_demand(ha,flipped_index);
      connectBFS_on_demand(hb,flipped_index);
      
    }

    
    #ifdef FULL_DEBUG
    assert(checkIntegrity());
    #endif
  }

  inline void erase(const K &k, uint32_t keyId = -1) {
    if (keyId == uint32_t(-1)) {
      keyId = queryIndex(k);
      if (keyId >= keyCnt || !(keys[keyId] == k)) return;
    }
    
    uint32_t ha, hb;
    getIndices(k, ha, hb);
    keyCnt--;
    
    // Delete the edge of keyId. By maintaining the linked lists on nodes ha and hb.
    OthelloCPCell headA = head[ha];
    if (headA.keyId == keyId) {
      head[ha] = nextAtA[keyId];
    } else {
      int t = headA.keyId;
      while (nextAtA[t].keyId != keyId)
        t = nextAtA[t].keyId;
      nextAtA[t] = nextAtA[keyId];
    }
    OthelloCPCell headB = head[hb];
    if (headB.keyId == keyId) {
      head[hb] = nextAtB[keyId];
    } else {
      int t = headB.keyId;
      while (nextAtB[t].keyId != keyId)
        t = nextAtB[t].keyId;
      nextAtB[t] = nextAtB[keyId];
    }
    

    // move the last to override current key-value
    if (keyId != keyCnt) {
      const K &lastKey = keys[keyCnt];
      keys[keyId] = lastKey;
      values[keyId] = values[keyCnt];
      
      uint32_t hal, hbl;
      getIndices(lastKey, hal, hbl);
      
      // repair the broken linked list because of key movement
      nextAtA[keyId] = nextAtA[keyCnt];
      if (head[hal].keyId == keyCnt) {
        head[hal] = {keyId, hbl};
      } else {
        int t = head[hal].keyId;
      //   cout<<"erase!!"<<endl;
      //   cout<<nextAtA.size()<<endl;
      // cout<<nextAtA[t].keyId<<" "<<t<<endl;
        while (nextAtA[t].keyId != keyCnt)
          t = nextAtA[t].keyId;
        nextAtA[t] = {keyId, hbl};
      }

      nextAtB[keyId] = nextAtB[keyCnt];
      if (head[hbl].keyId == keyCnt) {
        head[hbl] = {keyId, hal};
      } else {
        int t = head[hbl].keyId;
        while (nextAtB[t].keyId != keyCnt)
          t = nextAtB[t].keyId;
        nextAtB[t] = {keyId, hal};
      }

      // update the mapped index
      fixHalfTreeDFS<false, true, true>(keyId, hal, hbl);
    }
    
    if (maintainDisjointSet) {
      // repair the disjoint set
      connectBFS(ha);
      connectBFS(hb);
    }
    
    #ifdef FULL_DEBUG
    assert(checkIntegrity());
    #endif
  }
  
  bool checkIntegrity() const {
    for (int i = 0; i < size(); ++i) {
      V q;
      assert(query(keys[i], q));
      q &= VMASK;
      V e = values[i] & VMASK;
      assert(q == e);
      assert(queryIndex(keys[i]) == i);
    }
    
    return true;
  }
  
  //****************************************
  //*********As a randomizer
  //****************************************
public:
  uint64_t reportDataPlaneMemUsage() const {
    uint64_t size = mem.size() * sizeof(V);
    return size;
  }
  
  // return the mapped count of a value
  vector<uint32_t> getCnt() const {
    vector<uint32_t> cnt(1ULL << L);
    
    for (int i = 0; i < ma; i++) {
      for (int j = ma; j < ma + mb; j++) {
        cnt[memGet(i) ^ memGet(j)]++;
      }
    }
    return cnt;
  }
  
  void outputMappedValues(ofstream &fout) const {
    bool partial = (uint64_t) ma * (uint64_t) mb > (1UL << 22);
    
    if (partial) {
      for (int i = 0; i < (1 << 22); i++) {
        fout << uint32_t(memGet(InputBase::rand() % (ma - 1)) ^ memGet(ma + InputBase::rand() % (mb - 1))) << endl;
      }
    } else {
      for (int i = 0; i < ma; i++) {
        for (int j = ma; j < ma + mb; ++j) {
          fout << uint32_t(memGet(ma) ^ memGet(j)) << endl;
        }
      }
    }
  }
  
  int getStaticCnt() {
    return ma * mb;
  }
  
  uint64_t getMemoryCost() const {
    return mem.size() * sizeof(mem[0]) + keys.size() * sizeof(keys[0]) + values.size() * sizeof(values[0]) +
           indMem.size() * sizeof(indMem[0]);
  }
};


template<class K, class V, uint8_t L = sizeof(V) * 8>
class OthelloMap : public ControlPlaneOthello<K, V, L, false, true> {
public:
  explicit OthelloMap(uint32_t keyCapacity = 256) : ControlPlaneOthello<K, V, L, false, true>(keyCapacity) {}
};

template<class K>
class OthelloSet : public ControlPlaneOthello<K, bool, 0, false, true> {
public:
  explicit OthelloSet(uint32_t keyCapacity = 256) : ControlPlaneOthello<K, bool, 0, false, true>(keyCapacity) {}
  
  inline bool insert(const K &k) {
    return ControlPlaneOthello<K, bool, 0, false, true>::insert(make_pair(k, true));
  }
};
