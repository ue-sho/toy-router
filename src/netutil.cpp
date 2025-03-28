/**
 * @file netutil.cpp
 * @brief Implementation of network utility functions
 */

#include "netutil.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netpacket/packet.h>
#include <iostream>

/**
 * @brief Constructor
 */
NetworkUtil::NetworkUtil() {
}

/**
 * @brief Destructor
 */
NetworkUtil::~NetworkUtil() {
}

/**
 * @brief Convert MAC address to string
 * @param hwaddr MAC address
 * @return String representation of MAC address
 */
std::string NetworkUtil::EtherToString(const u_char* hwaddr) {
    char buf[50];
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
             hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
    return std::string(buf);
}

/**
 * @brief Convert IP address to string
 * @param addr IP address
 * @return String representation of IP address
 */
std::string NetworkUtil::InetToString(const struct in_addr* addr) {
    char buf[50];
    inet_ntop(AF_INET, addr, buf, sizeof(buf));
    return std::string(buf);
}

/**
 * @brief Convert in_addr_t to string
 * @param addr IP address
 * @return String representation of IP address
 */
std::string NetworkUtil::InAddrToString(in_addr_t addr) {
    struct in_addr in;
    in.s_addr = addr;
    return InetToString(&in);
}

/**
 * @brief Get device information
 * @param device Device name
 * @param hwaddr MAC address buffer
 * @param uaddr IP address buffer
 * @param subnet Subnet buffer
 * @param mask Netmask buffer
 * @return Success or failure code
 */
int NetworkUtil::GetDeviceInfo(const std::string& device, u_char hwaddr[6],
                              struct in_addr* uaddr, struct in_addr* subnet,
                              struct in_addr* mask) {
    int soc;
    struct ifreq ifr;

    if ((soc = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // Get hardware address
    strncpy(ifr.ifr_name, device.c_str(), sizeof(ifr.ifr_name) - 1);
    if (ioctl(soc, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl:hwaddr");
        close(soc);
        return -1;
    }
    memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, 6);

    // Get IP address
    if (ioctl(soc, SIOCGIFADDR, &ifr) < 0) {
        perror("ioctl:addr");
        close(soc);
        return -1;
    }
    memcpy(uaddr, &((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr, sizeof(struct in_addr));

    // Get subnet mask
    if (ioctl(soc, SIOCGIFNETMASK, &ifr) < 0) {
        perror("ioctl:mask");
        close(soc);
        return -1;
    }
    memcpy(mask, &((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr, sizeof(struct in_addr));

    // Calculate subnet
    subnet->s_addr = uaddr->s_addr & mask->s_addr;

    close(soc);
    return 1;
}

/**
 * @brief Print Ethernet header
 * @param eh Ethernet header
 * @return Success or failure code
 */
int NetworkUtil::PrintEtherHeader(struct ether_header* eh) {
    std::cout << "ether_header----------------------------" << std::endl;
    std::cout << "ether_dhost = " << EtherToString(eh->ether_dhost) << std::endl;
    std::cout << "ether_shost = " << EtherToString(eh->ether_shost) << std::endl;
    std::cout << "ether_type = " << std::hex << ntohs(eh->ether_type) << std::dec << std::endl;

    return 1;
}

/**
 * @brief Initialize raw socket
 * @param device Device name
 * @param promiscFlag Whether to enable promiscuous mode
 * @param ipOnly Whether to only capture IP packets
 * @return Socket descriptor or -1 on error
 */
int NetworkUtil::InitRawSocket(const std::string& device, int promiscFlag, int ipOnly) {
    struct ifreq ifr;
    struct sockaddr_ll sa;
    int soc;

    if (ipOnly) {
        if ((soc = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
            perror("socket");
            return -1;
        }
    } else {
        if ((soc = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
            perror("socket");
            return -1;
        }
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, device.c_str(), sizeof(ifr.ifr_name) - 1);
    if (ioctl(soc, SIOCGIFINDEX, &ifr) < 0) {
        perror("ioctl:SIOCGIFINDEX");
        close(soc);
        return -1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.sll_family = PF_PACKET;
    if (ipOnly) {
        sa.sll_protocol = htons(ETH_P_IP);
    } else {
        sa.sll_protocol = htons(ETH_P_ALL);
    }
    sa.sll_ifindex = ifr.ifr_ifindex;

    if (bind(soc, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        perror("bind");
        close(soc);
        return -1;
    }

    if (promiscFlag) {
        if (ioctl(soc, SIOCGIFFLAGS, &ifr) < 0) {
            perror("ioctl:SIOCGIFFLAGS");
            close(soc);
            return -1;
        }
        ifr.ifr_flags = ifr.ifr_flags | IFF_PROMISC;
        if (ioctl(soc, SIOCSIFFLAGS, &ifr) < 0) {
            perror("ioctl:SIOCSIFFLAGS");
            close(soc);
            return -1;
        }
    }

    return soc;
}

/**
 * @brief Calculate checksum
 * @param data Data to calculate checksum for
 * @param len Length of data
 * @return Checksum value
 */
u_int16_t NetworkUtil::Checksum(unsigned char* data, int len) {
    register u_int32_t sum = 0;

    for (int i = 0; i < len; i += 2) {
        if (i + 1 < len) {
            sum += data[i] + (data[i + 1] << 8);
        } else {
            sum += data[i];
        }
    }

    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

/**
 * @brief Calculate checksum for two data blocks
 * @param data1 First data block
 * @param len1 Length of first data block
 * @param data2 Second data block
 * @param len2 Length of second data block
 * @return Checksum value
 */
u_int16_t NetworkUtil::Checksum2(unsigned char* data1, int len1, unsigned char* data2, int len2) {
    register u_int32_t sum = 0;

    for (int i = 0; i < len1; i += 2) {
        if (i + 1 < len1) {
            sum += data1[i] + (data1[i + 1] << 8);
        } else {
            sum += data1[i];
        }
    }

    for (int i = 0; i < len2; i += 2) {
        if (i + 1 < len2) {
            sum += data2[i] + (data2[i + 1] << 8);
        } else {
            sum += data2[i];
        }
    }

    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

/**
 * @brief Check IP header checksum
 * @param iphdr IP header
 * @param option IP options
 * @param optionLen IP options length
 * @return 1 if checksum is valid, 0 otherwise
 */
int NetworkUtil::CheckIPChecksum(struct iphdr* iphdr, unsigned char* option, int optionLen) {
    unsigned char buf[BUFSIZ];
    u_int16_t sum;

    memset(buf, 0, sizeof(buf));
    memcpy(buf, iphdr, sizeof(struct iphdr));
    buf[10] = 0;
    buf[11] = 0;

    if (optionLen > 0) {
        memcpy(buf + sizeof(struct iphdr), option, optionLen);
    }

    sum = Checksum(buf, sizeof(struct iphdr) + optionLen);
    if (sum == 0 || sum == 0xFFFF) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief Send ARP request
 * @param soc Socket descriptor
 * @param target_ip Target IP address
 * @param target_mac Target MAC address buffer
 * @param my_ip My IP address
 * @param my_mac My MAC address
 * @return Success or failure code
 */
int NetworkUtil::SendArpRequest(int soc, in_addr_t target_ip, unsigned char target_mac[6],
                               in_addr_t my_ip, unsigned char my_mac[6]) {
    struct ether_header eh;
    struct ether_arp arp;
    u_char buf[sizeof(struct ether_header) + sizeof(struct ether_arp)];
    u_char *p = buf;

    // Set broadcast address
    memset(eh.ether_dhost, 0xff, 6);
    memcpy(eh.ether_shost, my_mac, 6);
    eh.ether_type = htons(ETHERTYPE_ARP);

    // Set ARP header
    arp.arp_hrd = htons(ARPHRD_ETHER);
    arp.arp_pro = htons(ETHERTYPE_IP);
    arp.arp_hln = 6;
    arp.arp_pln = 4;
    arp.arp_op = htons(ARPOP_REQUEST);

    memcpy(arp.arp_sha, my_mac, 6);
    memcpy(arp.arp_spa, &my_ip, 4);
    memset(arp.arp_tha, 0, 6);
    memcpy(arp.arp_tpa, &target_ip, 4);

    // Combine Ethernet and ARP headers
    memcpy(p, &eh, sizeof(struct ether_header));
    p += sizeof(struct ether_header);
    memcpy(p, &arp, sizeof(struct ether_arp));
    p += sizeof(struct ether_arp);

    // Send packet
    int ret = write(soc, buf, p - buf);
    if (ret < 0) {
        perror("write");
        return -1;
    }

    return 1;
}