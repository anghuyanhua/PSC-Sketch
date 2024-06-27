#ifndef _BLOOMFILTER_HSF_SCREENER_H_
#define _BLOOMFILTER_HSF_SCREENER_H_

#include <vector>
#include <limits>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "hash.h"
#include "HC_Sketch.h"
using namespace std;

class BloomFilter_HSF_Screener {
public:
    BloomFilter_HSF_Screener(int bloomfilterLen, int w, HC_Sketch* hc_sketch) : bloomfilterLen(bloomfilterLen), w(w), hc_sketch(hc_sketch)
    {
        bloomfilter = new int[bloomfilterLen];
        HSF_Screener = new int[w];
        memset(bloomfilter, 0, bloomfilterLen * sizeof(int));
        memset(HSF_Screener, 0, w * sizeof(int));
    }

    ~BloomFilter_HSF_Screener() {
        delete[] bloomfilter;
        delete[] HSF_Screener;
    }

    void update(const std::string& src, const std::string& dst){
        int min_val = std::numeric_limits<int>::max();
        bool flag = false;
        uint64_t hash_value = BOBHash64(src + dst, hash_bloomfilter_seeds[0]);
        for (int i = 0; i < hashNum_bloomfilter; ++i){
            uint64_t hash_idx = (hash_value & 0xFFFFFFFF) % bloomfilterLen;
            hash_value >>= 32;
            if(bloomfilter[hash_idx] == 0){
                bloomfilter[hash_idx] = 1;
                flag = true;
            }
        }
        if (flag){
            int estIncrement = 1;
            uint32_t hash_idx_bucket = BOBHash(src, hc_sketch->get_bucket_seed()) % hc_sketch->get_bucket_column();

            if (hc_sketch->query(src, hash_idx_bucket)){
                hc_sketch->update(src, estIncrement, hash_idx_bucket);
            }
            else{
                uint64_t hash_value_screener = BOBHash64(src, hash_HSF_Screener_seeds[0]);
                for (int i = 0; i < hashNum; ++i) {
                    uint64_t hash_idx_screener = (hash_value_screener & 0xFFFF) % w;
                    hash_indices[i] = hash_idx_screener;
                    hash_value_screener >>= 16;
                    //min_val = std::min(min_val, ColdFilter[hash_indices[i]]);
                    if (HSF_Screener[hash_idx_screener] < min_val){
                        min_val = HSF_Screener[hash_idx_screener];
                    }
                }
                if (min_val == H - 1){
                    hc_sketch->update(src, H, hash_idx_bucket);
                }
                else{
                    for (uint64_t hash_idx : hash_indices) {
                        if (HSF_Screener[hash_idx] < H - 1){
                            HSF_Screener[hash_idx] += estIncrement;
                        }
                    }
                }
            }
        }
    }

    void clean(){
        memset(bloomfilter, 0, bloomfilterLen * sizeof(int));
        memset(HSF_Screener, 0, w * sizeof(int));
    }


private:
    int bloomfilterLen;
    int* bloomfilter;
    
    int w;
    int* HSF_Screener;
    static constexpr int H = 5;
    HC_Sketch* hc_sketch;
    static constexpr int hashNum = 3;
    static constexpr int hashNum_bloomfilter = 2;

    static constexpr int hash_bloomfilter_seeds[hashNum_bloomfilter] = {105, 106};
    static constexpr int hash_HSF_Screener_seeds[hashNum] = {102, 103, 104};
    uint64_t hash_indices[hashNum]; 
};


#endif