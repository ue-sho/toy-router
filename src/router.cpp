/**
 * @file router.cpp
 * @brief Implementation of Router class
 */

#include "router.hpp"
#include <arpa/inet.h>
#include <cstdarg>
#include <cstring>
#include <iostream>
#include <fstream>
#include <csignal>
#include <cerrno>  // For errno

// ICMP time exceeded in transit
#ifndef ICMP_TIME_EXCEEDED
#define ICMP_TIME_EXCEEDED ICMP_TIMXCEED
#endif

/**
 * @brief RouterConfig constructor
 */
RouterConfig::RouterConfig()
    : receiving_interface("enp0s8"),
      sending_interface("enp0s9"),
      debug_out(true),
      next_router("169.254.238.208") {
}

/**
 * @brief Router constructor
 * @param config Router configuration
 */
Router::Router(const RouterConfig& config)
    : config(config), running(false) {
    memset(interface_info, 0, sizeof(interface_info));
}

/**
 * @brief Router destructor
 */
Router::~Router() {
    Stop();
    // Close sockets
    for (int i = 0; i < 2; i++) {
        if (interface_info[i].socket_descriptor > 0) {
            close(interface_info[i].socket_descriptor);
        }
    }
}

/**
 * @brief Debug print function
 * @param fmt Format string
 * @param ... Variable arguments
 * @return Success or failure code
 */
int Router::DebugPrintf(const char* fmt, ...) {
    if (config.debug_out) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
    return 0;
}

/**
 * @brief Debug perror function
 * @param msg Error message
 * @return Success or failure code
 */
int Router::DebugPerror(const char* msg) {
    if (config.debug_out) {
        perror(msg);
    }
    return 0;
}

/**
 * @brief Initialize router
 * @return Success or failure code
 */
int Router::Initialize() {
    // Parse next router IP
    if (inet_aton(config.next_router.c_str(), &next_router) == 0) {
        DebugPrintf("inet_aton:%s\n", config.next_router.c_str());
        return -1;
    }

    // Disable IP forwarding
    DisableIpForward();

    // Initialize interfaces
    interface_info[0].socket_descriptor = NetworkUtil::InitRawSocket(config.receiving_interface.c_str(), 1, 0);
    if (interface_info[0].socket_descriptor < 0) {
        DebugPerror("InitRawSocket");
        return -1;
    }

    interface_info[1].socket_descriptor = NetworkUtil::InitRawSocket(config.sending_interface.c_str(), 1, 0);
    if (interface_info[1].socket_descriptor < 0) {
        DebugPerror("InitRawSocket");
        close(interface_info[0].socket_descriptor);
        return -1;
    }

    // Get device information
    if (NetworkUtil::GetDeviceInfo(config.receiving_interface.c_str(),
                                  interface_info[0].hw_addr,
                                  &interface_info[0].ip_addr,
                                  &interface_info[0].subnet,
                                  &interface_info[0].netmask) < 0) {
        DebugPerror("GetDeviceInfo");
        close(interface_info[0].socket_descriptor);
        close(interface_info[1].socket_descriptor);
        return -1;
    }

    if (NetworkUtil::GetDeviceInfo(config.sending_interface.c_str(),
                                  interface_info[1].hw_addr,
                                  &interface_info[1].ip_addr,
                                  &interface_info[1].subnet,
                                  &interface_info[1].netmask) < 0) {
        DebugPerror("GetDeviceInfo");
        close(interface_info[0].socket_descriptor);
        close(interface_info[1].socket_descriptor);
        return -1;
    }

    // Print interface information
    DebugPrintf("[0] %s: %s\n", config.receiving_interface.c_str(),
                NetworkUtil::EtherToString(interface_info[0].hw_addr).c_str());
    DebugPrintf("[0] %s: %s\n", config.receiving_interface.c_str(),
                NetworkUtil::InetToString(&interface_info[0].ip_addr).c_str());
    DebugPrintf("[0] %s: %s\n", config.receiving_interface.c_str(),
                NetworkUtil::InetToString(&interface_info[0].subnet).c_str());
    DebugPrintf("[0] %s: %s\n", config.receiving_interface.c_str(),
                NetworkUtil::InetToString(&interface_info[0].netmask).c_str());

    DebugPrintf("[1] %s: %s\n", config.sending_interface.c_str(),
                NetworkUtil::EtherToString(interface_info[1].hw_addr).c_str());
    DebugPrintf("[1] %s: %s\n", config.sending_interface.c_str(),
                NetworkUtil::InetToString(&interface_info[1].ip_addr).c_str());
    DebugPrintf("[1] %s: %s\n", config.sending_interface.c_str(),
                NetworkUtil::InetToString(&interface_info[1].subnet).c_str());
    DebugPrintf("[1] %s: %s\n", config.sending_interface.c_str(),
                NetworkUtil::InetToString(&interface_info[1].netmask).c_str());

    return 0;
}

/**
 * @brief Send ICMP Time Exceeded message
 * @param device_number Device number
 * @param eth_hdr Ethernet header
 * @param ip_hdr IP header
 * @param data Data buffer
 * @param size Data size
 * @return Success or failure code
 */
int Router::SendIcmpTimeExceeded(int device_number, struct ether_header* eth_hdr,
                               struct iphdr* ip_hdr, u_char* data, int size) {
    (void)size; // Suppress unused parameter warning

    struct ether_header recieve_eth_hdr;
    memcpy(recieve_eth_hdr.ether_dhost, eth_hdr->ether_shost, 6);
    memcpy(recieve_eth_hdr.ether_shost, interface_info[device_number].hw_addr, 6);
    recieve_eth_hdr.ether_type = htons(ETHERTYPE_IP);

    struct iphdr recieve_ip_hdr;
    recieve_ip_hdr.version = 4;
    recieve_ip_hdr.ihl = 20 / 4;
    recieve_ip_hdr.tos = 0;
    recieve_ip_hdr.tot_len = htons(sizeof(struct icmp) + 64);
    recieve_ip_hdr.id = 0;
    recieve_ip_hdr.frag_off = 0;
    recieve_ip_hdr.ttl = 64;
    recieve_ip_hdr.protocol = IPPROTO_ICMP;
    recieve_ip_hdr.check = 0;
    recieve_ip_hdr.saddr = interface_info[device_number].ip_addr.s_addr;
    recieve_ip_hdr.daddr = ip_hdr->saddr;

    recieve_ip_hdr.check = NetworkUtil::Checksum((u_char*)&recieve_ip_hdr, sizeof(struct iphdr));

    struct icmp icmp_hdr;
    icmp_hdr.icmp_type = ICMP_TIME_EXCEEDED;
    icmp_hdr.icmp_code = ICMP_TIMXCEED_INTRANS;
    icmp_hdr.icmp_cksum = 0;
    icmp_hdr.icmp_void = 0;

    u_char* ip_ptr = data + sizeof(struct ether_header);

    icmp_hdr.icmp_cksum = NetworkUtil::Checksum2((u_char*)&icmp_hdr, 8, ip_ptr, 64);

    u_char buf[1500] = {'\0'};
    u_char* tmp_ptr = buf;
    memcpy(tmp_ptr, &recieve_eth_hdr, sizeof(struct ether_header));
    tmp_ptr += sizeof(struct ether_header);
    memcpy(tmp_ptr, &recieve_ip_hdr, sizeof(struct iphdr));
    tmp_ptr += sizeof(struct iphdr);
    memcpy(tmp_ptr, &icmp_hdr, 8);
    tmp_ptr += 8;
    memcpy(tmp_ptr, ip_ptr, 64);
    tmp_ptr += 64;
    int len = tmp_ptr - buf;  // ptrのずれ=大きさ

    DebugPrintf("write:SendIcmpTimeExceeded:[%d] %dbytes\n", device_number, len);
    write(interface_info[device_number].socket_descriptor, buf, len);

    return 0;
}

/**
 * @brief Analyze packet
 * @param device_number Device number
 * @param data Data buffer
 * @param size Data size
 * @return Success or failure code
 */
int Router::AnalyzePacket(int device_number, u_char* data, int size) {
    u_char* tmp_ptr = data;
    int tmp_len = size;

    // Ethernet header
    if (tmp_len < static_cast<int>(sizeof(struct ether_header))) {
        DebugPrintf("[%d]:tmp_len(%d) < sizeof(struct ether_header)\n", device_number, tmp_len);
        return -1;
    }
    struct ether_header* eth_hdr = (struct ether_header*)tmp_ptr;
    tmp_ptr += sizeof(struct ether_header);
    tmp_len -= sizeof(struct ether_header);

    // Check if destination MAC address matches our interface
    if (memcmp(&eth_hdr->ether_dhost, interface_info[device_number].hw_addr, 6) != 0) {
        DebugPrintf("[%d]:dhost not match %s\n", device_number,
                   NetworkUtil::EtherToString(eth_hdr->ether_dhost).c_str());
        return -1;
    }

    // ARP header
    if (ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP) {
        if (tmp_len < static_cast<int>(sizeof(struct ether_arp))) {
            DebugPrintf("[%d]:tmp_len(%d) < sizeof(struct ether_arp)\n", device_number, tmp_len);
            return -1;
        }
        struct ether_arp* arp_hdr = (struct ether_arp*)tmp_ptr;
        tmp_ptr += sizeof(struct ether_arp);
        tmp_len -= sizeof(struct ether_arp);

        if (arp_hdr->arp_op == htons(ARPOP_REQUEST)) {
            DebugPrintf("[%d]recv:ARP REQUEST:%dbytes\n", device_number, size);
            ip2mac_manager.GetIp2Mac(device_number, *(in_addr_t*)arp_hdr->arp_spa, arp_hdr->arp_sha);
        }
        if (arp_hdr->arp_op == htons(ARPOP_REPLY)) {
            DebugPrintf("[%d]recv:ARP REPLY:%dbytes\n", device_number, size);
            ip2mac_manager.GetIp2Mac(device_number, *(in_addr_t*)arp_hdr->arp_spa, arp_hdr->arp_sha);
        }
    }
    // IP header
    else if (ntohs(eth_hdr->ether_type) == ETHERTYPE_IP) {
        if (tmp_len < static_cast<int>(sizeof(struct iphdr))) {
            DebugPrintf("[%d]:tmp_len(%d) < sizeof(struct iphdr)\n", device_number, tmp_len);
            return -1;
        }
        struct iphdr* ip_hdr = (struct iphdr*)tmp_ptr;
        tmp_ptr += sizeof(struct iphdr);
        tmp_len -= sizeof(struct iphdr);

        u_char option[1500] = {'\0'};
        int option_len = ip_hdr->ihl * 4 - sizeof(struct iphdr);
        if (option_len > 0) {
            if (option_len >= 1500) {
                DebugPrintf("[%d]:IP option_len(%d):too big\n", device_number, option_len);
                return -1;
            }
            memcpy(option, tmp_ptr, option_len);
            tmp_ptr += option_len;
            tmp_len -= option_len;
        }

        if (ip_hdr->ttl <= 1) {
            DebugPrintf("[%d]:TTL <= 1\n", device_number);
            SendIcmpTimeExceeded(device_number, eth_hdr, ip_hdr, data, size);
            return -1;
        }

        // Check if the destination IP is our interface
        if (ip_hdr->daddr == interface_info[0].ip_addr.s_addr ||
            ip_hdr->daddr == interface_info[1].ip_addr.s_addr) {
            DebugPrintf("[%d]:recv:myaddr\n", device_number);
            return -1;
        }

        // Check if the packet is from external network
        int target_device = -1;
        if ((ip_hdr->daddr & interface_info[0].netmask.s_addr) == interface_info[0].subnet.s_addr) {
            target_device = 0;
        } else if ((ip_hdr->daddr & interface_info[1].netmask.s_addr) == interface_info[1].subnet.s_addr) {
            target_device = 1;
        } else {
            target_device = 1;
        }

        // Forward packet
        struct ether_header forward_eth_hdr;
        memcpy(forward_eth_hdr.ether_shost, interface_info[target_device].hw_addr, 6);
        forward_eth_hdr.ether_type = htons(ETHERTYPE_IP);

        // Decrement TTL
        ip_hdr->ttl--;
        ip_hdr->check = 0;
        ip_hdr->check = NetworkUtil::Checksum2((u_char*)ip_hdr, sizeof(struct iphdr), option, option_len);

        // Create forwarding packet
        u_char forward_buf[1500] = {'\0'};
        u_char* forward_ptr = forward_buf;
        memcpy(forward_ptr, &forward_eth_hdr, sizeof(struct ether_header));
        forward_ptr += sizeof(struct ether_header);
        memcpy(forward_ptr, ip_hdr, sizeof(struct iphdr));
        forward_ptr += sizeof(struct iphdr);
        if (option_len > 0) {
            memcpy(forward_ptr, option, option_len);
            forward_ptr += option_len;
        }
        memcpy(forward_ptr, tmp_ptr, tmp_len);
        forward_ptr += tmp_len;
        int forward_len = forward_ptr - forward_buf;

        // Get next hop IP
        in_addr_t next_hop;
        if (target_device == 0) {
            next_hop = ip_hdr->daddr;
        } else {
            next_hop = next_router.s_addr;
        }

        // Send packet
        IP2MAC* ip2mac = ip2mac_manager.GetIp2Mac(target_device, next_hop, nullptr);
        if (ip2mac == nullptr) {
            DebugPrintf("[%d]:ip2mac:error\n", device_number);
            return -1;
        }

        if (ip2mac->flag == FLAG_NG) {
            DebugPrintf("[%d]:ip2mac:error\n", device_number);
            return -1;
        } else if (ip2mac->flag == FLAG_OK) {
            memcpy(forward_eth_hdr.ether_dhost, ip2mac->hw_addr, 6);
            memcpy(forward_buf, &forward_eth_hdr, sizeof(struct ether_header));
            DebugPrintf("write:[%d] %dbytes\n", target_device, forward_len);
            write(interface_info[target_device].socket_descriptor, forward_buf, forward_len);
            return 0;
        } else {
            send_buffer.AppendSendData(ip2mac, target_device, next_hop, forward_buf, forward_len);
            if (ip2mac->flag == FLAG_FREE) {
                ip2mac->flag = FLAG_OK;
                NetworkUtil::SendArpRequest(interface_info[target_device].socket_descriptor,
                                         next_hop, nullptr,
                                         interface_info[target_device].ip_addr.s_addr,
                                         interface_info[target_device].hw_addr);
            }
        }
    }

    return 0;
}

/**
 * @brief Disable IP forwarding
 * @return Success or failure code
 */
int Router::DisableIpForward() {
    std::ofstream ofs("/proc/sys/net/ipv4/ip_forward");
    if (!ofs) {
        DebugPerror("cannot write /proc/sys/net/ipv4/ip_forward");
        return -1;
    }
    ofs << "0" << std::endl;
    ofs.close();

    return 0;
}

/**
 * @brief Process router function
 */
void Router::ProcessRouter() {
    struct pollfd targets[2];

    targets[0].fd = interface_info[0].socket_descriptor;
    targets[0].events = POLLIN | POLLERR;
    targets[1].fd = interface_info[1].socket_descriptor;
    targets[1].events = POLLIN | POLLERR;

    while (running) {
        int ready = poll(targets, 2, 1000);
        if (ready == -1) {
            if (errno == EINTR) {
                continue;
            }
            DebugPerror("poll");
            break;
        }

        if (ready == 0) {
            continue;
        }

        // Check for data on interfaces
        for (int i = 0; i < 2; i++) {
            if (targets[i].revents & (POLLIN | POLLERR)) {
                u_char buf[2048];
                int size = read(targets[i].fd, buf, sizeof(buf));
                if (size < 0) {
                    DebugPerror("read");
                } else if (size > 0) {
                    AnalyzePacket(i, buf, size);
                }
            }
        }

        // Process send buffer
        ip2mac_manager.BufferSend();
    }
}

/**
 * @brief Run router
 * @return Success or failure code
 */
int Router::Run() {
    running = true;
    process_thread = std::thread(&Router::ProcessRouter, this);
    return 0;
}

/**
 * @brief Stop router
 */
void Router::Stop() {
    running = false;
    if (process_thread.joinable()) {
        process_thread.join();
    }
}