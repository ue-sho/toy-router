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
 * @brief SendData copy constructor
 */
SendData::SendData(const SendData& other) : top(nullptr), bottom(nullptr), data_num(0), in_bucket_size(0) {
    // Deep copy of linked list
    DataBuf* current = other.top;
    while (current != nullptr) {
        DataBuf* new_buf = new DataBuf();
        new_buf->time = current->time;
        new_buf->size = current->size;

        if (current->data != nullptr && current->size > 0) {
            new_buf->data = new unsigned char[current->size];
            memcpy(new_buf->data, current->data, current->size);
        }

        // Add to our list
        if (top == nullptr) {
            top = new_buf;
            bottom = new_buf;
            new_buf->before = nullptr;
        } else {
            bottom->next = new_buf;
            new_buf->before = bottom;
            bottom = new_buf;
        }

        new_buf->next = nullptr;
        data_num++;
        in_bucket_size += new_buf->size;

        current = current->next;
    }
}

/**
 * @brief SendData copy assignment operator
 */
SendData& SendData::operator=(const SendData& other) {
    if (this == &other) {
        return *this;
    }

    // Clear existing data
    DataBuf* current = top;
    while (current != nullptr) {
        DataBuf* next = current->next;
        delete current;
        current = next;
    }

    top = nullptr;
    bottom = nullptr;
    data_num = 0;
    in_bucket_size = 0;

    // Deep copy from other
    current = other.top;
    while (current != nullptr) {
        DataBuf* new_buf = new DataBuf();
        new_buf->time = current->time;
        new_buf->size = current->size;

        if (current->data != nullptr && current->size > 0) {
            new_buf->data = new unsigned char[current->size];
            memcpy(new_buf->data, current->data, current->size);
        }

        // Add to our list
        if (top == nullptr) {
            top = new_buf;
            bottom = new_buf;
            new_buf->before = nullptr;
        } else {
            bottom->next = new_buf;
            new_buf->before = bottom;
            bottom = new_buf;
        }

        new_buf->next = nullptr;
        data_num++;
        in_bucket_size += new_buf->size;

        current = current->next;
    }

    return *this;
}

/**
 * @brief SendData move constructor
 */
SendData::SendData(SendData&& other) noexcept
    : top(other.top), bottom(other.bottom), data_num(other.data_num), in_bucket_size(other.in_bucket_size) {
    other.top = nullptr;
    other.bottom = nullptr;
    other.data_num = 0;
    other.in_bucket_size = 0;
}

/**
 * @brief SendData move assignment operator
 */
SendData& SendData::operator=(SendData&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    // Clear existing data
    DataBuf* current = top;
    while (current != nullptr) {
        DataBuf* next = current->next;
        delete current;
        current = next;
    }

    // Move data from other
    top = other.top;
    bottom = other.bottom;
    data_num = other.data_num;
    in_bucket_size = other.in_bucket_size;

    // Reset other
    other.top = nullptr;
    other.bottom = nullptr;
    other.data_num = 0;
    other.in_bucket_size = 0;

    return *this;
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
 * @brief IP2MAC copy constructor
 */
IP2MAC::IP2MAC(const IP2MAC& other)
    : flag(other.flag), device_number(other.device_number), ip_addr(other.ip_addr),
      lastTime(other.lastTime), send_data(other.send_data) {
    memcpy(hw_addr, other.hw_addr, 6);
}

/**
 * @brief IP2MAC copy assignment operator
 */
IP2MAC& IP2MAC::operator=(const IP2MAC& other) {
    if (this == &other) {
        return *this;
    }

    flag = other.flag;
    device_number = other.device_number;
    ip_addr = other.ip_addr;
    memcpy(hw_addr, other.hw_addr, 6);
    lastTime = other.lastTime;
    send_data = other.send_data;

    return *this;
}

/**
 * @brief IP2MAC move constructor
 */
IP2MAC::IP2MAC(IP2MAC&& other) noexcept
    : flag(other.flag), device_number(other.device_number), ip_addr(other.ip_addr),
      lastTime(other.lastTime), send_data(std::move(other.send_data)) {
    memcpy(hw_addr, other.hw_addr, 6);

    other.flag = FLAG_FREE;
    other.device_number = 0;
    other.ip_addr = 0;
    memset(other.hw_addr, 0, 6);
    other.lastTime = 0;
}

/**
 * @brief IP2MAC move assignment operator
 */
IP2MAC& IP2MAC::operator=(IP2MAC&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    flag = other.flag;
    device_number = other.device_number;
    ip_addr = other.ip_addr;
    memcpy(hw_addr, other.hw_addr, 6);
    lastTime = other.lastTime;
    send_data = std::move(other.send_data);

    other.flag = FLAG_FREE;
    other.device_number = 0;
    other.ip_addr = 0;
    memset(other.hw_addr, 0, 6);
    other.lastTime = 0;

    return *this;
}

/**
 * @brief IP2MAC destructor
 */
IP2MAC::~IP2MAC() {
    // SendData will clean up itself via its destructor
}