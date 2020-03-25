#ifndef CUCKOO_H
#define CUCKOO_H
#include <assert.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <iostream>
#include "../utils/hashutil.h"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <random>
using namespace std;

#define memcle(a) memset(a, 0, sizeof(a))
#define sqr(a) ((a) * (a))
#define debug(a) cerr << #a << " = " << a << ' '
#define deln(a) cerr << #a << " = " << a << endl
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ROUNDDOWN(a, b) ((a) - ((a) % (b)))
#define ROUNDUP(a, b) ROUNDDOWN((a) + (b - 1), b)

const int long_seg = 262144;


inline unsigned long lower_power_of_two(unsigned long x)
{
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x - (x >> 1);
}

inline int find_highest_bit(int v)
{
// tricks of bit 
// from http://graphics.stanford.edu/~seander/bithacks.html
    static const int MultiplyDeBruijnBitPosition[32] = 
    {
      0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
      8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
    };

    v |= v >> 1; // first round down to one less than a power of 2 
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    int r = MultiplyDeBruijnBitPosition[(uint32_t)(v * 0x07C4ACDDU) >> 27];
    return r;
}


inline int upperpower2(int x)
{
    int ret = 1;
    for (; ret * 2 < x; ) ret <<= 1;
    return ret;
}

// solve equation : 1 + x(logc - logx + 1) - c = 0
inline double F_d(double x, double c)
{
  return log(c) - log(x);
}

inline double F(double x, double c)
{
  return 1 + x * (log(c) - log(x) + 1) - c;
}

inline double solve_equation(double c) 
{
  double x = c + 0.1;
  while (abs(F(x, c)) > 0.01)
    x -= F(x, c) / F_d(x, c);
  return x;
}

inline double balls_in_bins_max_load(double balls, double bins)
{
    double m = balls;
    double n = bins;
    double c = m / (n * log(n));

    if (c < 5)
    {
      printf("c = %.5f\n", c);
      double dc = solve_equation(c);
      double ret = (dc - 1 + 2) * log(n);
      return ret;
    }

    double ret = (m / n) + 1.5 * sqrt(2 * m / n * log(n));
    return ret;
}

inline int proper_alt_range(int M, int i, int *len)
{
    double b = 4; // slots per bucket
    double lf = 0.95; // target load factor
    int alt_range = 8;
    for (; alt_range < M;)
    {
        double f = (4 - i) * 0.25;
      if (balls_in_bins_max_load(f * b * lf * M, M * 1.0 / alt_range) < 0.97 * b * alt_range)
        return alt_range;
      alt_range <<= 1;
    }
    return -1;
}


template <typename fp_t>
class Filter
{
    public:
        long long n; // number of buckets
        int m; // number of slots per bucket
        uint64_t memory_consumption;
        virtual void init(int _n, int _m, int _max_kick_steps, int fp_length) = 0;
        virtual void clear() = 0;
        virtual int insert(uint64_t ele) = 0;
        virtual bool lookup(uint64_t ele) = 0;
        virtual bool del(uint64_t ele) = 0;
        virtual double get_load_factor() {return 0;}
        virtual double get_full_bucket_factor() {return 0;}
        virtual void debug_test() {}

        // hash to range [0, n - 1]
        uint64_t position_hash(long long ele)
        {
            return (ele % n + n) % n;
        }
};

template <typename fp_t>
class SemiSortCuckooFilter : public Filter<fp_t>
{
public: 
    int max_2_power;
    int big_seg;
    int len[4];
    bool isSmall;
    int fp_len;
    default_random_engine e;
    bool debug_flag = false;
    bool balance = true;
    uint32_t *T;
    uint32_t encode_table[1 << 16];
    uint32_t decode_table[1 << 16];
    int filled_cell;
    int full_bucket;
    int max_kick_steps;

    //interface for semi-sorted bucket
    void make_balance();
    

    virtual int alternate(int pos, fp_t fp) = 0; // get alternate position

    pair<int, int> position_pair(uint64_t ele)
    {
        ele = HashUtil::MurmurHash64(ele ^ 0x12891927);

        fp_t fp = fingerprint(ele);

        int pos1 = this -> position_hash(ele);

        int pos2 = alternate(pos1, fp);

        pair<int, int> pos_pair (pos1, pos2);
        return pos_pair;
    }

    ~SemiSortCuckooFilter() 
    {
        free(T); 
    }

    void init_with_params(uint64_t _mc, default_random_engine &_e, long long _n, int _m, int _filled_cell, int _max_kick_steps,
    int _max_2_power, int _big_seg, bool _isSmall, int *_len, int _fp_len, int _T_len, uint32_t *_T)
    {
        this->e = _e;
        this->memory_consumption = _mc;
        this->n = _n;
        this->m = _m;
        this->filled_cell = _filled_cell;
        this -> max_kick_steps = _max_kick_steps;
        this->max_2_power = _max_2_power;
        this->big_seg = _big_seg;
        this->isSmall = _isSmall;
        for(int i=0; i<4; i++)
            this->len[i] = _len[i];
        this->fp_len = _fp_len;
        this -> T = (uint32_t *) calloc(this -> memory_consumption, sizeof(char));
        for(int i=0; i<_T_len; i++)
        {
            this -> T[i] = _T[i];
        }
        if (this->m == 4) {
            int index = 0;
            for (int i = 0; i < 16; i++)
                for (int j = 0; j < ((i == 0) ? 1 : i + 1); j++)
                    for (int k = 0; k < ((j == 0) ? 1 : j + 1); k++)
                        for (int l = 0; l < ((k == 0) ? 1 : k + 1); l++) {
                            int plain_bit = (i << 12) + (j << 8) + (k << 4) + l;
                            encode_table[plain_bit] = index;
                            decode_table[index] = plain_bit;
                            ++index;
                        }
        }
    }
    
    void operator = (SemiSortCuckooFilter const &obj)
    {
        this->e = obj.e;
        this->memory_consumption = obj.memory_consumption;
        this->n = obj.n;
        this->m = obj.m;
        this->filled_cell = obj.filled_cell;
        this -> max_kick_steps = obj.max_kick_steps;
        this->max_2_power = obj.max_2_power;
        this->big_seg = obj.big_seg;
        this->isSmall = obj.isSmall;
        copy(begin(obj.len), end(obj.len), begin(this->len));
        this->fp_len = obj.fp_len;
        copy(begin(obj.encode_table), end(obj.encode_table), begin(this->encode_table));
        copy(begin(obj.decode_table), end(obj.decode_table), begin(this->decode_table));
        this -> T = (uint32_t *) calloc(this -> memory_consumption, sizeof(char));
        int T_len = this -> memory_consumption * sizeof(char) /  sizeof(uint32_t);
        for(int i=0; i<T_len; i++)
        {
            this -> T[i] = obj.T[i];
        }
    }

    virtual void init(int _n, int _m, int _step, int fp_length) {
        fp_len = fp_length;
        _n += _n & 1;
        if (_n < 1315800)  //size < 5000000
            isSmall = true;
        else
            isSmall = false;
        if (!isSmall) {
            big_seg = 0;
            //big_seg = upperpower2(_n / 100);
            big_seg = max(big_seg, proper_alt_range(_n, 0, len));
            _n = ROUNDUP(_n, big_seg);
            // cout<<"_n is "<<_n<<" big_seg is "<<big_seg<<endl;

            big_seg--;
            len[0] = big_seg;
            for (int i = 1; i < 4; i++)
                len[i] = proper_alt_range(_n, i, len) - 1;

            len[0] = max(len[0], 1024);
            len[3] = max(len[3], 31);
        }

        printf("alt setting : ");
        for (int i = 0; i < 4; i++)
            printf("%d, ", len[i]);
        printf("\n");

        this->n = _n;
        this->m = _m;
        this->max_kick_steps = _step;
        this->filled_cell = 0;
        this->full_bucket = 0;

        uint64_t how_many_bit = (uint64_t) this->n * this->m * (fp_len - 1);


        this->memory_consumption = ROUNDUP(how_many_bit + 64, 8) / 8 + 8; // how many bytes !

        max_2_power = 1;
        for (; max_2_power * 2 < _n;)
            max_2_power <<= 1;

        this->T = (uint32_t *) calloc(this->memory_consumption, sizeof(char));

        if (this->m == 4) {
            int index = 0;
            for (int i = 0; i < 16; i++)
                for (int j = 0; j < ((i == 0) ? 1 : i + 1); j++)
                    for (int k = 0; k < ((j == 0) ? 1 : j + 1); k++)
                        for (int l = 0; l < ((k == 0) ? 1 : k + 1); l++) {
                            int plain_bit = (i << 12) + (j << 8) + (k << 4) + l;
                            encode_table[plain_bit] = index;
                            decode_table[index] = plain_bit;
                            ++index;
                        }
        }
        e.seed(1);
    }

    virtual void clear()
    {
        this -> filled_cell = 0;
        //memset(this -> T, 0, sizeof(fp_t) * (this -> n * this -> m));
        memset(this -> T, 0, this -> memory_consumption);
    }

    fp_t fingerprint(uint64_t ele)
    {
        if(fp_len == 0)
        {
            std::cout << "fp_len must be greater than 0\n";
            exit(-1);
        }
        //fp_t h = HashUtil::BobHash32(&ele, 4) % ((1ull << fp_len) -1) + 1;
        fp_t h = HashUtil::MurmurHash64(ele ^ 0x192837319273LL) % ((1ull << fp_len) -1) + 1;
        return h;
    }

    void get_bucket(int pos, fp_t *store)
    {
        // Default : 
        //
        // Little Endian Store
        // Store by uint32_t
        // store[this -> m] = bucket number


        // 1. read the endcoded bits from memory

        int bucket_length = (fp_len - 1) * 4;
        uint64_t start_bit_pos = (uint64_t)pos * bucket_length;
        uint64_t end_bit_pos = start_bit_pos + bucket_length - 1;
        uint64_t result = 0;

        if (ROUNDDOWN(start_bit_pos, 64) == ROUNDDOWN(end_bit_pos, 64))
        {
            uint64_t unit = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64];
            int reading_lower_bound = start_bit_pos & 63;
            int reading_upper_bound = end_bit_pos & 63;

            result = ((uint64_t)unit & ((-1ULL) >> (63 - reading_upper_bound))) >> reading_lower_bound;
        } else
        {
            uint64_t unit1 = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64];
            uint64_t unit2 = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64 + 1];

            int reading_lower_bound = start_bit_pos & 63;
            int reading_upper_bound = end_bit_pos & 63;

            uint64_t t1 = unit1 >> reading_lower_bound;
            uint64_t t2 = (unit2 & ((1ULL << (reading_upper_bound + 1)) - 1)) << (64 - reading_lower_bound);
            result = t1 + t2;
        }

        int decode_result = decode_table[result >> (4 * (fp_len - 4))];

        store[3] = (result & ((1 << (fp_len - 4)) - 1)) + ((decode_result & ((1 << 4) - 1)) << (fp_len - 4));
        store[2] = ((result >> (1 * (fp_len - 4))) & ((1 << (fp_len - 4)) - 1)) + (((decode_result >> 4) & ((1 << 4) - 1)) << (fp_len - 4)); 
        store[1] = ((result >> (2 * (fp_len - 4))) & ((1 << (fp_len - 4)) - 1)) + (((decode_result >> 8) & ((1 << 4) - 1)) << (fp_len - 4));
        store[0] = ((result >> (3 * (fp_len - 4))) & ((1 << (fp_len - 4)) - 1)) + (((decode_result >> 12) & ((1 << 4) - 1)) << (fp_len - 4));

        store[4] = 0;
        store[4] += store[0] != 0;
        store[4] += store[1] != 0;
        store[4] += store[2] != 0;
        store[4] += store[3] != 0;
    }

    inline void sort_pair(fp_t &a, fp_t &b)
    {
        if ((a) < (b)) swap(a, b);
    }

    void set_bucket(int pos, fp_t *store)
    {
        // 0. sort store ! descendant order >>>>>>

        sort_pair(store[0], store[2]);
        sort_pair(store[1], store[3]);
        sort_pair(store[0], store[1]);
        sort_pair(store[2], store[3]);
        sort_pair(store[1], store[2]);

        /*
        for (int i = 0; i < this -> m - 1; i++)
            if (store[i] != 0 && store[i + 1] == store[i])
                printf("same ? ");
        */
        // 1. compute the encode 

        uint64_t high_bit = 0;
        uint64_t low_bit = 0;

        low_bit = (store[3] & ((1 << (fp_len - 4)) - 1)) 
                + ((store[2] & ((1 << (fp_len - 4)) - 1)) << (1 * (fp_len - 4)))
                + (((uint64_t)store[1] & ((1 << (fp_len - 4)) - 1)) << (2 * (fp_len - 4)))
                + (((uint64_t)store[0] & ((1 << (fp_len - 4)) - 1)) << (3 * (fp_len - 4)));

        high_bit = ((store[3] >> (fp_len - 4)) & ((1 << 4) - 1))
                + (((store[2] >> (fp_len - 4)) & ((1 << 4) - 1)) << 4)
                + (((store[1] >> (fp_len - 4)) & ((1 << 4) - 1)) << 8)
                + (((store[0] >> (fp_len - 4)) & ((1 << 4) - 1)) << 12);

        // 2. store into memory
        uint64_t high_encode = encode_table[high_bit];
        uint64_t all_encode = (high_encode << (4 * (fp_len - 4))) + low_bit;

        int bucket_length = (fp_len - 1) * 4;
        uint64_t start_bit_pos = (uint64_t)pos * bucket_length;
        uint64_t end_bit_pos = start_bit_pos + bucket_length - 1;

        if (ROUNDDOWN(start_bit_pos, 64) == ROUNDDOWN(end_bit_pos, 64))
        {
            uint64_t unit = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64];
            int writing_lower_bound = start_bit_pos & 63;
            int writing_upper_bound = end_bit_pos & 63;

            ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64] = (unit & (((1ULL << writing_lower_bound) - 1) + ((-1ULL) - ((-1ULL) >> (63 - writing_upper_bound)))))
                                                + ((all_encode & ((1ULL << (writing_upper_bound - writing_lower_bound + 1)) - 1)) << writing_lower_bound);
        } else
        {
            uint64_t unit1 = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64];
            uint64_t unit2 = ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64 + 1];
            int writing_lower_bound = start_bit_pos & 63;
            int writing_upper_bound = end_bit_pos & 63;
            uint64_t lower_part = all_encode & ((1LL << (64 - writing_lower_bound)) - 1);
            uint64_t higher_part = all_encode >> (64 - writing_lower_bound);
            ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64] = (unit1 & ((1LL << writing_lower_bound) - 1)) 
                                                            + (lower_part << writing_lower_bound);
            ((uint64_t *)T)[ROUNDDOWN(start_bit_pos, 64) / 64 + 1] = ((unit2 >> (writing_upper_bound + 1)) << (writing_upper_bound + 1))
                                                                + higher_part;
        }
    }

    inline int high_bit(fp_t fp)
    {
        return (fp >> (fp_len - 4)) & ((1 << 4) - 1);
    }

    inline int low_bit(fp_t fp)
    {
        return fp & ((1 << (fp_len - 4)) - 1);
    }

    virtual int insert_to_bucket(fp_t *store, fp_t fp)
    {

        // if success return 0
        // if find the same : return 1 + position
        // if full : return 1 + 4

        //get_bucket(pos, store);

        if (store[this -> m] == this -> m) 
            return 1 + 4;
        else 
        {
            store[3] = fp;
            return 0;
        }
    }

    virtual int insert(uint64_t ele)
    {
        return 1;
    }

    virtual int lookup_in_bucket(int pos, fp_t fp)
    {
        // If lookup success return 1
        // If lookup fail and the bucket is full return 2
        // If lookup fail and the bucket is not full return 3

        fp_t store[8];
        get_bucket(pos, store);

        int isFull = 1;
        for (int i = 0; i < this -> m; i++)
        {
            fp_t t = store[i];
            if (t == fp) 
                return 1;
            isFull &= (t != 0);
        }
        return (isFull) ? 2 : 3;
    }

    bool lookup(uint64_t ele)
    {

        // If ele is positive, return true
        // negative -- return false

        ele = HashUtil::MurmurHash64(ele ^ 0x12891927);

        fp_t fp = fingerprint(ele);

        int pos1 = this -> position_hash(ele);

        int ok1 = lookup_in_bucket(pos1, fp);

        
        if (ok1 == 1) return true;
        //if (ok1 == 3) return false;

        int pos2 = alternate(pos1, fp);
        assert(pos1 == alternate(pos2, fp));
        int ok2 = lookup_in_bucket(pos2, fp);

        return ok2 == 1;
    }

    virtual int del_in_bucket(int pos, fp_t fp)
    {
        fp_t store[8];
        get_bucket(pos, store);

        for (int i = 0; i < this -> m; i++)
        {
            fp_t t = store[i];
            if (t == fp) 
            {
                store[i] = 0;
                --this -> filled_cell;
                set_bucket(pos, store);
                return 1;
            }
        }
        return 0;
    }

    virtual bool del(uint64_t ele)
    {

        // If ele is positive, return true
        // negative -- return false

        ele = HashUtil::MurmurHash64(ele ^ 0x12891927);
        fp_t fp = fingerprint(ele);
        int pos1 = this -> position_hash(ele);

        int ok1 = del_in_bucket(pos1, fp);

        if (ok1 == 1) return true;
        //if (ok1 == 3) return false;

        int pos2 = alternate(pos1, fp);
        int ok2 = del_in_bucket(pos2, fp);

        return ok2 == 1;
    }

    double get_load_factor()
    {
        return filled_cell * 1.0 / this -> n / this -> m;
    }

    double get_full_bucket_factor()
    {
        return full_bucket * 1.0 / this -> n;
    }

    void test_bucket()
    {
        for (int i = 0; i < this -> n; i++)
        {
            fp_t store[8];
            store[0] = this->e() % (1 << fp_len);
            store[1] = this->e() % (1 << fp_len);
            store[2] = this->e() % (1 << fp_len);
            store[3] = this->e() % (1 << fp_len);

            for (int j = 0; j < this -> m; j++)
                for (int k = j + 1; k < this -> m; k++)
                    if (store[j] == store[k])
                        store[k] = 0;

            set_bucket(i, store);

            fp_t tmp_store[8];
            get_bucket(i, tmp_store);
            for (int j = 0; j < 4; j++)
            {
                if (tmp_store[j] != store[j])
                    printf("first i = %d, j = %d\n", i, j);
                assert(tmp_store[j] == store[j]);
            }
        }

        for (int i = 0; i < this -> n; i++)
        {
            fp_t store[8];
            store[0] = this->e() % (1 << fp_len);
            store[1] = this->e() % (1 << fp_len);
            store[2] = this->e() % (1 << fp_len);
            store[3] = this->e() % (1 << fp_len);

            for (int j = 0; j < this -> m; j++)
                for (int k = j + 1; k < this -> m; k++)
                    if (store[j] == store[k])
                        store[k] = 0;

            set_bucket(i, store);

            fp_t tmp_store[8];
            get_bucket(i, tmp_store);
            for (int j = 0; j < 4; j++)
            {
                if (tmp_store[j] != store[j])
                    printf("i = %d, j = %d\n", i, j);
                assert(tmp_store[j] == store[j]);
            }
        }
    }
}; 

template <typename fp_t>
class VacuumFilter : public SemiSortCuckooFilter<fp_t>
{
private:
    // get alternate position
    virtual int alternate(int pos, fp_t fp)
    {
        if (this->isSmall) {
            int n = this->n;

            int fp_hash = HashUtil::MurmurHash32(fp);

            //delta : the xor number
            int delta = fp_hash & (this->max_2_power - 1);

            //bias : the rotation bias
            int bias = delta;

            pos = pos + bias;
            if (pos >= this->n)
                pos -= this->n;

            // find the corresponding segment of 'pos'
            // 1. pos ^ n
            // ----- the highest different bit between position and n will be set to 1
            //
            // 2. find the highest bit
            // ----- get the segment number
            //
            // 3. curlen = 1 << highest_bit
            // ----- get the segment length
            int segment_length = 1 << find_highest_bit(pos ^ n);

            // get the alternate position
            // 1. delta & (segment_length - 1)
            // ----- equals to delta % segment_length
            // 2. pos ^ ...
            // ----- xor (delta % segment_length)
            int t = (delta & (segment_length - 1));
            if (t == 0)
                t = 1;
            int ret = pos ^ t;
            // minus bias to avoid aggregation
            ret = ret - bias;
            if (ret < 0)
                ret += this->n;
            return ret;

        } else {
            uint32_t fp_hash = fp * 0x5bd1e995;
            int seg = this->len[fp & 3];
            return pos ^ (fp_hash & seg);
        }
    }

public: 
    int insert(uint64_t ele)
    {
        // If insert success return 0
        // If insert fail return 1

        ele = HashUtil::MurmurHash64(ele ^ 0x12891927);

        fp_t fp = this -> fingerprint(ele);
        int cur1 = this -> position_hash(ele);
        int cur2 = alternate(cur1, fp);

        fp_t store1[8];
        fp_t store2[8];

        this -> get_bucket(cur1, store1);
        this -> get_bucket(cur2, store2);

        if (store1[this -> m] <= store2[this -> m])
        {
            if (this -> insert_to_bucket(store1, fp) == 0) {this -> filled_cell++; this -> set_bucket(cur1, store1); return 0;}
        } else
        {
            if (this -> insert_to_bucket(store2, fp) == 0) {this -> filled_cell++; this -> set_bucket(cur2, store2); return 0;}
        }

        //randomly choose one bucket's elements to kick
        int rk = this->e() % this -> m;

        //get those item
        int cur;
        fp_t *cur_store;

        if (this->e() & 1)
            cur = cur1, cur_store = store1;
        else 
            cur = cur2, cur_store = store2;

        fp_t tmp_fp = cur_store[rk];
        cur_store[rk] = fp;
        this -> set_bucket(cur, cur_store);

        int alt = alternate(cur, tmp_fp);
        
        for (int i = 0; i < this -> max_kick_steps; i++)
        {
            memset(store1, 0, sizeof(store1));
            this -> get_bucket(alt, store1);
            if (store1[this -> m] == this -> m)
            {
                for (int j = 0; j < this -> m; j++)
                {
                    int nex = alternate(alt, store1[j]);
                    this -> get_bucket(nex, store2);
                    if (store2[this -> m] < this -> m)
                    {
                        store2[this -> m - 1] = store1[j];
                        store1[j] = tmp_fp;
                        this -> filled_cell++;
                        this -> set_bucket(nex, store2);
                        this -> set_bucket(alt, store1);
                        return 0;
                    }
                }

                rk = this->e() % this -> m;
                fp = store1[rk];
                store1[rk] = tmp_fp;
                this -> set_bucket(alt, store1);

                tmp_fp = fp;
                alt = alternate(alt, tmp_fp);
            } else
            {
                store1[this -> m - 1] = tmp_fp;
                this -> filled_cell++; 
                this -> set_bucket(alt, store1);
                return 0;
            }
        }
        return 1;
    }

};

#endif
