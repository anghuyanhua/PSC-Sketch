#ifndef _GSUSKETCH_H_
#define _GSUSKETCH_H_

#include <vector>
#include <fstream>
#include <algorithm>
#include <random>
#include <map>
#include "../../lib/hash.h"

using namespace std;

class GSUSketch {
public:
    GSUSketch(std::size_t w, std::size_t cell_num) 
        : w(w), cell_num(cell_num), field(3), gsuSketch_seed(9), t_fail(w, 0) {
        gsuSketch.resize(w, std::vector<std::vector<int64_t>>(cell_num, std::vector<int64_t>(field, 0)));
    }

    void update(int64_t src, int64_t interval) {
        std::size_t hash_idx = BOBHash(std::to_string(src) + std::to_string(interval), gsuSketch_seed) % w; 
        int64_t min_val = INT64_MAX;
        std::size_t min_val_index = 0;

        for (int i = 0; i < cell_num; ++i) {
            if (gsuSketch[hash_idx][i][0] == src && gsuSketch[hash_idx][i][1] == interval) {
                gsuSketch[hash_idx][i][2]++;
                return;
            }

            if (gsuSketch[hash_idx][i][2] < min_val){
                min_val = gsuSketch[hash_idx][i][2];
                min_val_index = i;
            }
        }

        if (rng() % (2 * min_val - t_fail[hash_idx] + 1) == 0){
            gsuSketch[hash_idx][min_val_index][0] = src;
            gsuSketch[hash_idx][min_val_index][1] = interval;
            if (min_val == 0){
                gsuSketch[hash_idx][min_val_index][2] = 1;
            }
            else{
                gsuSketch[hash_idx][min_val_index][2] += t_fail[hash_idx] / min_val;
            }
            t_fail[hash_idx] = 0;
        }
        else{
            t_fail[hash_idx] += 1;
        }
    }

    std::map<std::pair<uint64_t, uint64_t>, uint64_t> report() {
        std::vector<std::vector<int64_t>> topk_list;
        for (std::size_t i = 0; i < w; ++i) {
            for (std::size_t j = 0; j < cell_num; ++j) {
                topk_list.push_back(gsuSketch[i][j]);
            }
        }
        std::sort(topk_list.begin(), topk_list.end(), [](const std::vector<int64_t>& a, const std::vector<int64_t>& b) {
            return a[2] > b[2];
        });
        
        std::ofstream file("./PeriodicSketch_Estimation_Result/topk_list.txt");
        for (const auto& item : topk_list) {
            file << "IP: " << item[0] << ", Interval: " << item[1] << ", Count: " << item[2] << "\n";
        }
        for (const auto& item : topk_list) {
            if (item[2] > 1) { 
                est_dict[{item[0], item[1]}] = item[2];
            }
        }
        return est_dict;
    }

private:
    std::size_t w, cell_num, field;
    int gsuSketch_seed;
    std::vector<std::vector<std::vector<int64_t>>> gsuSketch; // w x cell_num x field
    std::vector<int> t_fail;
    std::map<std::pair<uint64_t, uint64_t>, uint64_t> est_dict;
};

#endif