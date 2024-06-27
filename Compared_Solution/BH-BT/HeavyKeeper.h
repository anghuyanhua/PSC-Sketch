#ifndef _HeavyKeeper_H_
#define _HeavyKeeper_H_

#include <vector>
#include <cstdint>
#include <algorithm>
#include <unordered_map>
#include "hash.h"
using namespace std;

class HeavyKeeper {
public:
    HeavyKeeper(std::size_t d, std::size_t w) : d(d), w(w), field(2), heavykeeper_hashNum(d), heavykeeper_seeds{4, 5, 6, 7}{
        heavykeeper.resize(w);
        for (auto& column : heavykeeper) {
            column.resize(d);
            for (auto& cell : column) {
                cell.resize(field, 0);
            }
        }
    }

    void update(const std::string& src) {
        int64_t src_int = std::stoll(src);
        for (size_t i = 0; i < heavykeeper_hashNum; ++i) {
            size_t hash_idx = BOBHash(src, heavykeeper_seeds[i]) % w;
            if (src_int == heavykeeper[hash_idx][i][0]) {
                heavykeeper[hash_idx][i][1] += 1;
            } 
            else if (0 == heavykeeper[hash_idx][i][0]) {
                heavykeeper[hash_idx][i][0] = src_int;
                heavykeeper[hash_idx][i][1] = 1;
            } 
            else {
                heavykeeper[hash_idx][i][1] -= 1;
                if (heavykeeper[hash_idx][i][1] <= 0) {
                    heavykeeper[hash_idx][i][0] = src_int;
                    heavykeeper[hash_idx][i][1] = 1;
                }
            }
        }
    }

    int64_t query(const std::string& src){
        int64_t src_int = std::stoll(src);
        std::vector<int64_t> res;
        for (std::size_t i = 0; i < heavykeeper_hashNum; ++i){
            std::size_t pos = BOBHash(src, heavykeeper_seeds[i]) % w;
            if (src_int == heavykeeper[pos][i][0]){
                res.push_back(heavykeeper[pos][i][1]);
            }
        }
        if (res.empty()) {
            return 0; 
        }
        std::sort(res.begin(), res.end());
        // return max
        return res.back();
    }

    void clean() {
        for (auto& column : heavykeeper) {
            for (auto& cell : column) {
                cell[0] = 0;
                cell[1] = 0;
            }
        }
    }

    std::unordered_map<int64_t, int64_t> extract_flows_and_frequencies() {
        std::unordered_map<int64_t, int64_t> flows;
        for (size_t i = 0; i < w; ++i) {
            for (size_t j = 0; j < d; ++j) {
                int64_t flow_id = heavykeeper[i][j][0];
                int64_t frequency = heavykeeper[i][j][1];

                if (flow_id != 0) {
                    auto it = flows.find(flow_id);
                    if (it == flows.end()) {
                        flows[flow_id] = frequency;
                    } else {
                        it->second = std::max(it->second, frequency);
                    }
                }
            }
        }

        std::unordered_map<int64_t, int64_t> filtered_flows;
        for (const auto& flow : flows) {   
            if (flow.second >= 5) {
                filtered_flows.insert(flow);
            }
        }
        return filtered_flows;
    }
    
private:
    std::vector<std::vector<std::vector<int64_t>>> heavykeeper;  // w x d x field
    std::size_t d, w, field, heavykeeper_hashNum;
    std::vector<int> heavykeeper_seeds;
};

#endif