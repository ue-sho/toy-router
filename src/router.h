/**
 * @file router.h
 * @brief Router class implementation - handles packet forwarding between interfaces
 */

#ifndef ROUTER_H
#define ROUTER_H

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include "../src/ip/ip.h"
#include "../src/ethernet/ethernet.h"
#include "../src/device/network_device.h"

// Router configuration
struct RouterConfig {
    std::string receiving_interface; // Receiving interface name
    std::string sending_interface;   // Sending interface name
    bool debug_output;               // Debug output flag
    std::string next_router;         // Next hop router IP address
};

// Router interface information
struct RouterInterface {
    std::shared_ptr<NetworkDevice> device;
    std::shared_ptr<Ethernet> ethernet;
    std::shared_ptr<IP> ip;
    IPv4Address ip_address;
    IPv4Address subnet_mask;
    IPv4Address network;
};

class Router {
public:
    // Constructor
    Router(const RouterConfig& config);

    // Destructor
    ~Router();

    // Initialize the router
    bool init();

    // Start the router
    bool start();

    // Stop the router
    void stop();

    // Debug output functions
    void debugPrintf(const char* format, ...);
    void debugPerror(const char* msg);

private:
    // Router configuration
    RouterConfig config_;

    // Router interfaces
    std::vector<RouterInterface> interfaces_;

    // Next hop router address
    IPv4Address next_router_;

    // Running flag
    std::atomic<bool> running_;

    // Router thread
    std::thread router_thread_;

    // Router thread function
    void routerThread();

    // Handle packet from interface
    void handlePacket(int interface_index, const std::unique_ptr<IPPacket>& packet);

    // Send ICMP Time Exceeded message
    void sendIcmpTimeExceeded(int interface_index, const MacAddress& dst_mac,
                            const IPv4Address& dst_ip, const uint8_t* original_packet,
                            size_t packet_size);

    // Forward packet to the appropriate interface
    bool forwardPacket(const std::unique_ptr<IPPacket>& packet, int source_interface);
};

#endif // ROUTER_H