# toy-router

A simple router implementation in C++ for educational purposes.

## Overview

This project implements a basic router that can forward packets between two network interfaces. It's a reimplementation of a C-based router in modern C++.

## Features

- Packet forwarding between two network interfaces
- ARP resolution for IP-to-MAC mapping
- ICMP Time Exceeded message generation
- Thread-safe buffer management

## Building

To build the router, run:

```bash
make
```

## Usage

Run the router with:

```bash
./router [receiving_interface] [sending_interface] [next_router_ip]
```

Where:
- `receiving_interface`: Name of the receiving network interface (default: enp0s8)
- `sending_interface`: Name of the sending network interface (default: enp0s9)
- `next_router_ip`: IP address of the next hop router (default: 169.254.238.208)

## Components

- `base.hpp/cpp`: Basic data structures and classes
- `ip2mac.hpp/cpp`: IP to MAC address resolution
- `netutil.hpp/cpp`: Network utility functions
- `router.hpp/cpp`: Main router implementation
- `send_buf.hpp/cpp`: Buffer management for packet sending

## Requirements

- Linux OS
- g++ with C++17 support
- Raw socket permissions (run as root or with appropriate capabilities)
