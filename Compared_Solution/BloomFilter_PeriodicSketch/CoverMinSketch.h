#ifndef _COVERMINSKETCH_H_
#define _COVERMINSKETCH_H_

#include <vector>
#include <cstdint>
#include <algorithm> 
#include "../../lib/hash.h"
#include "GSUSketch.h"

using namespace std;

class CoverMinSketch {
public:
    CoverMinSketch(int d, int w, GSUSketch& gsuSketch) : d(d), w(w), gsuSketch(gsuSketch) {
        coverminsketch.resize(d, std::vector<int32_t>(w, 0));
    }

    void update(int64_t src, int heavychange_time) {
        int min_val = INT32_MAX;
        for (int i = 0; i < d; ++i) {
            size_t hash_idx = BOBHash(std::to_string(src), CoverMin_seeds[i]) % w;
            min_val = std::min(min_val, coverminsketch[i][hash_idx]);
            coverminsketch[i][hash_idx] = heavychange_time;
        }
        int interval = heavychange_time - min_val;
        gsuSketch.update(src, interval);
    }
    
private:
    int d, w;
    std::vector<std::vector<int32_t>> coverminsketch;
    std::vector<int> CoverMin_seeds{9, 10, 11, 12};
    GSUSketch& gsuSketch;
};

#endif