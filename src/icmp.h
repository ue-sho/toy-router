/**
 * @file icmp.h
 * @brief ICMP protocol implementation
 */

#ifndef ICMP_H
#define ICMP_H

#include <cstdint>
#include <memory>
#include <vector>
#include "../src/common/common.h"

// ICMP message types
enum class IcmpType : uint8_t {
    ECHO_REPLY = 0,
    DESTINATION_UNREACHABLE = 3,
    SOURCE_QUENCH = 4,
    REDIRECT = 5,
    ECHO_REQUEST = 8,
    TIME_EXCEEDED = 11,
    PARAMETER_PROBLEM = 12,
    TIMESTAMP = 13,
    TIMESTAMP_REPLY = 14,
    INFO_REQUEST = 15,
    INFO_REPLY = 16
};

// ICMP codes for TIME_EXCEEDED type
enum class IcmpTimeExceededCode : uint8_t {
    TTL_EXPIRED_IN_TRANSIT = 0,
    FRAGMENT_REASSEMBLY_TIME_EXCEEDED = 1
};

// ICMP header structure
#pragma pack(push, 1)
struct IcmpHeader {
    uint8_t type;      // Message type
    uint8_t code;      // Message code
    uint16_t checksum; // Checksum
    uint32_t rest;     // Rest of header (depends on type and code)
};
#pragma pack(pop)

class ICMP {
public:
    // Create ICMP Time Exceeded message
    static std::vector<uint8_t> createTimeExceededMessage(
        IcmpTimeExceededCode code,
        const uint8_t* original_ip_packet,
        size_t original_packet_size
    );

    // Calculate ICMP checksum
    static uint16_t calculateChecksum(const uint8_t* data, size_t length);
};

#endif // ICMP_H