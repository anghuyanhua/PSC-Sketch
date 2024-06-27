#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <vector>
#include <cstdint>
#include "hash.h"
using namespace std;

class HashTable {
public:
    HashTable(std::size_t hash_table_len) : hash_table_len(hash_table_len), hashTableSeed(10), hashTable(hash_table_len, 0){

    }

    void update(const std::string& src){
        std::size_t hash_idx = BOBHash(src, hashTableSeed) % hash_table_len;
        int64_t src_int = std::stoll(src);
        if (hashTable[hash_idx] == 0) {
            hashTable[hash_idx] = src_int;
        }
        else if(hashTable[hash_idx] == src_int){

        }
        else {
            hashTable[hash_idx] = src_int;
        }
    }

    std::vector<int64_t> report(){
        std::vector<int64_t> non_zero_elements;
        for (auto element : hashTable) {
            if (element != 0){
                non_zero_elements.push_back(element);
            }
        }
        return non_zero_elements;
    }

    void clean() {
        std::fill(hashTable.begin(), hashTable.end(), 0);
    }

private:
    std::size_t hash_table_len;
    int hashTableSeed;
    std::vector<int64_t> hashTable;
};


#endif