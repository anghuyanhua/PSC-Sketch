#ifndef _DETECTIONHEAVYCHANGE_H_
#define _DETECTIONHEAVYCHANGE_H_

#include <vector>
#include <set>
#include <cstdint>
#include <fstream>
#include <algorithm>
#include "hash.h"
#include "HashTable.h"
#include "CM.h"
#include "HeavyChangeTable.h"

using namespace std;

void detection_heavy_change(CM& count_min1, CM& count_min2, 
                            HashTable& hash_table1, HashTable& hash_table2, 
                            int64_t T, HeavyChangeTable& heavyChangeTable, 
                            int64_t time_now) {
    std::vector<int64_t> report1 = hash_table1.report();
    std::vector<int64_t> report2 = hash_table2.report();

    std::set<int64_t> set1(report1.begin(), report1.end());
    std::set<int64_t> set2(report2.begin(), report2.end());

    std::set<int64_t> common_ips;
    std::set_intersection(set1.begin(), set1.end(), 
                          set2.begin(), set2.end(), 
                          std::inserter(common_ips, common_ips.begin()));

    for (auto ip : common_ips) {
        int64_t diff = std::abs(count_min1.query(std::to_string(ip)) - count_min2.query(std::to_string(ip)));
        if (count_min1.query(std::to_string(ip)) >= 5 && count_min2.query(std::to_string(ip)) >= 5 && diff >= T) {
            heavyChangeTable.update(ip, time_now);
        }
    }
}

#endif