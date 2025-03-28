/**
 * @file router.cpp
 * @brief Router class implementation - handles packet forwarding between interfaces
 */

#include "../../include/router.h"
#include "../../include/icmp.h"
#include <cstdarg>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

// Constructor
Router::Router(const RouterConfig& config)
    : config_(config), running_(false) {
    // Initialize interfaces vector with two interfaces
    interfaces_.resize(2);
}

// Destructor
Router::~Router() {
    stop();
}

// Initialize the router
bool Router::init() {
    debugPrintf("Initializing router...\n");

    // Parse next router IP address
    if (!parseIpAddress(config_.next_router, next_router_)) {
        debugPrintf("Failed to parse next router IP address: %s\n", config_.next_router.c_str());
        return false;
    }

    // Initialize receiving interface
    interfaces_[0].device = std::make_shared<RawSocketDevice>(config_.receiving_interface);
    if (!interfaces_[0].device->open()) {
        debugPerror("Failed to open receiving interface");
        return false;
    }

    // Initialize sending interface
    interfaces_[1].device = std::make_shared<RawSocketDevice>(config_.sending_interface);
    if (!interfaces_[1].device->open()) {
        debugPerror("Failed to open sending interface");
        interfaces_[0].device->close();
        return false;
    }

    // Create Ethernet layer for both interfaces
    for (int i = 0; i < 2; i++) {
        interfaces_[i].ethernet = std::make_shared<Ethernet>(interfaces_[i].device);
        if (!interfaces_[i].ethernet->init()) {
            debugPrintf("Failed to initialize Ethernet layer for interface %d\n", i);
            return false;
        }

        // Get IP address and network information for the interface
        // This would typically be retrieved from the system or configuration
        // For simplicity, we'll just provide placeholders here
        // In a real implementation, this would be determined from the actual interfaces

        // Example code to set interface IP and subnet information
        char ip_str[32];
        char subnet_str[32];

        if (i == 0) {
            // Example: Receiving interface on 192.168.1.0/24
            strcpy(ip_str, "192.168.1.1");
            strcpy(subnet_str, "255.255.255.0");
        } else {
            // Example: Sending interface on 192.168.2.0/24
            strcpy(ip_str, "192.168.2.1");
            strcpy(subnet_str, "255.255.255.0");
        }

        if (!parseIpAddress(ip_str, interfaces_[i].ip_address)) {
            debugPrintf("Failed to parse IP address for interface %d\n", i);
            return false;
        }

        if (!parseIpAddress(subnet_str, interfaces_[i].subnet_mask)) {
            debugPrintf("Failed to parse subnet mask for interface %d\n", i);
            return false;
        }

        // Calculate network address
        interfaces_[i].network = interfaces_[i].ip_address & interfaces_[i].subnet_mask;

        // Create IP layer for the interface
        interfaces_[i].ip = std::make_shared<IP>(
            interfaces_[i].ethernet,
            nullptr, // No ARP for now; real implementation would include ARP
            interfaces_[i].ip_address
        );

        if (!interfaces_[i].ip->init()) {
            debugPrintf("Failed to initialize IP layer for interface %d\n", i);
            return false;
        }

        // Register IP handler for router forwarding
        interfaces_[i].ethernet->registerHandler(
            EtherType::IPV4,
            [this, i](const uint8_t* data, size_t length,
                    const MacAddress& src_mac, const MacAddress& dst_mac) {
                // Create IP packet from buffer
                auto packet = IPPacket::fromBuffer(data, length);
                if (packet) {
                    handlePacket(i, packet);
                }
            }
        );
    }

    debugPrintf("Router initialized successfully\n");
    return true;
}

// Start the router
bool Router::start() {
    if (running_) {
        debugPrintf("Router is already running\n");
        return true;
    }

    running_ = true;
    router_thread_ = std::thread(&Router::routerThread, this);

    debugPrintf("Router started\n");
    return true;
}

// Stop the router
void Router::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (router_thread_.joinable()) {
        router_thread_.join();
    }

    debugPrintf("Router stopped\n");
}

// Router thread function
void Router::routerThread() {
    debugPrintf("Router thread started\n");

    while (running_) {
        // Process packets on both interfaces
        for (int i = 0; i < 2; i++) {
            interfaces_[i].ethernet->receiveFrames(100); // 100ms timeout
        }
    }

    debugPrintf("Router thread stopped\n");
}

// Handle packet from interface
void Router::handlePacket(int interface_index, const std::unique_ptr<IPPacket>& packet) {
    // Check TTL
    uint8_t ttl = packet->getTTL();
    if (ttl <= 1) {
        // TTL expired, send ICMP Time Exceeded
        // For this we would need the original packet and destination MAC
        // In a full implementation, these would be available from the context
        // For now, just log the event
        debugPrintf("TTL expired for packet on interface %d\n", interface_index);

        // Example of how to send ICMP Time Exceeded
        // sendIcmpTimeExceeded(interface_index, src_mac, packet->getSourceIP(), original_packet, original_packet_size);
        return;
    }

    // Decrement TTL and update checksum
    packet->setTTL(ttl - 1);
    packet->updateChecksum();

    // Forward packet to the appropriate interface
    if (!forwardPacket(packet, interface_index)) {
        debugPrintf("Failed to forward packet from interface %d\n", interface_index);
    }
}

// Send ICMP Time Exceeded message
void Router::sendIcmpTimeExceeded(
    int interface_index,
    const MacAddress& dst_mac,
    const IPv4Address& dst_ip,
    const uint8_t* original_packet,
    size_t packet_size
) {
    // Create ICMP Time Exceeded message
    auto icmp_message = ICMP::createTimeExceededMessage(
        IcmpTimeExceededCode::TTL_EXPIRED_IN_TRANSIT,
        original_packet,
        packet_size
    );

    // Send ICMP message via IP layer
    IPSendOptions options;
    options.ttl = IP_DEFAULT_TTL;

    interfaces_[interface_index].ip->sendPacket(
        dst_ip,
        IPProtocol::ICMP,
        icmp_message.data(),
        icmp_message.size(),
        options
    );
}

// Forward packet to the appropriate interface
bool Router::forwardPacket(const std::unique_ptr<IPPacket>& packet, int source_interface) {
    IPv4Address dst_ip = packet->getDestinationIP();

    // Determine destination interface
    int dest_interface = -1;

    // Check if destination is on a directly connected network
    for (int i = 0; i < 2; i++) {
        if (i != source_interface) { // Don't send back to source interface
            if ((dst_ip & interfaces_[i].subnet_mask) == interfaces_[i].network) {
                dest_interface = i;
                break;
            }
        }
    }

    // If not on a directly connected network, forward to next hop
    if (dest_interface == -1) {
        // Forward to next router
        // For a real implementation, we would have routing tables
        // For simplicity, we'll just use the configured next router
        dest_interface = (source_interface == 0) ? 1 : 0;
    }

    // Serialize the packet
    std::vector<uint8_t> buffer(packet->getTotalSize());
    packet->serialize(buffer.data(), buffer.size());

    // Forward the packet
    return interfaces_[dest_interface].ip->sendPacket(
        dst_ip,
        packet->getProtocol(),
        buffer.data() + packet->getHeaderSize(), // Skip IP header as IP layer will add its own
        buffer.size() - packet->getHeaderSize(),
        IPSendOptions{} // Default options
    );
}

// Debug output functions
void Router::debugPrintf(const char* format, ...) {
    if (!config_.debug_output) {
        return;
    }

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void Router::debugPerror(const char* msg) {
    if (!config_.debug_output) {
        return;
    }

    std::cerr << msg << ": " << strerror(errno) << std::endl;
}