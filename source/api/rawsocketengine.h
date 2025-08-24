// RawSocketEngine.h
#ifndef RAWSOCKETENGINE_H
#define RAWSOCKETENGINE_H

#include <sys/socket.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdexcept>
#include <iostream>
#include <vector>

#include "api/network/ethernet.hpp"

class RawSocketEngine {

private:
    int _sock;
    struct sockaddr_ll _sockaddr;
    struct ifreq _ifr;
    unsigned char _my_mac[ETH_ALEN];
    const char* interface_name = "eth0";

public:

    RawSocketEngine() {
        // creating the raw socket
        _sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (_sock < 0) {
            throw std::runtime_error("Failed to create raw socket");
        }
        
        // set interface name
        strncpy(_ifr.ifr_name, interface_name, IFNAMSIZ - 1);

        // retrieve the interface index
        if (ioctl(_sock, SIOCGIFINDEX, &_ifr) < 0) {
            close(_sock);
            throw std::runtime_error("Failed to get interface index for " + std::string(interface_name));
        }

        // retrieve the MAC address of the interface
        if (ioctl(_sock, SIOCGIFHWADDR, &_ifr) < 0) {
            close(_sock);
            throw std::runtime_error("Failed to get MAC address for " + std::string(interface_name));
        }
        memcpy(_my_mac, _ifr.ifr_hwaddr.sa_data, ETH_ALEN);

        // prepare sockaddr_ll structure
        memset(&_sockaddr, 0, sizeof(_sockaddr));
        _sockaddr.sll_family = AF_PACKET;
        _sockaddr.sll_protocol = htons(ETH_P_ALL);
        _sockaddr.sll_ifindex = _ifr.ifr_ifindex;

        // bind the socket to the interface
        if (bind(_sock, (struct sockaddr*)&_sockaddr, sizeof(_sockaddr)) < 0) {
            close(_sock);
            throw std::runtime_error("Failed to bind raw socket to interface " + std::string(interface_name));
        }

        std::cout << "Raw socket initialized and bound to " << interface_name << std::endl;
    }

    ~RawSocketEngine() {
        if (_sock >= 0) {
            close(_sock);
        }
    }

    /**
     * @brief Sends raw Ethernet frames
     * @param dst_mac Destination MAC address (6 bytes)
     * @param protocol EtherType
     * @param data Pointer to the payload data
     * @param size Size of the payload data in bytes
     * @return Number of bytes sent, or -1 on error
     */
    int send(const unsigned char* dst_mac, unsigned short protocol, const void* data, unsigned int size) {
        // padding is the OS's responsibility, we just send what we have

        if (size > ETH_DATA_LEN) {
            // should not happen in normal use (data does not get fragmented here)
            return -1; 
        }

        // building a buffer that can hold the header and the data. Its size is naturally header + data
        unsigned int frame_size = sizeof(struct ether_header) + size;
        std::vector<unsigned char> frame_buffer(frame_size);

        // get a pointer to the start of our buffer and cast it to an ether_header pointer
        struct ether_header* eth_header = reinterpret_cast<struct ether_header*>(frame_buffer.data());

        // filling the destination MAC address
        memcpy(eth_header->ether_dhost, dst_mac, ETH_ALEN);

        // filling our own source MAC address
        memcpy(eth_header->ether_shost, _my_mac, ETH_ALEN);

        // setting the EtherType (protocol)
        // htons converts from host byte order to network byte order (big-endian)
        // we also must do that in the receiving end (though at NIC)
        eth_header->ether_type = htons(protocol);

        // copying the payload data into the buffer right after the header
        memcpy(frame_buffer.data() + sizeof(struct ether_header), data, size);

        int bytes_sent = sendto(_sock, frame_buffer.data(), frame_size, 0, (struct sockaddr*)&_sockaddr, sizeof(_sockaddr));
        
        if (bytes_sent < 0) {
            perror("sendto failed");
        }

        return bytes_sent;
    }

    /**
     * @brief Receives raw Ethernet frames
     * @param buffer Pointer to a buffer where the received frame will be stored
     * @param buffer_size Size of the buffer in bytes
     * @return Number of bytes received, or -1 on error
     */
    int receive(void* buffer, unsigned int buffer_size) {
        
        // recvfrom is synchronous and will block until a frame is received
        int bytes_received = recvfrom(_sock, buffer, buffer_size, 0, NULL, NULL);

        if (bytes_received < 0) {
            perror("recvfrom failed");
        }
        
        // received data includes the Ethernet header; the caller must handle it
        return bytes_received;
    }

    Ethernet::MAC address() {
        // TO VERIFY!!
        return Ethernet::MAC(_my_mac);
    }

};

#endif