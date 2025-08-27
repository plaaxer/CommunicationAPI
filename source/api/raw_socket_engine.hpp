#ifndef RAW_SOCKET_ENGINE_HPP
#define RAW_SOCKET_ENGINE_HPP

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
    struct ifreq _ifr, if_mac;
    unsigned char _my_mac[ETH_ALEN];
    
    public:
    
    RawSocketEngine() {
        char ifname[] = "eth0";
        // creating the raw socket
        _sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (_sock < 0) {
            throw std::runtime_error("Failed to create raw socket");    
        }

        memset(&_ifr, 0, sizeof(_ifr));
        // set interface name
        strncpy(_ifr.ifr_name, ifname, sizeof(_ifr.ifr_name));

        // retrieve the interface index
        if (ioctl(_sock, SIOCGIFINDEX, &_ifr) < 0) {
            close(_sock);

            throw std::runtime_error("Failed to get interface index for " + std::string(ifname) +
                                     ": " + std::string(strerror(errno)));
        }

        // prepare sockaddr_ll structure
        _sockaddr.sll_ifindex = _ifr.ifr_ifindex;
        _sockaddr.sll_family = AF_PACKET;
        _sockaddr.sll_halen = ETH_ALEN; // Address length

        // std::cout << "[Kernel Instruction] Using interface index: " << _sockaddr.sll_ifindex << std::endl;

        // retrieve the MAC address of the interface
        if (ioctl(_sock, SIOCGIFHWADDR, &_ifr) < 0) {
            close(_sock);

            throw std::runtime_error("Failed to get MAC address for " + std::string(ifname) +
                                     ": " + std::string(strerror(errno)));
        }
        
        memcpy(_my_mac, _ifr.ifr_hwaddr.sa_data, ETH_ALEN);


        // bind the socket to the interface
        if (bind(_sock, (struct sockaddr*)&_sockaddr, sizeof(_sockaddr)) < 0) {
            close(_sock);
            throw std::runtime_error("Failed to bind raw socket to interface " + std::string(ifname));
        }

        std::cout << "Raw socket initialized and bound to " << ifname << std::endl;
    }

    ~RawSocketEngine() {
        if (_sock >= 0) {
            close(_sock);
        }
    }

    void print_hex(const char* title, const unsigned char* data, size_t size) {
        std::cout << title;
        std::cout << std::hex << std::setfill('0');
        for (size_t i = 0; i < size; ++i) {
            std::cout << std::setw(2) << static_cast<int>(data[i]) << " ";
        }
        std::cout << std::dec << std::endl; // Volta para o modo decimal
    }

    void print_mac(const char* title, const unsigned char* mac) {
        std::cout << title << std::hex << std::setfill('0');
        for (int i = 0; i < ETH_ALEN; ++i) {
            std::cout << std::setw(2) << static_cast<int>(mac[i]) << (i < ETH_ALEN - 1 ? ":" : "");
        }
        std::cout << std::dec << std::endl; // Reset to decimal mode
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

        memcpy(_sockaddr.sll_addr, dst_mac, ETH_ALEN);

        // building a buffer that can hold the header and the data. Its size is naturally header + data
        unsigned int frame_size = sizeof(struct ether_header) + size;
        std::vector<unsigned char> frame_buffer(frame_size);

        // get a pointer to the start of our buffer and cast it to an ether_header pointer
        struct ether_header* eth_header = reinterpret_cast<struct ether_header*>(frame_buffer.data());

        // filling the destination MAC address
        memcpy(eth_header->ether_dhost, dst_mac, ETH_ALEN);

        // setting dest mac in the sockaddr_ll structure
        memset(_sockaddr.sll_addr, 0xFF, ETH_ALEN);

        // filling our own source MAC address
        memcpy(eth_header->ether_shost, _my_mac, ETH_ALEN);

        // setting the EtherType (protocol)
        // htons converts from host byte order to network byte order (big-endian)
        // we also must do that in the receiving end (though at NIC)
        eth_header->ether_type = htons(protocol);

        // copying the payload data into the buffer right after the header
        memcpy(frame_buffer.data() + sizeof(struct ether_header), data, size);


    //     std::cout << "\n--- DEBUG: Preparando para enviar frame ---" << std::endl;

    //     // 1. Informações do cabeçalho
    //     print_hex("  [Header] MAC Destino: ", eth_header->ether_dhost, ETH_ALEN);
    //     print_hex("  [Header] MAC Origem:  ", eth_header->ether_shost, ETH_ALEN);
    //     std::cout << "  [Header] Protocolo:   0x" << std::hex << protocol << std::dec << std::endl;

    //     // 2. Informações da estrutura sockaddr_ll (para o kernel)
    //     std::cout << "  [sockaddr] Interface Index: " << _sockaddr.sll_ifindex << std::endl;
    //     print_hex("  [sockaddr] MAC Destino:   ", _sockaddr.sll_addr, ETH_ALEN);

    //     // 3. Amostra do frame completo que será enviado
    //     print_hex("[Frame Buffer] Primeiros 32 bytes: ", frame_buffer.data(), std::min((size_t)32, (size_t)frame_size));


        // std::cout << "[Kernel Instruction] Using interface index: " << _sockaddr.sll_ifindex << std::endl;
        // print_mac("[Kernel Instruction] Destination MAC (in sockaddr_ll): ", _sockaddr.sll_addr);

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

    Ethernet::MAC &address() {
        return *reinterpret_cast<Ethernet::MAC*>(_my_mac);
    }

};

#endif  // RAW_SOCKET_ENGINE_HPP
