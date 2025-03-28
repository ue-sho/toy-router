/**
 * @file base.hpp
 * @brief Basic structure definitions for router implementation
 */

#ifndef BASE_HPP
#define BASE_HPP

#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <vector>
#include <memory>
#include <mutex>

/**
 * @brief Network interface information
 */
class InterfaceInfo {
public:
    int socket_descriptor;    // Socket descriptor
    u_char hw_addr[6];        // MAC address
    struct in_addr ip_addr;   // IP address
    struct in_addr subnet;    // Subnet mask
    struct in_addr netmask;   // Net mask
};

#define FLAG_FREE 0
#define FLAG_OK 1
#define FLAG_NG -1

/**
 * @brief Class to hold data buffer
 */
class DataBuf {
public:
    DataBuf* next;           // Pointer to the next data
    DataBuf* before;         // Pointer to the previous data
    time_t time;             // Creation time
    int size;                // Data size
    unsigned char* data;     // Data

    DataBuf();
    ~DataBuf();
};

/**
 * @brief Class to manage send data
 */
class SendData {
public:
    DataBuf* top;                   // Pointer to the first data
    DataBuf* bottom;                // Pointer to the last data
    unsigned long data_num;         // Number of data entries
    unsigned long in_bucket_size;   // Total data size

    SendData();
    ~SendData();

private:
    std::mutex mutex;              // Mutex for thread safety
};

/**
 * @brief Class to manage IP to MAC address relation
 */
class IP2MAC {
public:
    int flag;                    // Flag indicating if entry is free
    int device_number;           // Device number
    in_addr_t ip_addr;           // IP address
    unsigned char hw_addr[6];    // MAC address
    time_t lastTime;             // Last data creation time
    SendData send_data;          // Send data

    IP2MAC();
    ~IP2MAC();
};

#endif // BASE_HPP