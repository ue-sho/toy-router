/**
 * @file router.hpp
 * @brief Header file for Router class
 */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>
#include <string>
#include <poll.h>
#include <thread>
#include <atomic>
#include <unistd.h>  // For close(), read(), write()
#include "base.hpp"
#include "send_buf.hpp"
#include "ip2mac.hpp"
#include "netutil.hpp"

/**
 * @brief Router configuration class
 */
class RouterConfig {
public:
    std::string receiving_interface;   // Receiving interface name
    std::string sending_interface;     // Sending interface name
    bool debug_out;                    // Debug output flag
    std::string next_router;           // Next hop router IP

    /**
     * @brief Constructor
     */
    RouterConfig();
};

/**
 * @brief Router class
 */
class Router {
public:
    /**
     * @brief Constructor
     * @param config Router configuration
     */
    Router(const RouterConfig& config);

    /**
     * @brief Destructor
     */
    ~Router();

    /**
     * @brief Initialize router
     * @return Success or failure code
     */
    int Initialize();

    /**
     * @brief Run router
     * @return Success or failure code
     */
    int Run();

    /**
     * @brief Stop router
     */
    void Stop();

private:
    RouterConfig config;                 // Router configuration
    InterfaceInfo interface_info[2];     // Interface information
    struct in_addr next_router;          // Next hop router IP address
    std::atomic<bool> running;           // Running flag
    std::thread process_thread;          // Processing thread
    IP2MACManager ip2mac_manager;        // IP to MAC address manager
    SendBuf send_buffer;                 // Send buffer

    /**
     * @brief Process router function
     */
    void ProcessRouter();

    /**
     * @brief Debug print function
     * @param fmt Format string
     * @param ... Variable arguments
     * @return Success or failure code
     */
    int DebugPrintf(const char* fmt, ...);

    /**
     * @brief Debug perror function
     * @param msg Error message
     * @return Success or failure code
     */
    int DebugPerror(const char* msg);

    /**
     * @brief Send ICMP Time Exceeded message
     * @param device_number Device number
     * @param eth_hdr Ethernet header
     * @param ip_hdr IP header
     * @param data Data buffer
     * @param size Data size
     * @return Success or failure code
     */
    int SendIcmpTimeExceeded(int device_number, struct ether_header* eth_hdr,
                             struct iphdr* ip_hdr, u_char* data, int size);

    /**
     * @brief Analyze packet
     * @param device_number Device number
     * @param data Data buffer
     * @param size Data size
     * @return Success or failure code
     */
    int AnalyzePacket(int device_number, u_char* data, int size);

    /**
     * @brief Disable IP forwarding
     * @return Success or failure code
     */
    int DisableIpForward();
};

#endif // ROUTER_HPP