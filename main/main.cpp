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

//PSH Sketch
#include "../PSC_Sketch/Experiment.h"

// Primitive Solution
#include "../Compared_Solution/PrimitiveSolution/BloomFilter.h"
#include "../Compared_Solution/PrimitiveSolution/HashTable.h"
#include "../Compared_Solution/PrimitiveSolution/CM.h"
#include "../Compared_Solution/PrimitiveSolution/HeavyChangeTable.h"
#include "../Compared_Solution/PrimitiveSolution/Topkstruct.h"
#include "../Compared_Solution/PrimitiveSolution/detection_heavy_change.h"
#include "../lib/hash.h"

//BH-BT
#include "../Compared_Solution/BH-BT/BloomFilter2.h"
#include "../Compared_Solution/BH-BT/HeavyKeeper.h"
#include "../Compared_Solution/BH-BT/BucketArray.h"
#include "../Compared_Solution/BH-BT/Topkstruct2.h"

//BloomFilter+PeriodicSketch
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

    // PSH Sketch Experiment
    for (double m : memory_list) {
        double memory = m * 1024 * 1024 * 8;
        double memory_bloomfilter = memory * 0.35;
        double memory_hsf_screener = memory * 0.25;
        double memory_hc_sketch = memory * 0.3;
        double memory_topk = memory * 0.10;

        int bloomfilterLen = static_cast<int>(memory_bloomfilter / 1);
        int w = static_cast<int>(memory_hsf_screener / 4);
        int bucket_row = 4;
        int bucket_column = static_cast<int>(memory_hc_sketch / (bucket_row * 104));
        int topk_cell_num = 4;
        int topk_column = static_cast<int>(memory_topk / (topk_cell_num * 48));

        Experiment* experiment = new Experiment(bloomfilterLen, w, bucket_row, bucket_column, topk_cell_num, topk_column);
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

        std::cout <<"Memory is: "<< m <<" MB, " << "Freebs_ColdFilter Running time: " << running_time.count() << "s\n";
        std::cout <<"Memory is: "<< m <<" MB, " << "Freebs_ColdFilter Throughput: " << line_count / running_time.count() / 1e6 << " M packets/s\n";
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
        int topk_column = static_cast<int>(memory_topkstruct / 48);

        BloomFilter bloomFilter(bloomFilterLen);
        std::vector<CM> count_min_windows(2, CM(4, CM_column));
        std::vector<HashTable> hashTable_windows(2, HashTable(hash_table_len));
        Topkstruct topkstruct(topk_column);
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

        std::cout <<"Memory is: "<< m <<" MB, " << "Primitive Solution Running time: " << running_time.count() << "s\n";
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







        
        //Strawman2实验
        memory = m * 1024 * 1024 * 8;
        // double memory_bloomFilter2 = memory * 0.1;
        // double memory_CM2 = memory * 0.45;
        // double memory_CoverminSketch2 = memory * 0.45;
        // double memory_GSUSketch2 = memory * 0.008;

        // double memory_bloomFilter2 = memory * 0.20;
        // double memory_CM2 = memory * 0.40;
        // double memory_CoverminSketch2 = memory * 0.39;
        // double memory_GSUSketch2 = memory * 0.01;

        // double memory_bloomFilter2 = memory * 0.1;
        // double memory_CM2 = memory * 0.5;
        // double memory_CoverminSketch2 = memory * 0.3;
        // double memory_GSUSketch2 = memory * 0.1;

        // double memory_bloomFilter2 = memory * 0.15;
        // double memory_CM2 = memory * 0.45;
        // double memory_CoverminSketch2 = memory * 0.3;
        // double memory_GSUSketch2 = memory * 0.1;

        // double memory_bloomFilter2 = memory * 0.1;
        // double memory_CM2 = memory * 0.45;
        // double memory_CoverminSketch2 = memory * 0.44;
        // double memory_GSUSketch2 = memory * 0.01;

        // double memory_bloomFilter2 = memory * 0.15;
        // double memory_CM2 = memory * 0.4;
        // double memory_CoverminSketch2 = memory * 0.3;
        // double memory_GSUSketch2 = memory * 0.15;

        // double memory_bloomFilter2 = memory * 0.15;
        // double memory_CM2 = memory * 0.4;
        // double memory_CoverminSketch2 = memory * 0.35;
        // double memory_GSUSketch2 = memory * 0.1;

        // double memory_bloomFilter2 = memory * 0.1;
        // double memory_CM2 = memory * 0.45;
        // double memory_CoverminSketch2 = memory * 0.4;
        // double memory_GSUSketch2 = memory * 0.05;

        double memory_bloomFilter2 = memory * 0.05;
        double memory_CM2 = memory * 0.45;
        double memory_CoverminSketch2 = memory * 0.45;
        double memory_GSUSketch2 = memory * 0.05;



        bloomFilterLen = static_cast<int>(memory_bloomFilter2 / 1);
        CM_row = 4;
        CM_column = static_cast<int>(memory_CM2 / (4 * 48));

        int CoverminSketch_column = static_cast<int>(memory_CoverminSketch2 / 64);
        //int CoverminSketch_column = static_cast<int>(memory_CoverminSketch2 / 80);
        //int cell_num = 4;
        int GSUSketch_column = static_cast<int>(memory_GSUSketch2 / (48));
        //int GSUSketch_column = static_cast<int>(memory_GSUSketch2 / (56 * cell_num));

        BloomFilterStrawman2 bloomFilterStrawman2(bloomFilterLen);
        CMStrawman2 CMSketchStrawman2(CM_row, CM_column);
        GSUSketchStrawman2 gsuSketchStrawman2(GSUSketch_column);
        CoverMinSketchStrawman2 coverminSketchStrawman2(CoverminSketch_column, 30, gsuSketchStrawman2);

        start_time = std::chrono::high_resolution_clock::now();
        for (size_t time_window = 0; time_window < num_windows; ++time_window) {
            // size_t start_idx = time_window * window_size;
            // size_t end_idx = std::min(start_idx + window_size, datas.size());

            // for (size_t i = start_idx; i < end_idx; ++i) {
            //     std::istringstream iss(datas[i]);
            //     std::string src, dst;
            //     iss >> src >> dst;
            //     bloomFilterStrawman2.update(src, dst, CMSketchStrawman2);
            // }

            for (int j = 0; j < window_size; ++j) {
                std::pair<std::string, std::string>& ip_pair = windowed_ip_pairs[time_window][j];
                const std::string& src = ip_pair.first;
                const std::string& dst = ip_pair.second;
                bloomFilterStrawman2.update(src, dst, CMSketchStrawman2);
            }


            auto ip_spread_dict = CMSketchStrawman2.extract_flows_and_frequencies();
            //std::ofstream f("../Strawman2_Modified_Covermin_Gsu/Strawman2_Estimation_Result_File/ip_spread_dict_" + std::to_string(time_window));
            
            for (const auto& [ip, spread] : ip_spread_dict) {
                //f << "IP:" << ip << ", Spread:" << spread << "\n";
                coverminSketchStrawman2.update(ip, spread, time_window);
            }
            //f.close();

            bloomFilterStrawman2.clean();
            CMSketchStrawman2.clean();
            // 直接不进行covermin清理工作，看下效果
            coverminSketchStrawman2.clean(time_window);
        }
        end_time = std::chrono::high_resolution_clock::now();
        running_time = end_time - start_time;

        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman2 Running time: " << running_time.count() << "s\n";
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman2 Throughput: " << line_count / running_time.count() / 1e6 << " M packets/s\n";
        Throughtout_Strawman2.push_back(line_count / running_time.count() / 1e6);

        auto est_dict_Strawman2 = gsuSketchStrawman2.report();
        metrics = compare(real_dict, est_dict_Strawman2);
        std::cout <<"Memory is : "<< m <<" MB, " << "Strawman2 AAE: " << metrics[0] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman2 ARE: " << metrics[1] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman2 Precision Rate (PR): " << metrics[2] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman2 Recall Rate (RR): " << metrics[3] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman2 F1SCORE: " << metrics[4] << std::endl << std::endl <<std::endl;
        AAE_Strawman2.push_back(metrics[0]);
        ARE_Strawman2.push_back(metrics[1]);
        PR_Strawman2.push_back(metrics[2]);
        RR_Strawman2.push_back(metrics[3]);
        F1SCORE_Strawman2.push_back(metrics[4]);








        //Strawman3实验
        memory = m * 1024 * 1024 * 8;
        // double memory_bloomFilter3 = memory * 0.1;
        // double memory_CM3 = memory * 0.5;
        // double memory_bucketArray3 = memory * 0.3;
        // double memory_CoverminSketch3 = memory * 0.01;
        // double memory_GSUSketch3 = memory * 0.008;

        // double memory_bloomFilter3 = memory * 0.15;
        // double memory_CM3 = memory * 0.45;
        // double memory_bucketArray3 = memory * 0.35;
        // double memory_CoverminSketch3 = memory * 0.03;
        // double memory_GSUSketch3 = memory * 0.02;

        // double memory_bloomFilter3 = memory * 0.05;
        // double memory_CM3 = memory * 0.5;
        // double memory_bucketArray3 = memory * 0.33;
        // double memory_CoverminSketch3 = memory * 0.02;
        // double memory_GSUSketch3 = memory * 0.1;

        // double memory_bloomFilter3 = memory * 0.1;
        // double memory_CM3 = memory * 0.55;
        // double memory_bucketArray3 = memory * 0.3;
        // double memory_CoverminSketch3 = memory * 0.03;
        // double memory_GSUSketch3 = memory * 0.02;

        // double memory_bloomFilter3 = memory * 0.15;
        // double memory_CM3 = memory * 0.4;
        // double memory_bucketArray3 = memory * 0.3;
        // double memory_CoverminSketch3 = memory * 0.05;
        // double memory_GSUSketch3 = memory * 0.1;

        double memory_bloomFilter3 = memory * 0.05;
        double memory_CM3 = memory * 0.4;
        double memory_bucketArray3 = memory * 0.4;
        double memory_CoverminSketch3 = memory * 0.15;
        double memory_GSUSketch3 = memory * 0.05;


        bloomFilterLen = static_cast<int>(memory_bloomFilter3 / 1);
        CM_row = 4;
        CM_column = static_cast<int>(memory_CM3 / (4 * 48));
        int bucketArray_column = static_cast<int>(memory_bucketArray3 / 56);
        //int bucketArray_column = static_cast<int>(memory_bucketArray3 / 64);
        int Covermin_row = 4;
        int Covermin_column = static_cast<int>(memory_CoverminSketch3 / (4 * 8));
        //int Covermin_column = static_cast<int>(memory_CoverminSketch3 / (4 * 16));
        int cell_num = 4;
        GSUSketch_column = static_cast<int>(memory_GSUSketch3 / (48 * cell_num));
        //GSUSketch_column = static_cast<int>(memory_GSUSketch3 / (56 * cell_num));

        BloomFilterStrawman3 bloomFilterStrawman3(bloomFilterLen);
        CMStrawman3 CMSketchStrawman3(CM_row, CM_column);
        GSUSketchStrawman3V2 gsuSketchStrawman3(GSUSketch_column, cell_num);
        //GSUSketchStrawman3 gsuSketch(GSUSketch_column, cell_num);
        CoverMinSketchStrawman3V2 coverminSketchStrawman3(Covermin_row, Covermin_column, gsuSketchStrawman3);
        //CoverMinSketchStrawman3 coverminSketch(Covermin_row, Covermin_column, gsuSketch);
        BucketArrayStrawman3 bucketArrayStrawman3(bucketArray_column, 30, coverminSketchStrawman3);
        
    
        start_time = std::chrono::high_resolution_clock::now();
        for (size_t time_window = 0; time_window < num_windows; ++time_window) {
            // size_t start_idx = time_window * window_size;
            // size_t end_idx = std::min(start_idx + window_size, datas.size());

            // for (size_t i = start_idx; i < end_idx; ++i) {
            //     std::istringstream iss(datas[i]);
            //     std::string src, dst;
            //     iss >> src >> dst;
            //     bloomFilterStrawman3.update(src, dst, CMSketchStrawman3);
            // }
            
            for (int j = 0; j < window_size; ++j) {
                std::pair<std::string, std::string>& ip_pair = windowed_ip_pairs[time_window][j];
                const std::string& src = ip_pair.first;
                const std::string& dst = ip_pair.second;
                bloomFilterStrawman3.update(src, dst, CMSketchStrawman3);
            }

            auto ip_spread_dict = CMSketchStrawman3.extract_flows_and_frequencies();
            //std::ofstream f("../Strawman3_Raw_Version_Covermin_Gsu/Strawman3_Estimation_Result_File/ip_spread_dict_" + std::to_string(time_window));
            for (const auto& [ip, spread] : ip_spread_dict) {
                //f << "IP:" << ip << ", Spread:" << spread << "\n";
                bucketArrayStrawman3.update(ip, spread, time_window);
            }
            //f.close();

            bloomFilterStrawman3.clean();
            CMSketchStrawman3.clean();
            bucketArrayStrawman3.clean(time_window);
        }
        end_time = std::chrono::high_resolution_clock::now();
        running_time = end_time - start_time;

        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman3 Running time: " << running_time.count() << "s\n";
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman3 Throughput: " << line_count / running_time.count() / 1e6 << " M packets/s\n";
        Throughtout_Strawman3.push_back(line_count / running_time.count() / 1e6);

        auto est_dict_Strawman3 = gsuSketchStrawman3.report();
        metrics = compare(real_dict, est_dict_Strawman3);
        std::cout <<"Memory is : "<< m <<" MB, " << "Strawman3 AAE: " << metrics[0] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman3 ARE: " << metrics[1] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman3 Precision Rate (PR): " << metrics[2] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman3 Recall Rate (RR): " << metrics[3] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman3 F1SCORE: " << metrics[4] << std::endl << std::endl <<std::endl;
        AAE_Strawman3.push_back(metrics[0]);
        ARE_Strawman3.push_back(metrics[1]);
        PR_Strawman3.push_back(metrics[2]);
        RR_Strawman3.push_back(metrics[3]);
        F1SCORE_Strawman3.push_back(metrics[4]);
        











        /*
        // Strawman4:1200个CM和1200个hashtable实验
        memory = m * 1024 * 1024 * 8;
        // double memory_bloomFilter = memory * 0.05;
        // double memory_hash_table = memory * 0.37;
        // double memory_CM = memory * 0.57;
        // double memory_HeavyChangeTable = memory * 0.002;
        // double memory_topkstruct = memory * 0.008;
        
        // 这个比例怎么确定，怎么分配呢？
        // double memory_bloomFilter = memory * 0.15;
        // double memory_hash_table = memory * 0.3;
        // double memory_CM = memory * 0.45;
        // double memory_HeavyChangeTable = memory * 0.002;
        // double memory_topkstruct = memory * 0.008;

        // double memory_bloomFilter = memory * 0.02;
        // double memory_hash_table = memory * 0.47;
        // double memory_CM = memory * 0.5;
        // double memory_HeavyChangeTable = memory * 0.002;
        // double memory_topkstruct = memory * 0.008;

        // double memory_bloomFilter = memory * 0.02;
        // double memory_hash_table = memory * 0.45;
        // double memory_CM = memory * 0.5;
        // double memory_HeavyChangeTable = memory * 0.012;
        // double memory_topkstruct = memory * 0.018;

        // double memory_bloomFilter = memory * 0.1;
        // double memory_hash_table = memory * 0.45;
        // double memory_CM = memory * 0.43;
        // double memory_HeavyChangeTable = memory * 0.01;
        // double memory_topkstruct = memory * 0.01;

        // 这个配置跑出来的效果不错
        // double memory_bloomFilter = memory * 0.05;
        // double memory_hash_table = memory * 0.37;
        // double memory_CM = memory * 0.51;
        // double memory_HeavyChangeTable = memory * 0.02;
        // double memory_topkstruct = memory * 0.05;

        // 这个配置效果最好
        // double memory_bloomFilter = memory * 0.05;
        // double memory_hash_table = memory * 0.37;
        // double memory_CM = memory * 0.45;
        // double memory_HeavyChangeTable = memory * 0.03;
        // double memory_topkstruct = memory * 0.1;

        // double memory_bloomFilter = memory * 0.1;
        // double memory_hash_table = memory * 0.3;
        // double memory_CM = memory * 0.4;
        // double memory_HeavyChangeTable = memory * 0.05;
        // double memory_topkstruct = memory * 0.15;

        double memory_bloomFilter_Strawman4 = memory * 0.05;
        double memory_hash_table_Strawman4 = memory * 0.37;
        double memory_CM_Strawman4 = memory * 0.51;
        double memory_HeavyChangeTable_Strawman4 = memory * 0.02;
        double memory_topkstruct_Strawman4 = memory * 0.05;




        int bloomFilterLen_Strawman4 = static_cast<int>(memory_bloomFilter_Strawman4 / 1);
        //这里需要根据创建的CMSketch实例数量修改除数
        int hash_table_len_Strawman4 = static_cast<int>(memory_hash_table_Strawman4 / (32 * num_windows));
        //int hash_table_len_Strawman4 = static_cast<int>(memory_hash_table_Strawman4 / (32 * 2));
        int CM_row_Strawman4 = 4;
        int CM_column_Strawman4 = static_cast<int>(memory_CM_Strawman4 / (4 * 16 * num_windows));
        //int CM_column = static_cast<int>(memory_CM / (4 * 16 * 2));
        int HeavyChangeTable_column_Strawman4 = static_cast<int>(memory_HeavyChangeTable_Strawman4 / 40);
        int topk_column_Strawman4 = static_cast<int>(memory_topkstruct_Strawman4 / 48);

        BloomFilter bloomFilter_strawman4(bloomFilterLen_Strawman4);
        std::vector<CM> count_min_windows_strawman4(num_windows + 1, CM(4, CM_column_Strawman4));
        //std::vector<CM> count_min_windows(2, CM(4, CM_column));
        std::vector<HashTable> hashTable_windows_strawman4(num_windows + 1, HashTable(hash_table_len_Strawman4));
        //std::vector<HashTable> hashTable_windows(2, HashTable(hash_table_len));
        Topkstruct topkstruct_strawman4(topk_column_Strawman4);
        HeavyChangeTable heavyChangeTable_strawman4(HeavyChangeTable_column_Strawman4, topkstruct_strawman4);

        start_time = std::chrono::high_resolution_clock::now();

        // for (int i = 0; i < num_windows; ++i){
        //     auto& window_data = windowed_data[i];
        //     for (auto& data : window_data) {
        //         std::istringstream ss(data);
        //         std::string src, dst;
        //         ss >> src >> dst;
        //         // 然后在这里使用src和dst
        //         bloomFilter.update(src, dst, count_min_windows[i + 1], hashTable_windows[i + 1]);
        //     }
        // }

        for (size_t time_window = 0; time_window < num_windows; ++time_window) {
            size_t start_idx = time_window * window_size;
            size_t end_idx = std::min(start_idx + window_size, datas.size());

            for (size_t i = start_idx; i < end_idx; ++i) {
                std::istringstream ss(datas[i]);
                std::string src, dst;
                ss >> src >> dst;
                // [time_window+1]表示当前时间窗口，[time_window]表示上一时间窗口
                bloomFilter_strawman4.update(src, dst, count_min_windows_strawman4[time_window + 1], hashTable_windows_strawman4[time_window + 1]);
                // 偶数窗口和奇数窗口(time_window + 1)%2==0为偶数窗口
                //bloomFilter.update(src, dst, count_min_windows[(time_window + 1) % 2], hashTable_windows[(time_window + 1) % 2]);
            }

            detection_heavy_change(count_min_windows_strawman4[time_window], count_min_windows_strawman4[time_window + 1], 
                                   hashTable_windows_strawman4[time_window], hashTable_windows_strawman4[time_window + 1], 
                                   30, heavyChangeTable_strawman4, time_window);
            // detection_heavy_change(count_min_windows[time_window % 2], count_min_windows[(time_window + 1) % 2], 
            //                        hashTable_windows[time_window % 2], hashTable_windows[(time_window + 1) % 2], 
            //                        30, heavyChangeTable, time_window);
            bloomFilter_strawman4.clean();
            // hashTable_windows[time_window % 2].clean();
            // count_min_windows[time_window % 2].clean();
        }
        end_time = std::chrono::high_resolution_clock::now();
        running_time = end_time - start_time;

        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman4 Running time: " << running_time.count() << "s\n";
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman4 Throughput: " << datas.size() / running_time.count() / 1e6 << " M packets/s\n";
        Throughtout_Strawman4.push_back(datas.size() / running_time.count() / 1e6);

        std::map<std::pair<uint64_t, uint64_t>, uint64_t> est_dict_Strawman4 = topkstruct_strawman4.report();
        metrics = compare(real_dict, est_dict_Strawman4);
        std::cout <<"Memory is : "<< m <<" MB, " << "Strawman4 AAE: " << metrics[0] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman4 ARE: " << metrics[1] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman4 Precision Rate (PR): " << metrics[2] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman4 Recall Rate (RR): " << metrics[3] << std::endl;
        std::cout <<"Memory is: "<< m <<" MB, " << "Strawman4 F1SCORE: " << metrics[4] << std::endl << std::endl <<std::endl;
        AAE_Strawman4.push_back(metrics[0]);
        ARE_Strawman4.push_back(metrics[1]);
        PR_Strawman4.push_back(metrics[2]);
        RR_Strawman4.push_back(metrics[3]);
        F1SCORE_Strawman4.push_back(metrics[4]);    
        */
    }

    //打印向量信息
    printVectorInfo("AAE_Freebs", AAE_Freebs);
    printVectorInfo("AAE_Strawman1", AAE_Strawman1);
    printVectorInfo("AAE_Strawman2", AAE_Strawman2);
    printVectorInfo("AAE_Strawman3", AAE_Strawman3);
    //printVectorInfo("AAE_Strawman4", AAE_Strawman4);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("ARE_Freebs", ARE_Freebs);
    printVectorInfo("ARE_Strawman1", ARE_Strawman1);
    printVectorInfo("ARE_Strawman2", ARE_Strawman2);
    printVectorInfo("ARE_Strawman3", ARE_Strawman3);
    //printVectorInfo("ARE_Strawman4", ARE_Strawman4);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("PR_Freebs", PR_Freebs);
    printVectorInfo("PR_Strawman1", PR_Strawman1);
    printVectorInfo("PR_Strawman2", PR_Strawman2);
    printVectorInfo("PR_Strawman3", PR_Strawman3);
    //printVectorInfo("PR_Strawman4", PR_Strawman4);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("RR_Freebs", RR_Freebs);
    printVectorInfo("RR_Strawman1", RR_Strawman1);
    printVectorInfo("RR_Strawman2", RR_Strawman2);
    printVectorInfo("RR_Strawman3", RR_Strawman3);
    //printVectorInfo("RR_Strawman4", RR_Strawman4);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("F1SCORE_Freebs", F1SCORE_Freebs);
    printVectorInfo("F1SCORE_Strawman1", F1SCORE_Strawman1);
    printVectorInfo("F1SCORE_Strawman2", F1SCORE_Strawman2);
    printVectorInfo("F1SCORE_Strawman3", F1SCORE_Strawman3);
    //printVectorInfo("F1SCORE_Strawman4", F1SCORE_Strawman4);
    std::cout << endl;
    std::cout << endl;
    printVectorInfo("Throu_Freebs", Throughtout_Freebs);
    printVectorInfo("Throu_Strawman1", Throughtout_Strawman1);
    printVectorInfo("Throu_Strawman2", Throughtout_Strawman2);
    printVectorInfo("Throu_Strawman3", Throughtout_Strawman3);
    //printVectorInfo("Throu_Strawman4", Throughtout_Strawman4);

    return 0;
}
