#ifndef _HEAVYCHANGETABLE_H_
#define _HEAVYCHANGETABLE_H_

#include <vector>
#include <cstdint>
#include "../../lib/hash.h"
#include "Topkstruct.h"

using namespace std;

class HeavyChangeTable {
public:
    HeavyChangeTable(std::size_t w, Topkstruct& topkstruct) : w(w), field(2), tableSeed(8), topkstruct(topkstruct) {
        heavychange_table.resize(w, std::vector<int64_t>(field, 0));
    }

    void update(int64_t src, int64_t time_now){
        std::size_t hash_idx = BOBHash(std::to_string(src), tableSeed) % w;
        if (src == heavychange_table[hash_idx][0]) {
            int64_t interval = time_now - heavychange_table[hash_idx][1];
            topkstruct.update(src, interval);
            heavychange_table[hash_idx][1] = time_now;
        }
        else if (heavychange_table[hash_idx][0] == 0) {
            heavychange_table[hash_idx][0] = src;
            heavychange_table[hash_idx][1] = time_now;
        }
        else {
            heavychange_table[hash_idx][0] = src;
            heavychange_table[hash_idx][1] = time_now;
        }
    }

    std::vector<int64_t> query(int src) {
        std::size_t hash_idx = BOBHash(std::to_string(src), tableSeed) % w;
        return {heavychange_table[hash_idx][0], heavychange_table[hash_idx][1]};
    }

private:
    std::vector<std::vector<int64_t>> heavychange_table;  // w x field
    std::size_t w, field;
    int tableSeed;
    Topkstruct &topkstruct;
};

#endif