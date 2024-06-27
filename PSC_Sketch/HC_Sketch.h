#ifndef _HC_SKETCH_H_
#define _HC_SKETCH_H_

#include <vector>
#include <limits>
#include <fstream>
#include <cmath>
#include <cstring>
#include <set>
#include "hash.h"
#include "Topk_Structure.h"
using namespace std;

class HC_Sketch {
public:
    HC_Sketch(int d, int w, Topk_Structure* topk_structure) : d(d), w(w), field(4), hash_bucket_seed(104), topk_structure(topk_structure), H(5) {
        hc_sketch = new int64_t**[w];
        for (int i = 0; i < w; ++i) {
            hc_sketch[i] = new int64_t*[d];
            for (int j = 0; j < d; ++j) {
                hc_sketch[i][j] = new int64_t[field];
                memset(hc_sketch[i][j], 0, field * sizeof(int64_t));
            }
        }
    }

    ~HC_Sketch(){
        for (int i = 0; i < w; ++i) {
            for (int j = 0; j < d; ++j) {
                delete[] hc_sketch[i][j];
            }
            delete[] hc_sketch[i];
        }
        delete[] hc_sketch;
    }

    int get_bucket_column(){
        return w;
    }

    int get_bucket_seed(){
        return hash_bucket_seed;
    }

    void update(const std::string& src, int estIncrement, uint32_t hash_idx){
        int64_t src_uint = std::stoll(src);
        int min_val_idx = 0;
        int64_t min_val = std::numeric_limits<int64_t>::max();
        int empty_cell_flag = 0;
        int empty_cell_index = 0;

        for (int i = 0; i < d; i++){
            if (min_val > hc_sketch[hash_idx][i][2] && hc_sketch[hash_idx][i][3] == 0) {
                min_val = hc_sketch[hash_idx][i][2];
                min_val_idx = i;
            }
            if (src_uint == hc_sketch[hash_idx][i][0]) {
                hc_sketch[hash_idx][i][2] += estIncrement;
                return;
            } 
            else if (hc_sketch[hash_idx][i][0] == 0) {
                if (empty_cell_flag == 0){
                    empty_cell_flag = 1;
                    empty_cell_index = i;
                }
            }
        }
        if (empty_cell_flag == 1){
            hc_sketch[hash_idx][empty_cell_index][0] = src_uint;
            hc_sketch[hash_idx][empty_cell_index][1] = 0;
            hc_sketch[hash_idx][empty_cell_index][2] = estIncrement;
            hc_sketch[hash_idx][empty_cell_index][3] = 0;
            return;
        }
        // 发生替换
        hc_sketch[hash_idx][min_val_idx][2] -= 1;
        if (hc_sketch[hash_idx][min_val_idx][2] <= 0) {
            hc_sketch[hash_idx][min_val_idx][0] = src_uint;
            hc_sketch[hash_idx][min_val_idx][1] = 0;
            hc_sketch[hash_idx][min_val_idx][2] = estIncrement;
            hc_sketch[hash_idx][min_val_idx][3] = 0;
        }
    }
    
    int query(const std::string& src, uint32_t hash_idx){
        int64_t src_uint = std::stoll(src);
        for (int i = 0; i < d; i++){
            if (src_uint == hc_sketch[hash_idx][i][0]){
                return hc_sketch[hash_idx][i][2];
            }
        }
        return 0;
    }

    void outputToFile(const std::string& filename) {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            return;
        }

        for (int i = 0; i < w; i++) {
            for (int j = 0; j < d; j++) {
                outFile << "ID: " << hc_sketch[i][j][0] << ", "
                        << "Pre: " << hc_sketch[i][j][1] << ", "
                        << "Cur: " << hc_sketch[i][j][2] << ", "
                        << "Time: " << hc_sketch[i][j][3] << "\n";
            }
        }
        outFile.close();
    }

    void clean(){
        for (int i = 0; i < w; i++) {
            for (int j = 0; j < d; j++) {
                if (hc_sketch[i][j][0] != 0){
                    if (hc_sketch[i][j][2] == 0 && hc_sketch[i][j][3] == 0){
                        hc_sketch[i][j][0] = 0;
                        hc_sketch[i][j][1] = 0;
                    }
                    else {
                        hc_sketch[i][j][1] = hc_sketch[i][j][2];
                        hc_sketch[i][j][2] = 0;
                    }
                }
            }
        }
    }

    void detectHeavyChange(int time_now){
        for (int i = 0; i < w; ++i){
            for (int j = 0; j < d; ++j){
                int64_t& id = hc_sketch[i][j][0];
                int64_t& pre = hc_sketch[i][j][1];
                int64_t& cur = hc_sketch[i][j][2];
                int64_t& last_change_time = hc_sketch[i][j][3];

                if (pre > 0 && cur > 0 && std::abs(pre - cur) >= 30){
                    if (last_change_time == 0){
                        last_change_time = time_now;
                    }
                    else{
                        int interval = time_now - last_change_time;
                        topk_structure->update(id, interval);
                        last_change_time = time_now;
                    }
                }
            }
        }
    }

private:
    int d, w, field;
    int hash_bucket_seed;
    int H;
    Topk_Structure* topk_structure;
    int64_t*** hc_sketch;
};

#endif