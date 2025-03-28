/**
 * @file ip2mac.cpp
 * @brief Implementation of class managing IP to MAC address mapping
 */

#include "ip2mac.hpp"
#include <cstring>
#include <ctime>
#include <algorithm>

/**
 * @brief Constructor
 * @param capacity Initial capacity of the IP2MAC table
 */
IP2MACManager::IP2MACManager(size_t capacity) {
    ip2mac_table.resize(capacity);
    for (auto& entry : ip2mac_table) {
        entry.flag = FLAG_FREE;
    }
}

/**
 * @brief Destructor
 */
IP2MACManager::~IP2MACManager() {
    // Vectors will automatically clean up
}

/**
 * @brief Search for an IP2MAC entry
 * @param deviceNo Device number
 * @param addr IP address
 * @param hwaddr MAC address (will be filled if found)
 * @return Pointer to IP2MAC entry or nullptr if not found
 */
IP2MAC* IP2MACManager::Search(int deviceNo, in_addr_t addr, unsigned char* hwaddr) {
    std::lock_guard<std::mutex> lock(mutex);

    for (auto& entry : ip2mac_table) {
        if (entry.flag != FLAG_FREE && entry.device_number == deviceNo && entry.ip_addr == addr) {
            if (hwaddr != nullptr) {
                memcpy(hwaddr, entry.hw_addr, 6);
            }
            return &entry;
        }
    }

    return nullptr;
}

/**
 * @brief Get or create an IP2MAC entry
 * @param deviceNo Device number
 * @param addr IP address
 * @param hwaddr MAC address
 * @return Pointer to IP2MAC entry
 */
IP2MAC* IP2MACManager::GetIp2Mac(int deviceNo, in_addr_t addr, unsigned char* hwaddr) {
    std::lock_guard<std::mutex> lock(mutex);

    // Look for existing entry
    for (auto& entry : ip2mac_table) {
        if (entry.flag != FLAG_FREE && entry.device_number == deviceNo && entry.ip_addr == addr) {
            entry.lastTime = time(nullptr);
            if (hwaddr != nullptr) {
                memcpy(entry.hw_addr, hwaddr, 6);
            }
            return &entry;
        }
    }

    // Find free entry
    for (auto& entry : ip2mac_table) {
        if (entry.flag == FLAG_FREE) {
            entry.flag = FLAG_OK;
            entry.device_number = deviceNo;
            entry.ip_addr = addr;
            entry.lastTime = time(nullptr);
            if (hwaddr != nullptr) {
                memcpy(entry.hw_addr, hwaddr, 6);
            }
            return &entry;
        }
    }

    // Table is full, replace oldest entry
    auto oldest = std::min_element(ip2mac_table.begin(), ip2mac_table.end(),
        [](const IP2MAC& a, const IP2MAC& b) { return a.lastTime < b.lastTime; });

    if (oldest != ip2mac_table.end()) {
        oldest->flag = FLAG_OK;
        oldest->device_number = deviceNo;
        oldest->ip_addr = addr;
        oldest->lastTime = time(nullptr);
        if (hwaddr != nullptr) {
            memcpy(oldest->hw_addr, hwaddr, 6);
        }
        return &(*oldest);
    }

    return nullptr;
}

/**
 * @brief Send one buffered packet for a specific IP2MAC entry
 * @param deviceNo Device number
 * @param ip2mac Pointer to IP2MAC entry
 * @return Success or failure code
 */
int IP2MACManager::BufferSendOne(int deviceNo, IP2MAC* ip2mac) {
    if (ip2mac == nullptr) {
        return -1;
    }

    // This method would need access to socket and network functions
    // Actual implementation depends on the router's network interface

    return 1;
}

/**
 * @brief Append send request data
 * @param deviceNo Device number
 * @param ip2macNo IP2MAC entry number
 * @return Success or failure code
 */
int IP2MACManager::AppendSendReqData(int deviceNo, int ip2macNo) {
    std::lock_guard<std::mutex> lock(send_req_mutex);

    SendReqData req;
    req.deviceNo = deviceNo;
    req.ip2macNo = ip2macNo;

    send_req_data.push_back(req);

    return 1;
}

/**
 * @brief Get send request data
 * @param deviceNo Pointer to store device number
 * @param ip2macNo Pointer to store IP2MAC entry number
 * @return Success or failure code
 */
int IP2MACManager::GetSendReqData(int* deviceNo, int* ip2macNo) {
    std::lock_guard<std::mutex> lock(send_req_mutex);

    if (send_req_data.empty()) {
        return -1;
    }

    SendReqData req = send_req_data.front();
    send_req_data.erase(send_req_data.begin());

    *deviceNo = req.deviceNo;
    *ip2macNo = req.ip2macNo;

    return 1;
}

/**
 * @brief Send all buffered data
 * @return Success or failure code
 */
int IP2MACManager::BufferSend() {
    // Process all pending send requests
    int deviceNo, ip2macNo;
    while (GetSendReqData(&deviceNo, &ip2macNo) == 1) {
        // Calculate index in the table
        size_t index = ip2macNo;
        if (index < ip2mac_table.size()) {
            BufferSendOne(deviceNo, &ip2mac_table[index]);
        }
    }

    return 1;
}