#ifndef _BUCKETARRAY_H_
#define _BUCKETARRAY_H_

#include <vector>
#include <fstream>
#include <algorithm>
#include "hash.h"
#include "Topkstruct2.h"
using namespace std;

class BucketArray {
public:
    BucketArray(size_t w, int64_t T, Topkstruct2& topkstruct): w(w), T(T), field(4), bucketArray(w, std::vector<int64_t>(field, 0)), bucket_seeds(11), topkstruct(topkstruct) {
    
    }

    void update(int64_t src, int64_t spread, int64_t time_now) {
        size_t hash_idx = BOBHash(std::to_string(src), bucket_seeds) % w;
        if (src == bucketArray[hash_idx][0]) {
            int64_t diff = std::abs(bucketArray[hash_idx][1] - spread);
            if (bucketArray[hash_idx][1] != 0 && diff >= T) {
                if (bucketArray[hash_idx][3] == 0) {
                    bucketArray[hash_idx][3] = time_now;
                } 
                else {
                    int64_t interval = time_now - bucketArray[hash_idx][3];
                    topkstruct.update(src, interval);
                    bucketArray[hash_idx][3] = time_now;
                }
            }
            bucketArray[hash_idx][1] = spread;
            bucketArray[hash_idx][2] = time_now;
        } 
        else if (bucketArray[hash_idx][0] == 0) {
            bucketArray[hash_idx][0] = src;
            bucketArray[hash_idx][1] = spread;
            bucketArray[hash_idx][2] = time_now;
            bucketArray[hash_idx][3] = 0;
        } 
        else {
            bucketArray[hash_idx][0] = src;
            bucketArray[hash_idx][1] = spread;
            bucketArray[hash_idx][2] = time_now;
            bucketArray[hash_idx][3] = 0;
        }
    }

    std::vector<int64_t> query(int64_t src) {
        size_t hash_idx = BOBHash(std::to_string(src), bucket_seeds) % w;
        if (src == bucketArray[hash_idx][0]) {
            return {bucketArray[hash_idx][0], bucketArray[hash_idx][1], bucketArray[hash_idx][2]};
        } 
        else {
            return {}; 
        }
    }

    void clean(int64_t time_now) {
        if (time_now != 0) {
            for (size_t i = 0; i < w; ++i) {
                if (bucketArray[i][2] == time_now - 1 && bucketArray[i][3] == 0) {
                    for (size_t j = 0; j < field; ++j) {
                        bucketArray[i][j] = 0;
                    }
                }
                if (bucketArray[i][2] == time_now -1 && bucketArray[i][3] != 0){
                    bucketArray[i][1] = 0;
                }
            }
        }
    }

private:
    size_t w;
    int64_t T;
    size_t field;
    std::vector<std::vector<int64_t>> bucketArray;
    int bucket_seeds;
    Topkstruct2& topkstruct; 
};

#endif