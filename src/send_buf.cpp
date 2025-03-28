/**
 * @file send_buf.cpp
 * @brief Implementation of functions related to managing send buffer data
 */

#include "send_buf.hpp"
#include <cstring>
#include <ctime>

/**
 * @brief Constructor
 */
SendBuf::SendBuf() {
}

/**
 * @brief Destructor
 */
SendBuf::~SendBuf() {
}

/**
 * @brief Append data to send buffer
 * @param ip2mac Pointer to IP2MAC entry
 * @param deviceNo Device number
 * @param addr IP address
 * @param data Data to append
 * @param size Size of data
 * @return Success or failure code
 */
int SendBuf::AppendSendData(IP2MAC* ip2mac, int deviceNo, in_addr_t addr, unsigned char* data, int size) {
    if (ip2mac == nullptr) {
        return -1;
    }

    DataBuf* buf = new DataBuf();
    if (buf == nullptr) {
        return -1;
    }

    buf->size = size;
    buf->data = new unsigned char[size];
    if (buf->data == nullptr) {
        delete buf;
        return -1;
    }

    memcpy(buf->data, data, size);
    buf->time = time(nullptr);

    // Insert into linked list
    buf->next = nullptr;
    buf->before = ip2mac->send_data.bottom;

    if (ip2mac->send_data.bottom == nullptr) {
        ip2mac->send_data.top = buf;
        ip2mac->send_data.bottom = buf;
    } else {
        ip2mac->send_data.bottom->next = buf;
        ip2mac->send_data.bottom = buf;
    }

    ip2mac->send_data.data_num++;
    ip2mac->send_data.in_bucket_size += size;

    return 1;
}

/**
 * @brief Get data from send buffer
 * @param ip2mac Pointer to IP2MAC entry
 * @param size Pointer to store data size
 * @param data Pointer to store data pointer
 * @return Success or failure code
 */
int SendBuf::GetSendData(IP2MAC* ip2mac, int* size, unsigned char** data) {
    if (ip2mac == nullptr) {
        return -1;
    }

    if (ip2mac->send_data.top == nullptr) {
        return -1;
    }

    DataBuf* buf = ip2mac->send_data.top;
    *size = buf->size;
    *data = buf->data;

    ip2mac->send_data.top = buf->next;
    if (ip2mac->send_data.top != nullptr) {
        ip2mac->send_data.top->before = nullptr;
    } else {
        ip2mac->send_data.bottom = nullptr;
    }

    ip2mac->send_data.data_num--;
    ip2mac->send_data.in_bucket_size -= buf->size;

    // Transfer ownership of data to caller, prevent destruction in buf destructor
    buf->data = nullptr;
    delete buf;

    return 1;
}

/**
 * @brief Free send data
 * @param ip2mac Pointer to IP2MAC entry
 * @return Success or failure code
 */
int SendBuf::FreeSendData(IP2MAC* ip2mac) {
    if (ip2mac == nullptr) {
        return -1;
    }

    DataBuf* ptr = ip2mac->send_data.top;
    while (ptr != nullptr) {
        DataBuf* next = ptr->next;
        delete ptr;
        ptr = next;
    }

    ip2mac->send_data.top = nullptr;
    ip2mac->send_data.bottom = nullptr;
    ip2mac->send_data.data_num = 0;
    ip2mac->send_data.in_bucket_size = 0;

    return 1;
}

/**
 * @brief Send buffered data
 * @return Success or failure code
 */
int SendBuf::BufferSend() {
    // Implementation depends on IP2MAC table management
    // Will be implemented when IP2MAC class is fully defined
    return 1;
}