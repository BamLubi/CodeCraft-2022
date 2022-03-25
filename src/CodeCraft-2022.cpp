#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <string>
#include <unordered_map>
#include <cmath>
#include <algorithm>

using namespace std;

struct IdHash {
    size_t operator()(const string &s) const {
        return s.size() == 1 ? (size_t)s[0] : (size_t)s[1] << 8 | s[0];
    }
};

/**
 * @brief 带宽需求序列
 * 
 * @param id_map 客户节点ID下标映射
 * @param demand_value 请求队列向量
 */
typedef struct {
    unordered_map<string, size_t, IdHash> id_map;
    vector<vector<uint32_t>> demand_value;
} demand_table_t;

/**
 * @brief 边缘节点带宽上限
 */
typedef unordered_map<string, uint32_t, IdHash> bandwidth_table_t;

/**
 * @brief 网络时延QoS表
 * 
 * @param server_id_map 行ID下标映射
 * @param client_id_map 列ID下标映射
 * @param qos_value Qos
 */
typedef struct {
    unordered_map<string, size_t, IdHash> server_id_map;
    unordered_map<string, size_t, IdHash> client_id_map;
    vector<vector<uint32_t>> qos_value;
} qos_table_t;

/**
 * @brief 带宽调度分配表
 */
typedef unordered_map<string, bandwidth_table_t, IdHash> allocate_table_t;

/**
 * @brief 数据预处理
 * 
 * @param demand_table 客户节点带宽需求
 * @param bandwidth_table 边缘节点带宽上限
 * @param qos_table 客户节点与边缘节点的网络时延
 * @param qos_constraint QoS限制
 */
void data_loader(demand_table_t &demand_table,
                bandwidth_table_t &bandwidth_table,
                qos_table_t &qos_table,
                uint32_t &qos_constraint) {
    ifstream fs;
    string line_buffer;
    string element_buffer;
    stringstream ss;

    // 读取 demand.csv
    fs.open("data/demand.csv", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer);
    ss = stringstream(line_buffer);
    getline(ss, element_buffer, ','); // 忽略表名
    for (int i = 0; getline(ss, element_buffer, ','); ++i) {
        if (int pos = element_buffer.find('\r') != -1) element_buffer.erase(pos, 1);
        if (int pos = element_buffer.find('\n') != -1) element_buffer.erase(pos, 1);
        demand_table.id_map[element_buffer] = i;
    }
    while (getline(fs, line_buffer)) {
        vector<uint32_t> demand_line_buffer;
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ','); // 忽略时间戳
        while (getline(ss, element_buffer, ',')) {
            demand_line_buffer.emplace_back(stoi(element_buffer));
        }
        demand_table.demand_value.emplace_back(demand_line_buffer);
    }
    fs.close();

    // 读取 site_bandwidth.csv
    fs.open("data/site_bandwidth.csv", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer); // 忽略表头
    for (int i = 0; getline(fs, line_buffer); ++i) {
        string id_buffer;
        ss = stringstream(line_buffer);
        getline(ss, id_buffer, ',');
        if (int pos = id_buffer.find('\r') != -1) id_buffer.erase(pos, 1);
        if (int pos = id_buffer.find('\n') != -1) id_buffer.erase(pos, 1);
        getline(ss, element_buffer);
        bandwidth_table[id_buffer] = stoi(element_buffer);
    }
    fs.close();

    // 读取 qos.csv
    fs.open("data/qos.csv", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer);
    ss = stringstream(line_buffer);
    getline(ss, element_buffer, ','); // 忽略表名
    for (int i = 0; getline(ss, element_buffer, ','); ++i) {
        if (int pos = element_buffer.find('\r') != -1) element_buffer.erase(pos, 1);
        if (int pos = element_buffer.find('\n') != -1) element_buffer.erase(pos, 1);
        qos_table.client_id_map[element_buffer] = i;
    }
    for (int i = 0; getline(fs, line_buffer); ++i) {
        vector<uint32_t> qos_line_buffer;
        ss = stringstream(line_buffer);
        getline(ss, element_buffer, ',');
        qos_table.server_id_map[element_buffer] = i;
        while (getline(ss, element_buffer, ',')) {
            qos_line_buffer.emplace_back(stoi(element_buffer));
        }
        qos_table.qos_value.emplace_back(qos_line_buffer);
    }
    fs.close();

    // 读取 qos_constrain
    fs.open("data/config.ini", ios::in);
    if (!fs)
        exit(-1);
    getline(fs, line_buffer); // 忽略头部
    getline(fs, line_buffer);
    sscanf(line_buffer.c_str(), "qos_constraint=%u\n", &qos_constraint);
    fs.close();
}

/**
 * @brief 分配方案写入到文件
 * 
 * @param allocate_table 分配表
 */
void data_output(vector<allocate_table_t> &allocate_tables) {
    ofstream fs;
    fs.open("output/solution.txt", ios::out);
    if (!fs)
        exit(-1);
    for (int i = 0; i < allocate_tables.size(); ++i) {
        if (i > 0)
            fs << endl;
        for (auto& [client_id, allocate_list] : allocate_tables[i]) {
            if (client_id != allocate_tables[i].begin()->first)
                fs << endl;
            fs << client_id << ':';
            for (auto& [server_id, allocate_bandwidth] : allocate_list) {
                if (server_id != allocate_list.begin()->first)
                    fs << ',';
                fs << '<' << server_id << ',' << allocate_bandwidth << '>';
            }
        }
    }
    fs.close();
}

vector<string> get_valid_server(const string &client_id, qos_table_t &qos_table, uint32_t qos_constraint) {
    vector<string> ret;
    for (const auto [server_id, server_index] : qos_table.server_id_map) {
        if (qos_table.qos_value[server_index][qos_table.client_id_map[client_id]] < qos_constraint) {
            ret.emplace_back(server_id);
        }
    }
    return ret;
}

string get_match_server(vector<string> &server_id, bandwidth_table_t &server_bandwidth) {
    int idx = 0;
    int min = 0x7fffffff;
    for (int i = 0; i < server_id.size(); ++i) {
        if (min > server_bandwidth[server_id[i]]) {
            min = server_bandwidth[server_id[i]];
            idx = i;
        }
    }
    return server_id[idx];
}

allocate_table_t calculate_atime(demand_table_t &demand_table,
                                 uint32_t demand_index,
                                 qos_table_t &qos_table,
                                 uint32_t qos_constraint,
                                 bandwidth_table_t &bandwidth_table) {
    uint32_t slice = 100;
    allocate_table_t client_bandwidth;
    bandwidth_table_t server_bandwidth(bandwidth_table);
    bandwidth_table_t global_bandwidth;
    queue<pair<string, uint32_t>> demand_queue;
    unordered_map<string, vector<string>, IdHash> valid_server_list;

    for (const auto& [client_id, client_index] : demand_table.id_map) {
        auto client_demand = demand_table.demand_value[demand_index][client_index];
        client_bandwidth[client_id] = {};
        if (client_demand > 0) {
            demand_queue.push({client_id, client_demand});
            valid_server_list[client_id] = get_valid_server(client_id, qos_table, qos_constraint);
        }
    }
    while (!demand_queue.empty()) {
        auto front = demand_queue.front();
        demand_queue.pop();
        auto& valid_server = valid_server_list[front.first];
        auto match_server = get_match_server(valid_server, global_bandwidth);
        uint32_t s = min(slice, min(front.second, server_bandwidth[match_server]));
        client_bandwidth[front.first][match_server] += s;
        server_bandwidth[match_server] -= s;
        global_bandwidth[match_server] += s;
        front.second -= s;
        if (front.second > 0) demand_queue.push(front);
    }

    return client_bandwidth;
}

/**
 * @brief 计算分配表
 * 
 * @param demand_table 客户节点带宽需求
 * @param bandwidth_table 边缘节点带宽上限
 * @param qos_table 客户节点与边缘节点的网络时延
 * @param qos_constraint QoS限制
 * @param allocate_table 分配表
 */
vector<allocate_table_t> calculate(demand_table_t &demand_table,
                                   bandwidth_table_t &bandwidth_table,
                                   qos_table_t &qos_table,
                                   uint32_t qos_constraint) {
    vector<allocate_table_t> ret(demand_table.demand_value.size());
    for (int i = 0; i < demand_table.demand_value.size(); ++i) {
        int sum = 0;
        for (int j = 0; j < demand_table.demand_value[i].size(); ++j) {
            sum += demand_table.demand_value[i][j];
        }
        demand_table.demand_value[i].push_back(i);
        demand_table.demand_value[i].push_back(sum);
    }
    sort(demand_table.demand_value.begin(), demand_table.demand_value.end(), [](vector<uint32_t>& a, vector<uint32_t>& b){
        return a.back() > b.back();
    });
    for (int i = 0; i < demand_table.demand_value.size(); ++i) {
        int sum = demand_table.demand_value[i].back();
        demand_table.demand_value[i].pop_back();
        int idx = demand_table.demand_value[i].back();
        demand_table.demand_value[i].pop_back();
        ret[idx] = calculate_atime(demand_table, i, qos_table, qos_constraint, bandwidth_table);
    }
    return ret;
}

int main(int argc, char const *argv[]) {
    demand_table_t demand_table; // 客户节点带宽需求
    bandwidth_table_t bandwidth_table; // 边缘节点带宽上限
    qos_table_t qos_table; // 客户节点与边缘节点的网络时延
    uint32_t qos_constraint; // QoS限制
    vector<allocate_table_t> allocate_tables; // 分配表

    // 1. 数据预处理
    data_loader(demand_table, bandwidth_table, qos_table, qos_constraint);
    // 2. 计算分配表
    allocate_tables = calculate(demand_table, bandwidth_table, qos_table, qos_constraint);
    // 3. 输出
    data_output(allocate_tables);

    return 0;
}