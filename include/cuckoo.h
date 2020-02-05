#ifndef CUCKOO_H
#define CUCKOO_H
#include <assert.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <iostream>
#include "hashutil.h"
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

unsigned long lower_power_of_two(unsigned long x);

inline int find_highest_bit(int v);

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
        uint64_t position_hash(long long ele); // hash to range [0, n - 1]
        virtual double get_load_factor() {return 0;}
        virtual double get_full_bucket_factor() {return 0;}
        virtual void debug_test() {}
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
        virtual void init(int _n, int _m, int _max_kick_steps, int fp_length);
        void clear();
        virtual int insert(uint64_t ele);
        bool lookup(uint64_t ele);
        virtual bool del(uint64_t ele);
        pair<int, int> position_pair(uint64_t ele);
        double get_load_factor();
        double get_full_bucket_factor();
        bool debug_flag = false;
        bool balance = true;
        uint32_t *T;
        uint32_t encode_table[1 << 16];
        uint32_t decode_table[1 << 16];
	    ~SemiSortCuckooFilter();
        int filled_cell;
        int full_bucket;
        int max_kick_steps;
        void init_with_params(uint64_t _mc, default_random_engine &_e, long long _n, int _m, int _filled_cell, int _max_kick_steps,
            int _max_2_power, int _big_seg, bool _isSmall, int *_len, int _fp_len, int _T_len, uint32_t *_T);
        void operator = (SemiSortCuckooFilter const &obj);
        fp_t fingerprint(uint64_t ele); // 32-bit to 'fp_len'-bit fingerprint

        //interface for semi-sorted bucket
        void get_bucket(int pos, fp_t *store);
        void set_bucket(int pos, fp_t *sotre);
        void test_bucket();
        void make_balance();
        inline int high_bit(fp_t fp);
        inline int low_bit(fp_t fp);
        inline void sort_pair(fp_t &a, fp_t &b);

        virtual int alternate(int pos, fp_t fp) = 0; // get alternate position
        virtual int insert_to_bucket(fp_t *store, fp_t fp); // insert one fingerprint to bucket [pos] 
        virtual int lookup_in_bucket(int pos, fp_t fp); // lookup one fingerprint in  bucket [pos]
        virtual int del_in_bucket(int pos, fp_t fp); // lookup one fingerprint in  bucket [pos]
        
}; 

template <typename fp_t>
class VacuumFilter : public SemiSortCuckooFilter<fp_t>
{
    private:
        // get alternate position
        virtual int alternate(int pos, fp_t fp); 

    public: 
        int insert(uint64_t ele);

};

int upperpower2(int x);

// solve equation : 1 + x(logc - logx + 1) - c = 0
double F_d(double x, double c);
double F(double x, double c);
double solve_equation(double c);
double balls_in_bins_max_load(double balls, double bins);

int proper_alt_range(int M, int i, int *len);



#endif
