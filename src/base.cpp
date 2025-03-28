/**
 * @file base.cpp
 * @brief Implementation of base classes for router
 */

#include "base.hpp"
#include <cstring>

/**
 * @brief DataBuf constructor
 */
DataBuf::DataBuf() : next(nullptr), before(nullptr), time(0), size(0), data(nullptr) {
}

/**
 * @brief DataBuf destructor
 */
DataBuf::~DataBuf() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }
}

/**
 * @brief SendData constructor
 */
SendData::SendData() : top(nullptr), bottom(nullptr), data_num(0), in_bucket_size(0) {
}

/**
 * @brief SendData destructor
 */
SendData::~SendData() {
    DataBuf* ptr = top;
    while (ptr != nullptr) {
        DataBuf* next = ptr->next;
        delete ptr;
        ptr = next;
    }
    top = nullptr;
    bottom = nullptr;
}

/**
 * @brief IP2MAC constructor
 */
IP2MAC::IP2MAC() : flag(FLAG_FREE), device_number(0), ip_addr(0), lastTime(0) {
    memset(hw_addr, 0, 6);
}

/**
 * @brief IP2MAC destructor
 */
IP2MAC::~IP2MAC() {
    // SendData will clean up itself via its destructor
}