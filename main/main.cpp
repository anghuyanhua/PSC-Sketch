#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <map>
#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric> 
#include <iomanip>
#include "groundtruth.h"

// PSC Sketch
#include "../PSC_Sketch/Experiment.h"

// Primitive Solution
#include "../Compared_Solution/PrimitiveSolution/BloomFilter.h"
#include "../Compared_Solution/PrimitiveSolution/HashTable.h"
#include "../Compared_Solution/PrimitiveSolution/CM.h"
#include "../Compared_Solution/PrimitiveSolution/HeavyChangeTable.h"
#include "../Compared_Solution/PrimitiveSolution/Topkstruct.h"
#include "../Compared_Solution/PrimitiveSolution/detection_heavy_change.h"
#include "../lib/hash.h"

// BH-BT
#include "../Compared_Solution/BH-BT/BloomFilter2.h"
#include "../Compared_Solution/BH-BT/HeavyKeeper.h"
#include "../Compared_Solution/BH-BT/BucketArray.h"
#include "../Compared_Solution/BH-BT/Topkstruct2.h"

// BloomFilter+PeriodicSketch
#include "../Compared_Solution/BloomFilter_PeriodicSketch/BloomFilter3.h"
#include "../Compared_Solution/BloomFilter_PeriodicSketch/HeavyKeeper3.h"
#include "../Compared_Solution/BloomFilter_PeriodicSketch/BucketArray3.h"
#include "../Compared_Solution/BloomFilter_PeriodicSketch/CoverMinSketch.h"
#include "../Compared_Solution/BloomFilter_PeriodicSketch/GSUSketch.h"
using namespace std;

std::vector<double> compare(std::map<std::pair<uint64_t, uint64_t>, uint64_t> real_dict, std::map<std::pair<uint64_t, uint64_t>, uint64_t> est_dict){
    double total_aae = 0.0, total_are = 0.0;
    size_t common_keys_count = 0;
    for (const auto& [key, real_value] : real_dict) {
        auto it = est_dict.find(key);
        if (it != est_dict.end()) {
            ++common_keys_count;
            uint64_t est_value = it->second;
            uint64_t abs_error = (real_value > est_value) ? (real_value - est_value) : (est_value - real_value);
            total_aae += abs_error;
            total_are += abs_error / static_cast<double>(real_value);
        }
    }
    double aae = common_keys_count > 0 ? total_aae / common_keys_count : 0.0;
    double are = common_keys_count > 0 ? total_are / common_keys_count : 0.0;
    double pr = est_dict.size() > 0 ? static_cast<double>(common_keys_count) / est_dict.size() : 0.0;
    double rr = real_dict.size() > 0 ? static_cast<double>(common_keys_count) / real_dict.size() : 0.0;
    double f1score = pr + rr > 0 ? 2 * pr * rr / (pr + rr) : 0.0;
    std::vector<double> metrics;
    metrics.push_back(aae);
    metrics.push_back(are);
    metrics.push_back(pr);
    metrics.push_back(rr);
    metrics.push_back(f1score);
    return metrics;
}

void printVectorInfo(const std::string& name, const std::vector<double>& vec) {
    std::cout << name << " = [";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i];
        if (i < vec.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

int main(){
    std::string file_path = "../dataset/processed_dataset_two_minutes.txt";
    std::ifstream f(file_path);
    if (!f.is_open()){
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return 1;
    }
    std::vector<std::string> temp_datas;
    std::string line;
    while (std::getline(f, line)) {
        temp_datas.push_back(line);
    }
    f.close();
    int line_count = temp_datas.size();
    std::string* datas = new std::string[line_count];
    for (int i = 0; i < line_count; ++i) {
        datas[i] = std::move(temp_datas[i]);
    }
    std::cout << "Total packets: " << line_count << std::endl;
    const int num_windows = 1200;
    const int window_size = line_count / num_windows;
    std::string** windowed_data = new std::string*[num_windows];
    for (int i = 0; i < num_windows; ++i) {
        int start_idx = i * window_size;
        int end_idx = std::min(start_idx + window_size, line_count);
        int window_length = end_idx - start_idx;
        windowed_data[i] = new std::string[window_length];
        for (int j = 0; j < window_length; ++j) {
            windowed_data[i][j] = datas[start_idx + j];
        }
    }
    std::pair<std::string, std::string>** windowed_ip_pairs = new std::pair<std::string, std::string>*[num_windows];
    for (int i = 0; i < num_windows; ++i) {
        int start_idx = i * window_size;
        int end_idx = std::min(start_idx + window_size, line_count);
        int window_length = end_idx - start_idx;
        windowed_ip_pairs[i] = new std::pair<std::string, std::string>[window_length];
        for (int j = start_idx; j < end_idx; ++j) {
            std::istringstream iss(datas[j]);
            std::string src, dst;
            if (iss >> src >> dst) {
                windowed_ip_pairs[i][j - start_idx] = std::make_pair(src, dst);
            } else {
                std::cerr << "Failed to parse IPs from line: " << datas[j] << std::endl;
            }
        }
    }

    std::vector<double> AAE_Ours, ARE_Ours, PR_Ours, RR_Ours, F1SCORE_Ours;
    std::vector<double> AAE_Primitive, ARE_Primitive, PR_Primitive, RR_Primitive, F1SCORE_Primitive;
    std::vector<double> AAE_BH_BT, ARE_BH_BT, PR_BH_BT, RR_BH_BT, F1SCORE_BH_BT;
    std::vector<double> AAE_PeriodicSketch, ARE_PeriodicSketch, PR_PeriodicSketch, RR_PeriodicSketch, F1SCORE_PeriodicSketch;
    std::vector<double> Throughtout_Ours, Throughtout_Primitive, Throughtout_BH_BT, Throughtout_PeriodicSketch;
    std::vector<double> memory_list = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};

    // Groundtruth Experiment
    auto real_dict = groundtruth(file_path);

    // PSC Sketch Experiment
    for (double m : memory_list) {
        double memory = m * 1024 * 1024 * 8;
        double memory_bloomfilter = memory * 0.35;
        double memory_hsf_screener = memory * 0.25;
        double memory_hc_sketch = memory * 0.3;
        double memory_topk = memory * 0.10;
        int bloomfilterLen = static_cast<int>(memory_bloomfilter / 1);
        int w = static_cast<int>(memory_hsf_screener / 4);
        int hcsketch_row = 4;
        int hcsketch_column = static_cast<int>(memory_hc_sketch / (hcsketch_row * 104));
        int topk_cell_num = 4;
        int topk_column = static_cast<int>(memory_topk / (topk_cell_num * 48));

        Experiment* experiment = new Experiment(bloomfilterLen, w, hcsketch_row, hcsketch_column, topk_cell_num, topk_column);
        auto start_time = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_windows; ++i){
            for (int j = 0; j < window_size; ++j) {
                std::pair<std::string, std::string>& ip_pair = windowed_ip_pairs[i][j];
                const std::string& src = ip_pair.first;
                const std::string& dst = ip_pair.second;
                experiment->start(src, dst);
            }
            experiment->getHC_Sketch()->detectHeavyChange(i);
            experiment->getBloomFilter_HSF_Screener()->clean();
            experiment->getHC_Sketch()->clean();
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> running_time = end_time - start_time;

        std::cout <<"Memory is: "<< m <<" MB, " << "Ours Running Time: " << running_time.count() << "s\n";
        std::cout <<"Memory is: "<< m <<" MB, " << "Ours Throughput: " << line_count / running_time.count() / 1e6 << " M packets/s\n";
        Throughtout_Ours.push_back(line_count / running_time.count() / 1e6);
        std::map<std::pair<uint64_t, uint64_t>, uint64_t> est_dict_my_alogorithm = experiment->getTopk_Structure()->report();
        std::vector<double> metrics = compare(real_dict, est_dict_my_alogorithm);
        std::cout <<"Memory is : "<< m <<" MB, " << "Ours AAE: " << metrics[0] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Ours ARE: " << metrics[1] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Ours Precision Rate (PR): " << metrics[2] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Ours Recall Rate (RR): " << metrics[3] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Ours F1SCORE: " << metrics[4] << std::endl << std::endl <<std::endl;
        AAE_Ours.push_back(metrics[0]);
        ARE_Ours.push_back(metrics[1]);
        PR_Ours.push_back(metrics[2]);
        RR_Ours.push_back(metrics[3]);
        F1SCORE_Ours.push_back(metrics[4]);



        // Primitive Solution Experiment
        memory = m * 1024 * 1024 * 8;
        double memory_bloomFilter = memory * 0.05;
        double memory_hash_table = memory * 0.37;
        double memory_CM = memory * 0.51;
        double memory_HeavyChangeTable = memory * 0.02;
        double memory_topkstruct = memory * 0.05;
        int bloomFilterLen = static_cast<int>(memory_bloomFilter / 1);
        int hash_table_len = static_cast<int>(memory_hash_table / (32 * 2));
        int CM_row = 4;
        int CM_column = static_cast<int>(memory_CM / (4 * 16 * 2));
        int HeavyChangeTable_column = static_cast<int>(memory_HeavyChangeTable / 40);
        int topkstruct_column = static_cast<int>(memory_topkstruct / 48);
        BloomFilter bloomFilter(bloomFilterLen);
        std::vector<CM> count_min_windows(2, CM(4, CM_column));
        std::vector<HashTable> hashTable_windows(2, HashTable(hash_table_len));
        Topkstruct topkstruct(topkstruct_column);
        HeavyChangeTable heavyChangeTable(HeavyChangeTable_column, topkstruct);

        start_time = std::chrono::high_resolution_clock::now();
        for (size_t time_window = 0; time_window < num_windows; ++time_window) {
            for (int j = 0; j < window_size; ++j) {
                std::pair<std::string, std::string>& ip_pair = windowed_ip_pairs[time_window][j];
                const std::string& src = ip_pair.first;
                const std::string& dst = ip_pair.second;
                bloomFilter.update(src, dst, count_min_windows[(time_window + 1) % 2], hashTable_windows[(time_window + 1) % 2]);
            }
            detection_heavy_change(count_min_windows[time_window % 2], count_min_windows[(time_window + 1) % 2], 
                                   hashTable_windows[time_window % 2], hashTable_windows[(time_window + 1) % 2], 
                                   30, heavyChangeTable, time_window);
            bloomFilter.clean();
            hashTable_windows[time_window % 2].clean();
            count_min_windows[time_window % 2].clean();
        }
        end_time = std::chrono::high_resolution_clock::now();
        running_time = end_time - start_time;

        std::cout <<"Memory is: "<< m <<" MB, " << "Primitive Solution Running Time: " << running_time.count() << "s\n";
        std::cout <<"Memory is: "<< m <<" MB, " << "Primitive Solution Throughput: " << line_count / running_time.count() / 1e6 << " M packets/s\n";
        Throughtout_Primitive.push_back(line_count / running_time.count() / 1e6);
        std::map<std::pair<uint64_t, uint64_t>, uint64_t> est_dict_Primitive = topkstruct.report();
        metrics = compare(real_dict, est_dict_Primitive);
        std::cout <<"Memory is : "<< m <<" MB, " << "Primitive Solution AAE: " << metrics[0] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Primitive Solution ARE: " << metrics[1] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Primitive Solution Precision Rate (PR): " << metrics[2] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Primitive Solution Recall Rate (RR): " << metrics[3] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Primitive Solution F1SCORE: " << metrics[4] << std::endl << std::endl <<std::endl;
        AAE_Primitive.push_back(metrics[0]);
        ARE_Primitive.push_back(metrics[1]);
        PR_Primitive.push_back(metrics[2]);
        RR_Primitive.push_back(metrics[3]);
        F1SCORE_Primitive.push_back(metrics[4]);



        // BH-BT Experiment
        memory = m * 1024 * 1024 * 8;
        double memory_bloomFilter2 = memory * 0.05;
        double memory_HeavyKeeper = memory * 0.45;
        double memory_bucket = memory * 0.45;
        double memory_topkstruct2 = memory * 0.05;
        bloomFilterLen = static_cast<int>(memory_bloomFilter2 / 1);
        int HeavyKeeper_row = 4;
        int HeavyKeeper_column = static_cast<int>(memory_HeavyKeeper / (4 * 48));
        int bucket_column = static_cast<int>(memory_bucket / 64);
        int topkstruct2_column = static_cast<int>(memory_topkstruct2 / (48));
        BloomFilter2 bloomFilter2(bloomFilterLen);
        HeavyKeeper heavykeeper(HeavyKeeper_row, HeavyKeeper_column);
        Topkstruct2 topkstruct2(topkstruct2_column);
        BucketArray bucketarray(bucket_column, 30, topkstruct2);

        start_time = std::chrono::high_resolution_clock::now();
        for (size_t time_window = 0; time_window < num_windows; ++time_window) {
            for (int j = 0; j < window_size; ++j) {
                std::pair<std::string, std::string>& ip_pair = windowed_ip_pairs[time_window][j];
                const std::string& src = ip_pair.first;
                const std::string& dst = ip_pair.second;
                bloomFilter2.update(src, dst, heavykeeper);
            }
            auto ip_spread_dict = heavykeeper.extract_flows_and_frequencies();
            for (const auto& [ip, spread] : ip_spread_dict) {
                bucketarray.update(ip, spread, time_window);
            }
            bloomFilter2.clean();
            heavykeeper.clean();
            bucketarray.clean(time_window);
        }
        end_time = std::chrono::high_resolution_clock::now();
        running_time = end_time - start_time;

        std::cout <<"Memory is: "<< m <<" MB, " << "BH-BT Running Time: " << running_time.count() << "s\n";
        std::cout <<"Memory is: "<< m <<" MB, " << "BH-BT Throughput: " << line_count / running_time.count() / 1e6 << " M packets/s\n";
        Throughtout_BH_BT.push_back(line_count / running_time.count() / 1e6);
        auto est_dict_BH_BT = topkstruct2.report();
        metrics = compare(real_dict, est_dict_BH_BT);
        std::cout <<"Memory is : "<< m <<" MB, " << "BH-BT AAE: " << metrics[0] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "BH-BT ARE: " << metrics[1] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "BH-BT Precision Rate (PR): " << metrics[2] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "BH-BT Recall Rate (RR): " << metrics[3] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "BH-BT F1SCORE: " << metrics[4] << std::endl << std::endl <<std::endl;
        AAE_BH_BT.push_back(metrics[0]);
        ARE_BH_BT.push_back(metrics[1]);
        PR_BH_BT.push_back(metrics[2]);
        RR_BH_BT.push_back(metrics[3]);
        F1SCORE_BH_BT.push_back(metrics[4]);



        // BloomFilter+PeriodicSketch Experiment
        memory = m * 1024 * 1024 * 8;
        double memory_bloomFilter3 = memory * 0.05;
        double memory_heavykeeper3 = memory * 0.4;
        double memory_bucketArray3 = memory * 0.4;
        double memory_CoverminSketch = memory * 0.15;
        double memory_GSUSketch = memory * 0.05;
        bloomFilterLen = static_cast<int>(memory_bloomFilter3 / 1);
        HeavyKeeper_row = 4;
        HeavyKeeper_column = static_cast<int>(memory_heavykeeper3 / (4 * 48));
        int bucketArray_column = static_cast<int>(memory_bucketArray3 / 56);
        int Covermin_row = 4;
        int Covermin_column = static_cast<int>(memory_CoverminSketch / (4 * 8));
        int cell_num = 4;
        int GSUSketch_column = static_cast<int>(memory_GSUSketch / (48 * cell_num));
        BloomFilter3 bloomFilter3(bloomFilterLen);
        HeavyKeeper3 heavykeeper3(HeavyKeeper_row, HeavyKeeper_column);
        GSUSketch gsuSketch(GSUSketch_column, cell_num);
        CoverMinSketch coverminSketch(Covermin_row, Covermin_column, gsuSketch);
        BucketArray3 bucketArray3(bucketArray_column, 30, coverminSketch);
        
        start_time = std::chrono::high_resolution_clock::now();
        for (size_t time_window = 0; time_window < num_windows; ++time_window) {
            for (int j = 0; j < window_size; ++j) {
                std::pair<std::string, std::string>& ip_pair = windowed_ip_pairs[time_window][j];
                const std::string& src = ip_pair.first;
                const std::string& dst = ip_pair.second;
                bloomFilter3.update(src, dst, heavykeeper3);
            }
            auto ip_spread_dict = heavykeeper3.extract_flows_and_frequencies();
            for (const auto& [ip, spread] : ip_spread_dict) {
                bucketArray3.update(ip, spread, time_window);
            }
            bloomFilter3.clean();
            heavykeeper3.clean();
            bucketArray3.clean(time_window);
        }
        end_time = std::chrono::high_resolution_clock::now();
        running_time = end_time - start_time;

        std::cout <<"Memory is: "<< m <<" MB, " << "BloomFilter+PeriodicSketch Running Time: " << running_time.count() << "s\n";
        std::cout <<"Memory is: "<< m <<" MB, " << "BloomFilter+PeriodicSketch Throughput: " << line_count / running_time.count() / 1e6 << " M packets/s\n";
        Throughtout_PeriodicSketch.push_back(line_count / running_time.count() / 1e6);
        auto est_dict_PeriodicSketch = gsuSketch.report();
        metrics = compare(real_dict, est_dict_PeriodicSketch);
        std::cout <<"Memory is : "<< m <<" MB, " << "BloomFilter+PeriodicSketch AAE: " << metrics[0] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "BloomFilter+PeriodicSketch ARE: " << metrics[1] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "BloomFilter+PeriodicSketch Precision Rate (PR): " << metrics[2] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "BloomFilter+PeriodicSketch Recall Rate (RR): " << metrics[3] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "BloomFilter+PeriodicSketch F1SCORE: " << metrics[4] << std::endl << std::endl <<std::endl;
        AAE_PeriodicSketch.push_back(metrics[0]);
        ARE_PeriodicSketch.push_back(metrics[1]);
        PR_PeriodicSketch.push_back(metrics[2]);
        RR_PeriodicSketch.push_back(metrics[3]);
        F1SCORE_PeriodicSketch.push_back(metrics[4]);
    }

    // Print Experimental Results
    printVectorInfo("AAE_Ours", AAE_Ours);
    printVectorInfo("AAE_Primitive", AAE_Primitive);
    printVectorInfo("AAE_BH_BT", AAE_BH_BT);
    printVectorInfo("AAE_PeriodicSketch", AAE_PeriodicSketch);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("ARE_Ours", ARE_Ours);
    printVectorInfo("ARE_Primitive", ARE_Primitive);
    printVectorInfo("ARE_BH_BT", ARE_BH_BT);
    printVectorInfo("ARE_PeriodicSketch", ARE_PeriodicSketch);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("PR_Ours", PR_Ours);
    printVectorInfo("PR_Primitive", PR_Primitive);
    printVectorInfo("PR_BH_BT", PR_BH_BT);
    printVectorInfo("PR_PeriodicSketch", PR_PeriodicSketch);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("RR_Ours", RR_Ours);
    printVectorInfo("RR_Primitive", RR_Primitive);
    printVectorInfo("RR_BH_BT", RR_BH_BT);
    printVectorInfo("RR_PeriodicSketch", RR_PeriodicSketch);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("F1SCORE_Ours", F1SCORE_Ours);
    printVectorInfo("F1SCORE_Primitive", F1SCORE_Primitive);
    printVectorInfo("F1SCORE_BH_BT", F1SCORE_BH_BT);
    printVectorInfo("F1SCORE_PeriodicSketch", F1SCORE_PeriodicSketch);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("Throu_Freebs", Throughtout_Ours);
    printVectorInfo("Throu_Strawman1", Throughtout_Primitive);
    printVectorInfo("Throu_Strawman2", Throughtout_BH_BT);
    printVectorInfo("Throu_Strawman3", Throughtout_PeriodicSketch);
    return 0;
}
