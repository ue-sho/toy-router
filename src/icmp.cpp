/**
 * @file icmp.cpp
 * @brief ICMP protocol implementation
 */

#include "../../include/icmp.h"
#include <cstring>
#include <algorithm>

std::vector<uint8_t> ICMP::createTimeExceededMessage(
    IcmpTimeExceededCode code,
    const uint8_t* original_ip_packet,
    size_t original_packet_size
) {
    // Create ICMP message
    // For TTL exceeded, we include the IP header and the first 8 bytes of the IP data
    constexpr size_t ORIGINAL_DATA_SIZE = 8; // First 8 bytes of original IP datagram

    // Calculate how much of the original packet to include
    size_t original_data_to_include = std::min(
        original_packet_size,
        sizeof(IcmpHeader) + ORIGINAL_DATA_SIZE
    );

    // Create buffer for ICMP message
    std::vector<uint8_t> icmp_message(sizeof(IcmpHeader) + original_data_to_include);

    // Set up ICMP header
    IcmpHeader* header = reinterpret_cast<IcmpHeader*>(icmp_message.data());
    header->type = static_cast<uint8_t>(IcmpType::TIME_EXCEEDED);
    header->code = static_cast<uint8_t>(code);
    header->checksum = 0;
    header->rest = 0; // Unused for Time Exceeded

    // Copy original IP header + 8 bytes of data
    std::memcpy(
        icmp_message.data() + sizeof(IcmpHeader),
        original_ip_packet,
        original_data_to_include
    );

    // Calculate checksum
    header->checksum = calculateChecksum(icmp_message.data(), icmp_message.size());

    return icmp_message;
}

uint16_t ICMP::calculateChecksum(const uint8_t* data, size_t length) {
    uint32_t sum = 0;

    // Handle complete 16-bit blocks
    const uint16_t* data16 = reinterpret_cast<const uint16_t*>(data);
    size_t len16 = length / 2;

    for (size_t i = 0; i < len16; i++) {
        sum += ntohs(data16[i]);
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
    }

    // Handle remaining byte if length is odd
    if (length & 1) {
        // Add last byte padded with zero
        sum += static_cast<uint16_t>(data[length - 1]) << 8;
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Take one's complement
    return static_cast<uint16_t>(~sum);
}