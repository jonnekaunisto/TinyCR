#ifndef HELPERS_H
#define HELPERS_H
#include <vector>
#include <stdint.h>
#include <string>
#include <cstring>
#include <math.h>

#define memcle(a) memset(a, 0, sizeof(a))
#define sqr(a) ((a) * (a))
#define debug(a) cerr << #a << " = " << a << ' '
#define deln(a) cerr << #a << " = " << a << endl
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ROUNDDOWN(a, b) ((a) - ((a) % (b)))
#define ROUNDUP(a, b) ROUNDDOWN((a) + (b - 1), b)


const int long_seg = 262144;

uint32_t combine_chars_as_uint(std::vector<uint8_t> &data, uint32_t val);

uint64_t combine_chars_as_uint(std::vector<uint8_t> &data, uint64_t val);

uint32_t combine_chars_as_uint32_t(std::vector<uint8_t> &data, uint32_t begin);

uint64_t combine_chars_as_uint64_t(std::vector<uint8_t> &data, uint32_t begin);

void split_uint_t(uint32_t &x, std::vector<uint8_t> &v);

void split_uint_t(uint64_t &x, std::vector<uint8_t> &v);

void split_uint_t_vector(std::vector<uint64_t> &x, std::vector<uint8_t> &v);

void split_uint_t_vector(std::vector<uint32_t> &x, std::vector<uint8_t> &v);

void convert_string_to_uint8_t_vector(std::string &x, std::vector<uint8_t> &v);


unsigned long lower_power_of_two(unsigned long x);

int find_highest_bit(int v);


int upperpower2(int x);

// solve equation : 1 + x(logc - logx + 1) - c = 0
double F_d(double x, double c);

double F(double x, double c);

double solve_equation(double c);

double balls_in_bins_max_load(double balls, double bins);

int proper_alt_range(int M, int i, int *len);

#endif //HELPERS_H