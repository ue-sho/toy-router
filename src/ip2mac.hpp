/**
 * @file ip2mac.hpp
 * @brief Header file for class managing IP to MAC address mapping
 */

#ifndef IP2MAC_HPP
#define IP2MAC_HPP

#include <netinet/in.h>
#include <vector>
#include <mutex>
#include "base.hpp"

/**
 * @brief Class for managing IP to MAC address mapping
 */
class IP2MACManager {
public:
    /**
     * @brief Constructor
     * @param capacity Initial capacity of the IP2MAC table
     */
    IP2MACManager(size_t capacity = 4096);

    /**
     * @brief Destructor
     */
    ~IP2MACManager();

    /**
     * @brief Search for an IP2MAC entry
     * @param deviceNo Device number
     * @param addr IP address
     * @param hwaddr MAC address (will be filled if found)
     * @return Pointer to IP2MAC entry or nullptr if not found
     */
    IP2MAC* Search(int deviceNo, in_addr_t addr, unsigned char* hwaddr);

    /**
     * @brief Get or create an IP2MAC entry
     * @param deviceNo Device number
     * @param addr IP address
     * @param hwaddr MAC address
     * @return Pointer to IP2MAC entry
     */
    IP2MAC* GetIp2Mac(int deviceNo, in_addr_t addr, unsigned char* hwaddr);

    /**
     * @brief Send one buffered packet for a specific IP2MAC entry
     * @param deviceNo Device number
     * @param ip2mac Pointer to IP2MAC entry
     * @return Success or failure code
     */
    int BufferSendOne(int deviceNo, IP2MAC* ip2mac);

    /**
     * @brief Append send request data
     * @param deviceNo Device number
     * @param ip2macNo IP2MAC entry number
     * @return Success or failure code
     */
    int AppendSendReqData(int deviceNo, int ip2macNo);

    /**
     * @brief Get send request data
     * @param deviceNo Pointer to store device number
     * @param ip2macNo Pointer to store IP2MAC entry number
     * @return Success or failure code
     */
    int GetSendReqData(int* deviceNo, int* ip2macNo);

    /**
     * @brief Send all buffered data
     * @return Success or failure code
     */
    int BufferSend();

private:
    std::vector<IP2MAC> ip2mac_table;    // Table of IP2MAC entries
    std::mutex mutex;                    // Mutex for thread safety

    struct SendReqData {
        int deviceNo;
        int ip2macNo;
    };

    std::vector<SendReqData> send_req_data;  // Send request queue
    std::mutex send_req_mutex;              // Mutex for send request queue
};

#endif // IP2MAC_HPP