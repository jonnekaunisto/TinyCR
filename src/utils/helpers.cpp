#include "helpers.h"



uint32_t combine_chars_as_uint(std::vector<uint8_t> &data, uint32_t val)
{
    /*read 4 chars at begin and combine them as uint32_t*/
    uint8_t ch1, ch2, ch3, ch4;
    ch1 = data[0];
    ch2 = data[1];
    ch3 = data[2];
    ch4 = data[3];

    return (ch1<<24) + (ch2<<16) + (ch3<<8) + ch4;
}

uint64_t combine_chars_as_uint(std::vector<uint8_t> &data, uint64_t val)
{
    /*read 4 chars at begin and combine them as uint32_t*/
    uint8_t ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8;
    ch1 = data[0];
    ch2 = data[1];
    ch3 = data[2];
    ch4 = data[3];
    ch5 = data[4];
    ch6 = data[5];
    ch7 = data[6];
    ch8 = data[7];

    return ((uint64_t)ch1<<56) + ((uint64_t)ch2<<48) + ((uint64_t)ch3<<40) + ((uint64_t)ch4<<32) 
    + ((uint64_t)ch5<<24) + ((uint64_t)ch6<<16) + ((uint64_t)ch7<<8) + (uint64_t)ch8;
    
}

uint32_t combine_chars_as_uint32_t(std::vector<uint8_t> &data, uint32_t begin)
{
    /*read 4 chars at begin and combine them as uint32_t*/
    uint8_t ch1, ch2, ch3, ch4;
    ch1 = data[begin];
    ch2 = data[begin+1];
    ch3 = data[begin+2];
    ch4 = data[begin+3];

    uint32_t x = (ch1<<24) + (ch2<<16) + (ch3<<8) + ch4;
    return x;
}


uint64_t combine_chars_as_uint64_t(std::vector<uint8_t> &data, uint32_t begin)
{
    /*read 4 chars at begin and combine them as uint32_t*/
    uint8_t ch1, ch2, ch3, ch4, ch5, ch6, ch7, ch8;
    ch1 = data[begin];
    ch2 = data[begin+1];
    ch3 = data[begin+2];
    ch4 = data[begin+3];
    ch5 = data[begin+4];
    ch6 = data[begin+5];
    ch7 = data[begin+6];
    ch8 = data[begin+7];
    

    uint64_t x = ((uint64_t)ch1<<56) + ((uint64_t)ch2<<48) + ((uint64_t)ch3<<40) + ((uint64_t)ch4<<32) 
    + ((uint64_t)ch5<<24) + ((uint64_t)ch6<<16) + ((uint64_t)ch7<<8) + (uint64_t)ch8;
    return x;
}

void split_uint_t(uint32_t &x, std::vector<uint8_t> &v)
{
    for(int i =3; i>=0; i--)
    {
        uint8_t temp = (x>>(i*8)) & 0x000000ff;
        v.push_back(temp);
    }
}

void split_uint_t(uint64_t &x, std::vector<uint8_t> &v)
{
    for(int i =7; i>=0; i--)
    {
        uint8_t temp = (x>>(i*8)) & 0x000000ff;
        v.push_back(temp);
    }
}

void split_uint_t_vector(std::vector<uint64_t> &x, std::vector<uint8_t> &v)
{
    for(int i=0; i<x.size(); i++)
    {
        for(int j=7;j>=0;j--)
        {
            uint8_t temp = (x[i]>>(j*8)) & 0x000000ff;
            v.push_back(temp);
        }
    }
}

void split_uint_t_vector(std::vector<uint32_t> &x, std::vector<uint8_t> &v)
{
    for(int i=0; i<x.size(); i++)
    {
        for(int j=3;j>=0;j--)
        {
            uint8_t temp = (x[i]>>(j*8)) & 0x000000ff;
            v.push_back(temp);
        }
    }
}

void convert_string_to_uint8_t_vector(std::string &x, std::vector<uint8_t> &v)
{
    const char* cx = x.c_str();
    for(int i=0; i<x.length(); i++)
    {
        uint8_t vt;
        std::memcpy(&vt, &cx[i], 1);
        v.push_back(vt);
    }
}


unsigned long lower_power_of_two(unsigned long x)
{
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x - (x >> 1);
}

int find_highest_bit(int v)
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


int upperpower2(int x)
{
    int ret = 1;
    for (; ret * 2 < x; ) ret <<= 1;
    return ret;
}

// solve equation : 1 + x(logc - logx + 1) - c = 0
double F_d(double x, double c)
{
  return log(c) - log(x);
}

double F(double x, double c)
{
  return 1 + x * (log(c) - log(x) + 1) - c;
}

double solve_equation(double c) 
{
  double x = c + 0.1;
  while (abs(F(x, c)) > 0.01)
    x -= F(x, c) / F_d(x, c);
  return x;
}

double balls_in_bins_max_load(double balls, double bins)
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

int proper_alt_range(int M, int i, int *len)
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
