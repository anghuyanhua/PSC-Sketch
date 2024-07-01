#ifndef _BLOOMFILTER_H_
#define _BLOOMFILTER_H_

#include <vector>
#include <cstdint>
#include "../../lib/hash.h"
#include "HashTable.h"
#include "CM.h"

using namespace std;

class BloomFilter {
public:
    BloomFilter(std::size_t bloomfilterLen) : bloomfilterLen(bloomfilterLen), bloomfilter(bloomfilterLen, 0), bloomfilter_hashNum(3), bloomfilter_Seed{1, 2, 3}{

    }

    void update(const std::string& src, const std::string& dst, CM& cm, HashTable& hashTable) {
        if (!query(src, dst)){
            hashTable.update(src);
            cm.update(src);
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