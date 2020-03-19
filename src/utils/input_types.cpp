//
// Created by ssqstone on 2018/7/19.
//

#include "input_types.h"

Distribution InputBase::distribution = Distribution::uniform;
std::default_random_engine InputBase::generator;

int InputBase::bound = INT32_MAX;
zipf_distribution<int, double> InputBase::expo;
std::uniform_int_distribution<int> InputBase::unif(0, bound);
