#ifndef HELPERS_H
#define HELPERS_H
#include <vector>
#include <stdint.h>
#include <string>
#include <cstring>

uint32_t combine_chars_as_uint(std::vector<uint8_t> &data, uint32_t val);

uint32_t combine_chars_as_uint32_t(std::vector<uint8_t> &data, uint32_t begin);

uint64_t combine_chars_as_uint64_t(std::vector<uint8_t> &data, uint32_t begin);

void split_uint_t(uint32_t &x, std::vector<uint8_t> &v);

void split_uint_t(uint64_t &x, std::vector<uint8_t> &v);

void split_uint_t_vector(std::vector<uint64_t> &x, std::vector<uint8_t> &v);

void split_uint_t_vector(std::vector<uint32_t> &x, std::vector<uint8_t> &v);

void convert_string_to_uint8_t_vector(std::string &x, std::vector<uint8_t> &v);

#endif //HELPERS_H