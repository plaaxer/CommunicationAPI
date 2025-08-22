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

class RawSocketEngine {

private:
    const char* interface_name = "eth0";
    int sock;
    struct ifreq ifr;

public:
    RawSocketEngine() {

        // creating the raw socket, as in distribuída
        sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (sock < 0) {
            throw std::runtime_error("Failed to create raw socket");
        }
        
        // cleans up the ifreq structure
        memset(&ifr, 0, sizeof(ifr));

        // sets the interface name
        strncpy(ifr.ifr_name, interface_name, IFNAMSIZ - 1);

        // retrieves the interface index
        if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
            close(sock);
            throw std::runtime_error("Failed to get interface index for " + std::string(interface_name));
        }

        struct sockaddr_ll sockaddr;

        // cleans up
        memset(&sockaddr, 0, sizeof(sockaddr));

        // sets the socket address family, protocol, and interface index
        sockaddr.sll_family = AF_PACKET;
        sockaddr.sll_protocol = htons(ETH_P_ALL);
        sockaddr.sll_ifindex = ifr.ifr_ifindex;

        // binds the socket to the interface
        if (bind(sock, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
            close(sock);
            throw std::runtime_error("Failed to bind raw socket to interface " + std::string(interface_name));
        }

        std::cout << "Raw socket initialized and bound to " << interface_name << std::endl;
    }

    ~RawSocketEngine() {
        close(sock);
    }

    int send(const uint8_t* dst_mac, const void* data, unsigned int size) {
        return -1;
    }

    int receive(void* data, unsigned int size) {
        return -1;
    }

};

#endif