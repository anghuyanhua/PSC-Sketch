#ifndef _TOPKSTRUCT2_H_
#define _TOPKSTRUCT2_H_

#include <vector>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include <map>
#include "../../lib/hash.h"

using namespace std;

class Topkstruct2 {
public:
    Topkstruct2(std::size_t w) : w(w), field(3), topkSeed(9) {
        topkstruct.resize(w);
        for (auto& item : topkstruct) {
            item.resize(field, 0); 
        }
    }

    void update(int64_t src, int64_t interval) {
        std::size_t hash_idx = BOBHash(std::to_string(src) + std::to_string(interval), topkSeed) % w;
        if (src == topkstruct[hash_idx][0] && interval == topkstruct[hash_idx][1]) {
            topkstruct[hash_idx][2] += 1;
        } 
        else if (topkstruct[hash_idx][0] == 0) {
            topkstruct[hash_idx][0] = src;
            topkstruct[hash_idx][1] = interval;
            topkstruct[hash_idx][2] = 1;
        } 
        else {
            topkstruct[hash_idx][2] -= 1;
            if (topkstruct[hash_idx][2] <= 0) {
                topkstruct[hash_idx][0] = src;
                topkstruct[hash_idx][1] = interval;
                topkstruct[hash_idx][2] = 1;
            }
        }
    }

    std::map<std::pair<uint64_t, uint64_t>, uint64_t> report() {
        std::vector<std::vector<int64_t>> sorted_topk_list(topkstruct.begin(), topkstruct.end());
        std::sort(sorted_topk_list.begin(), sorted_topk_list.end(),
                  [](const std::vector<int64_t>& a, const std::vector<int64_t>& b) {
                      return a[2] > b[2];
                  });

        std::ofstream file("./BH_BT_Estimation_Result/topk_list.txt");
        for (const auto& item : sorted_topk_list) {
            if (item[0] != 0) { 
                file << item[0] << ", " << item[1] << ", " << item[2] << "\n";
            }
        }

        for (const auto& item : sorted_topk_list) {
            if (item[2] > 1) { 
                est_dict[{item[0], item[1]}] = item[2];
            }
        }
        return est_dict;
    }

private:
    std::vector<std::vector<int64_t>> topkstruct; // w x field
    std::size_t w, field, topkSeed;
    std::map<std::pair<uint64_t, uint64_t>, uint64_t> est_dict;
};

#endif