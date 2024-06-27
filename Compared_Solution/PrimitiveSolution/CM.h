#ifndef _CM_H_
#define _CM_H_

#include <vector>
#include <cstdint>
#include <algorithm>
#include "hash.h"
using namespace std;

class CM {
public:
    CM(std::size_t d, std::size_t w) : d(d), w(w), CM_hashNum(d), CM_seeds{4, 5, 6, 7}{
        CMSketch.resize(d, vector<int64_t>(w, 0));
    }

    void update(const std::string& src) {
        for (std::size_t i = 0; i < CM_hashNum; ++i){
            std::size_t pos = BOBHash(src, CM_seeds[i]) % w;
            ++CMSketch[i][pos];
        }
    }

    int64_t query(const std::string& src){
        std::vector<int64_t> res;
        for (std::size_t i = 0; i < CM_hashNum; ++i){
            std::size_t pos = BOBHash(src, CM_seeds[i]) % w;
            res.push_back(CMSketch[i][pos]);
        }
        std::sort(res.begin(), res.end());
        return res.front();
    }

    void clean() {
        for (auto& row : CMSketch) {
            std::fill(row.begin(), row.end(), 0);
        }
    }
    
private:
    std::vector<std::vector<int64_t>> CMSketch;  // w x d x field
    std::size_t d, w, CM_hashNum;
    std::vector<int> CM_seeds;
};

#endif