#ifndef HASH_H
#define HASH_H

#include <random>
#include <iostream>
#include <limits.h>
#include <stdint.h>
#include "murmur3.h"
#include "BOBHash.h"

template<typename T>
inline uint32_t hash1(const T& data, uint32_t seed = 0);
inline double randomGenerator();

static std::random_device rd;
static std::mt19937 rng(rd());
static std::uniform_real_distribution<double> dis(0.0, 1.0);
inline double randomGenerator() {
    return dis(rng);
}

template<typename T>
inline uint32_t hash1(const T& data, uint32_t seed) {
    uint32_t output;
    MurmurHash3_x86_32(&data, sizeof(T), seed, &output);
    return output;
}

inline uint32_t hash1(const std::string& data, uint32_t seed) {
    uint32_t output;
    //MurmurHash3_x86_32(data.data(), data.size(), seed, &output);
    MurmurHash3_x86_32(data.c_str(), data.size(), seed, &output);
    return output;
}

inline uint32_t BOBHash(const std::string& data, uint32_t seed) {
    return BOBHash::BOBHash32(reinterpret_cast<const uint8_t*>(data.data()), data.length(), seed);
}

inline uint64_t BOBHash64(const std::string& data, uint32_t seed) {
    return BOBHash::BOBHash64(reinterpret_cast<const uint8_t*>(data.data()), data.length(), seed);
}
#endif
