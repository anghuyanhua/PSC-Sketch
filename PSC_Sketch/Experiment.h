#ifndef _EXPERIMENT_H_
#define _EXPERIMENT_H_

#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <map>
#include <memory>
#include "BloomFilter_HSF_Screener.h"
#include "HC_Sketch.h"
#include "Topk_Structure.h"
using namespace std;

class Experiment {
public:
    Experiment(int bitmapLen, int w, int bucket_row, int bucket_column, int topk_structure_cell_num, int topk_structure_column) : bitmapLen(bitmapLen), w(w), bucket_row(bucket_row), bucket_column(bucket_column), topk_structure_cell_num(topk_structure_cell_num), topk_structure_column(topk_structure_column) {
        topk_structure = new Topk_Structure(topk_structure_cell_num, topk_structure_column);
        hc_sketch = new HC_Sketch(bucket_row, bucket_column, topk_structure);
        bloomfilter_hsf_screener = new BloomFilter_HSF_Screener(bitmapLen, w, hc_sketch);
    }

    ~Experiment() {
        delete topk_structure;
        delete hc_sketch;
        delete bloomfilter_hsf_screener;
    }

    BloomFilter_HSF_Screener* getBloomFilter_HSF_Screener() const {
        return bloomfilter_hsf_screener;
    }

    HC_Sketch* getHC_Sketch() const {
        return hc_sketch;
    }
    
    Topk_Structure* getTopk_Structure() const {
        return topk_structure;
    }
    
    void start(int index, const std::vector<std::string>& datas){
        for (const auto& pkt : datas) {
            std::istringstream iss(pkt);
            std::string src, dst;
            iss >> src >> dst;
            bloomfilter_hsf_screener->update(src, dst);
            real_set_dict[src].insert(dst);
        }
    }

    void start(const std::string& src, const std::string& dst){
            bloomfilter_hsf_screener->update(src, dst); 
    }

    void query(int time_window) {
        real_dict.clear();
        est_dict.clear();
        for (const auto& pair : real_set_dict) {
            const auto& src = pair.first;
            real_dict[src] = pair.second.size();
            uint32_t hash_idx_bucket = BOBHash(src, hc_sketch->get_bucket_seed()) % hc_sketch->get_bucket_column();
            est_dict[src] = hc_sketch->query(src, hash_idx_bucket);
        }
        std::cout << "[Messages]: The estimation process is finished. " << std::endl;

        std::ofstream realSetDictFile("./Real_Set_Dict_Result/real_set_dict" + std::to_string(time_window) + ".txt");
        for (const auto& pair : real_set_dict) {
            const std::string& src = pair.first;
            const std::set<std::string>& dsts = pair.second;

            realSetDictFile << src << ": ";
            for (const auto& dst : dsts) {
                realSetDictFile << dst << " ";
            }
            realSetDictFile << "\n";
        }
        std::ofstream realFile("./Real_Set_Dict_Result/src_spreads_real_dict_" + std::to_string(time_window) + ".txt");
        for (const auto& pair : real_dict) {
            realFile << pair.first << ": " << pair.second << "\n";
        }
        realFile.close();
        std::ofstream estFile("./Real_Set_Dict_Result/src_spreads_est_dict_" + std::to_string(time_window) + ".txt");
        for (const auto& pair : est_dict) {
            estFile << pair.first << ": " << pair.second << "\n";
        }
        estFile.close();
    }

private:
    int bitmapLen;
    int w;
    int bucket_row;
    int bucket_column;
    int topk_structure_cell_num;
    int topk_structure_column;
    Topk_Structure* topk_structure;
    HC_Sketch* hc_sketch;
    BloomFilter_HSF_Screener* bloomfilter_hsf_screener;
    std::map<std::string, std::set<std::string>> real_set_dict;
    std::map<std::string, int> real_dict;
    std::map<std::string, int> est_dict;
};

#endif