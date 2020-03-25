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
