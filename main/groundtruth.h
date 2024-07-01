#ifndef _GROUNDTRUTH_H_
#define _GROUNDTRUTH_H_
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <cstdint>
using namespace std;

vector<string> split(const string &s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

map<pair<uint64_t, uint64_t>, uint64_t> groundtruth(const string &file_path) {
    const int num_windows = 1200;
    vector<string> lines;
    string line;
    ifstream file(file_path);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << file_path << endl;
        return {};    
    }
    while (getline(file, line)) {
        lines.push_back(line);
    }
    file.close();
    std::cout << "Reading data is finished! " << endl;
    int window_size = lines.size() / num_windows;
    vector<vector<string>> windows_data(num_windows);
    for (int i = 0; i < num_windows; ++i) {
        auto start_iter = begin(lines) + i * window_size;
        auto end_iter = begin(lines) + (i + 1) * window_size;
        windows_data[i] = vector<string>(start_iter, end_iter);
    }
    std::cout << "The window splitting is finished! " << endl;

    // Record flow spread
    std::map<int, std::map<string, int>> result_dict;
    for (int i = 0; i < num_windows; ++i) {
        std::map<string, std::set<string>> ip_counts;
        for (auto &line : windows_data[i]) {
            auto tokens = split(line, ' ');
            string source_ip = tokens[0];
            string dest_ip = tokens[1];
            ip_counts[source_ip].insert(dest_ip);
        }
        for (auto &[ip, dest_ips] : ip_counts) {
            result_dict[i][ip] = dest_ips.size();
        }
    }
    std::cout << "Recording real spreads is finished! " << endl;
    map<int, map<string, int>> filtered_result_dict;
    for (auto &[window, ip_counts] : result_dict) {
        for (auto &[ip, count] : ip_counts) {
            if (count >= 5) {
                filtered_result_dict[window][ip] = count;
            }
        }
    }
    std::cout << "Filtering small spreads is finished! " << endl;

    // Detect heavy spread change
    map<int, map<string, int>> burst_filtered_dict;
    for (auto it = filtered_result_dict.begin(); it != prev(filtered_result_dict.end()); ++it) {
        int current_subwindow = it->first;
        int next_subwindow = next(it)->first;
        for (auto &[ip, freq_current] : it->second) {
            if (next(it)->second.find(ip) != next(it)->second.end()) {
                int freq_next = next(it)->second[ip];
                int freq_difference = freq_next - freq_current;
                if (abs(freq_difference) >= 30) {
                    burst_filtered_dict[next_subwindow][ip] = freq_next;
                }
            }
        }
    }
    std::cout << "Detection is finished! " << endl;

    // Calculate the time interval
    map<string, vector<int>> ip_windows_mapping;
    for (auto &[window, ip_counts] : burst_filtered_dict) {
        for (auto &[ip, _] : ip_counts) {
            ip_windows_mapping[ip].push_back(window);
        }
    }
    map<string, vector<int>> multiple_occurrence_ips;
    for (const auto& pair : ip_windows_mapping) {
        if (pair.second.size() > 1) {
            multiple_occurrence_ips.insert(pair);
        }
    }
    map<string, vector<int>> time_intervals;
    for (const auto& pair : multiple_occurrence_ips) {
        const string& ip = pair.first;
        const vector<int>& windows = pair.second;
        vector<int> intervals;
        for (size_t i = 1; i < windows.size(); ++i) {
            intervals.push_back(windows[i] - windows[i - 1]);
        }
        time_intervals[ip] = intervals;
    }
    std::cout << "Calculating the time interval is finished! " << endl;

    map<pair<string, int>, int> interval_counts;
    for (const auto& pair : time_intervals) {
        const string& ip = pair.first;
        for (int interval : pair.second) {
            interval_counts[make_pair(ip, interval)]++;
        }
    }
    std::cout << "Interval count is finished! " << endl;
    vector<pair<pair<string, int>, int>> sorted_intervals(interval_counts.begin(), interval_counts.end());
    std::sort(sorted_intervals.begin(), sorted_intervals.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    // Output the results
    ofstream top_k_file("./GroundTruth_Result_File/top-k_windows_interval_frequency.txt");
    ofstream top_k_gt1_file("./GroundTruth_Result_File/top-k_value_more_than_1_frequency.txt");
    for (size_t i = 0; i < sorted_intervals.size() && i < 200; ++i) {
        const auto& key = sorted_intervals[i].first;
        int count = sorted_intervals[i].second;
        top_k_file << "IP: " << key.first << ", Interval: " << key.second << ", Count: " << count << "\n";
        if (count > 1) {
            top_k_gt1_file << "IP: " << key.first << ", Interval: " << key.second << ", Count: " << count << "\n";
        }
    }
    top_k_file.close();
    top_k_gt1_file.close();
    std::cout << "Top output Finished!" << endl;

    map<pair<uint64_t, uint64_t>, uint64_t> data_dict;
    for (const auto& item : sorted_intervals) {
        const auto& key = item.first; 
        uint64_t value = static_cast<uint64_t>(item.second); 
        if (value > 1) {
            uint64_t newKeyFirstPart;
            newKeyFirstPart = stoull(key.first); 
            uint64_t newKeySecondPart = static_cast<uint64_t>(key.second);
            data_dict[{newKeyFirstPart, newKeySecondPart}] = value;
        }
    }
    std::cout << "Return is finished! " << endl;
    return data_dict;
}
#endif
