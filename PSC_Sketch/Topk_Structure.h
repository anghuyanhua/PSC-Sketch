#ifndef _TOPK_STRUCTURE_H_
#define _TOPK_STRUCTURE_H_

#include <vector>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <map>
#include "../lib/hash.h"
using namespace std;

class Topk_Structure {
public:
    Topk_Structure(int cell_num, int w) : cell_num(cell_num), w(w), field(3), hash_topk_structure_seed(106) {
        topk_structure = new int64_t**[w];
        for (int i = 0; i < w; ++i) {
            topk_structure[i] = new int64_t*[cell_num];
            for (int j = 0; j < cell_num; ++j) {
                topk_structure[i][j] = new int64_t[field];
                memset(topk_structure[i][j], 0, field * sizeof(int64_t));
            }
        }
    }

    ~Topk_Structure() {
        for (int i = 0; i < w; ++i) {
            for (int j = 0; j < cell_num; ++j) {
                delete[] topk_structure[i][j];
            }
            delete[] topk_structure[i];
        }
        delete[] topk_structure;
    }

    void update(int64_t src, int interval) {
        string common_ip = std::to_string(src) + std::to_string(interval);
        uint32_t hash_idx = BOBHash(common_ip, hash_topk_structure_seed) % w;

        int min_val_idx  = 0;
        int64_t min_val = std::numeric_limits<int64_t>::max();
        for (int i = 0; i< cell_num; i++){
            if (min_val > topk_structure[hash_idx][i][2]){
                min_val = topk_structure[hash_idx][i][2];
                min_val_idx = i;
            }
            if (src == topk_structure[hash_idx][i][0] && interval == topk_structure[hash_idx][i][1]){
                topk_structure[hash_idx][i][2] += 1;
                return;
            }
            else if (topk_structure[hash_idx][i][0] == 0){
                topk_structure[hash_idx][i][0] = src;
                topk_structure[hash_idx][i][1] = interval;
                topk_structure[hash_idx][i][2] = 1;
                return;
            }
        }
        topk_structure[hash_idx][min_val_idx][2] -= 1;
        if (topk_structure[hash_idx][min_val_idx][2] <= 0){
            topk_structure[hash_idx][min_val_idx][0] = src;
            topk_structure[hash_idx][min_val_idx][1] = interval;
            topk_structure[hash_idx][min_val_idx][2] = 1;
        }
    }

    std::map<std::pair<uint64_t, uint64_t>, uint64_t> report(){
        std::vector<std::vector<int64_t>> topk_list;
        for (int i = 0; i < w; ++i) {
            for(int j = 0; j < cell_num; ++j){
                topk_list.push_back({topk_structure[i][j][0], topk_structure[i][j][1], topk_structure[i][j][2]});
            }
        }
        std::sort(topk_list.begin(), topk_list.end(), [](const std::vector<int64_t>& a, const std::vector<int64_t>& b){
            return a[2] > b[2];
        });
        
        std::ofstream f("./PSCSketch_Estimation_Result/topk_list.txt");
        for (const auto& item : topk_list) {
            f << item[0] << ", " << item[1] << ", " << item[2] << "\n";
        }
        
        for (const auto& item : topk_list) {
            if (item[2] > 1) { 
                est_dict[{item[0], item[1]}] = item[2];
            }
        }
        return est_dict;
    }

private:
    int w;
    int cell_num;
    int field;
    int64_t*** topk_structure;
    int hash_topk_structure_seed;
    std::map<std::pair<uint64_t, uint64_t>, uint64_t> est_dict;
};

#endif
