/**
 * @file netutil.hpp
 * @brief Header file for network utility functions
 */

#ifndef NETUTIL_HPP
#define NETUTIL_HPP

#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/ip.h>  // For iphdr
#include <netinet/if_ether.h>
#include <string>
#include "base.hpp"

/**
 * @brief Class for network utility functions
 */
class NetworkUtil {
public:
    /**
     * @brief Constructor
     */
    NetworkUtil();

    /**
     * @brief Destructor
     */
    ~NetworkUtil();

    /**
     * @brief Convert MAC address to string
     * @param hwaddr MAC address
     * @return String representation of MAC address
     */
    static std::string EtherToString(const u_char* hwaddr);

    /**
     * @brief Convert IP address to string
     * @param addr IP address
     * @return String representation of IP address
     */
    static std::string InetToString(const struct in_addr* addr);

    /**
     * @brief Convert in_addr_t to string
     * @param addr IP address
     * @return String representation of IP address
     */
    static std::string InAddrToString(in_addr_t addr);

    /**
     * @brief Get device information
     * @param device Device name
     * @param hwaddr MAC address buffer
     * @param uaddr IP address buffer
     * @param subnet Subnet buffer
     * @param mask Netmask buffer
     * @return Success or failure code
     */
    static int GetDeviceInfo(const std::string& device, u_char hwaddr[6],
                             struct in_addr* uaddr, struct in_addr* subnet,
                             struct in_addr* mask);

    /**
     * @brief Print Ethernet header
     * @param eh Ethernet header
     * @return Success or failure code
     */
    static int PrintEtherHeader(struct ether_header* eh);

    /**
     * @brief Initialize raw socket
     * @param device Device name
     * @param promiscFlag Whether to enable promiscuous mode
     * @param ipOnly Whether to only capture IP packets
     * @return Socket descriptor or -1 on error
     */
    static int InitRawSocket(const std::string& device, int promiscFlag, int ipOnly);

    /**
     * @brief Calculate checksum
     * @param data Data to calculate checksum for
     * @param len Length of data
     * @return Checksum value
     */
    static u_int16_t Checksum(unsigned char* data, int len);

    /**
     * @brief Calculate checksum for two data blocks
     * @param data1 First data block
     * @param len1 Length of first data block
     * @param data2 Second data block
     * @param len2 Length of second data block
     * @return Checksum value
     */
    static u_int16_t Checksum2(unsigned char* data1, int len1, unsigned char* data2, int len2);

    /**
     * @brief Check IP header checksum
     * @param iphdr IP header
     * @param option IP options
     * @param optionLen IP options length
     * @return 1 if checksum is valid, 0 otherwise
     */
    static int CheckIPChecksum(struct iphdr* iphdr, unsigned char* option, int optionLen);

    /**
     * @brief Send ARP request
     * @param soc Socket descriptor
     * @param target_ip Target IP address
     * @param target_mac Target MAC address buffer
     * @param my_ip My IP address
     * @param my_mac My MAC address
     * @return Success or failure code
     */
    static int SendArpRequest(int soc, in_addr_t target_ip, unsigned char target_mac[6],
                             in_addr_t my_ip, unsigned char my_mac[6]);
};

#endif // NETUTIL_HPP