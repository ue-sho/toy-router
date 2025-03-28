/**
 * @file send_buf.hpp
 * @brief Header file for functions related to managing send buffer data
 */

#ifndef SEND_BUF_HPP
#define SEND_BUF_HPP

#include <netinet/in.h>
#include "base.hpp"

/**
 * @brief Class for managing send buffer data
 */
class SendBuf {
public:
    /**
     * @brief Constructor
     */
    SendBuf();

    /**
     * @brief Destructor
     */
    ~SendBuf();

    /**
     * @brief Append data to send buffer
     * @param ip2mac Pointer to IP2MAC entry
     * @param deviceNo Device number
     * @param addr IP address
     * @param data Data to append
     * @param size Size of data
     * @return Success or failure code
     */
    int AppendSendData(IP2MAC* ip2mac, int deviceNo, in_addr_t addr, unsigned char* data, int size);

    /**
     * @brief Get data from send buffer
     * @param ip2mac Pointer to IP2MAC entry
     * @param size Pointer to store data size
     * @param data Pointer to store data pointer
     * @return Success or failure code
     */
    int GetSendData(IP2MAC* ip2mac, int* size, unsigned char** data);

    /**
     * @brief Free send data
     * @param ip2mac Pointer to IP2MAC entry
     * @return Success or failure code
     */
    int FreeSendData(IP2MAC* ip2mac);

    /**
     * @brief Send buffered data
     * @return Success or failure code
     */
    int BufferSend();
};

#endif // SEND_BUF_HPP