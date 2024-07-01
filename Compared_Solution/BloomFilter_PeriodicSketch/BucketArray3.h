#ifndef _BUCKETARRAY3_H_
#define _BUCKETARRAY3_H_

#include <vector>
#include <cstdint>
#include <cmath>
#include <fstream>
#include "../../lib/hash.h"
#include "CoverMinSketch.h"
using namespace std;

class BucketArray3 {
public:
    BucketArray3(int w, int T, CoverMinSketch& coverminSketch) : w(w), T(T), coverminSketch(coverminSketch){
        bucketArray.resize(w, std::vector<int64_t>(field, 0));
    }

    void update(int64_t src, int64_t spread, int time_now) {
        size_t hash_idx = BOBHash(std::to_string(src), bucketArray_seeds) % w;
        if (src == bucketArray[hash_idx][0]) {
            int diff = std::abs(bucketArray[hash_idx][1] - spread);
            if (diff >= T) {
                coverminSketch.update(src, time_now);
                bucketArray[hash_idx][2] = time_now;
            }
            bucketArray[hash_idx][1] = spread;
            bucketArray[hash_idx][2] = time_now;
        } 
        else if (bucketArray[hash_idx][0] == 0) {
            bucketArray[hash_idx] = {src, spread, time_now};
        } 
        else {
            bucketArray[hash_idx] = {src, spread, time_now};
        }
    }

    std::vector<int64_t> query(int src) {
        size_t hash_idx = BOBHash(std::to_string(src), bucketArray_seeds) % w;
        if (src == bucketArray[hash_idx][0]) {
            return bucketArray[hash_idx];
        } 
        else {
            return {}; 
        }
    }

    void clean(int time_now) {
        if (time_now != 0) {
            for (int i = 0; i < w; ++i) {
                if (bucketArray[i][2] == time_now - 1) {
                    bucketArray[i] = {0, 0, 0};
                }
            }
        }
    }
    
private:
    int w;
    int T;
    int field = 3;
    std::vector<std::vector<int64_t>> bucketArray;
    int bucketArray_seeds = 8;
    CoverMinSketch& coverminSketch;
};

#endif