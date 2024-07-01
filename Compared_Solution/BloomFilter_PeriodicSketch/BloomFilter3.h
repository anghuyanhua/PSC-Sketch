#ifndef _BLOOMFILTER3_H_
#define _BLOOMFILTER3_H_

#include <vector>
#include <cstdint>
#include "../../lib/hash.h"
#include "HeavyKeeper3.h"

using namespace std;

class BloomFilter3 {
public:
    BloomFilter3(std::size_t bloomfilterLen) : bloomfilterLen(bloomfilterLen), bloomfilter(bloomfilterLen, 0), bloomfilter_hashNum(3), bloomfilter_Seed{1, 2, 3}{

    }

    void update(const std::string& src, const std::string& dst, HeavyKeeper3& heavykeeper) {
        if (!query(src, dst)){
            heavykeeper.update(src);
            for (int i = 0; i < bloomfilter_hashNum; ++i){
                std::size_t hash_idx = BOBHash(src + dst, bloomfilter_Seed[i]) % bloomfilterLen;
                bloomfilter[hash_idx] = 1;
            }
        }
    }

    bool query(const std::string& src, const std::string& dst) {
        for (int i = 0; i < bloomfilter_hashNum; ++i){
            std::size_t hash_idx = BOBHash(src + dst, bloomfilter_Seed[i]) % bloomfilterLen;
            if (bloomfilter[hash_idx] == 0){
                return false;
            }
        }
        return true;
    }

    void clean(){
        std::fill(bloomfilter.begin(), bloomfilter.end(), 0);
    }

private:
    std::size_t bloomfilterLen;
    std::vector<int8_t> bloomfilter;
    int bloomfilter_hashNum;
    int bloomfilter_Seed[3];
};

#endif