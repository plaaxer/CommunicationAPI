#ifndef NIC_HPP
#define NIC_HPP

#include <vector>
#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <arpa/inet.h>

#include "api/network/statistics.hpp"
#include "api/observer/conditionally_data_observed.h"
#include "api/network/definitions/ethernet.hpp"
#include "api/network/definitions/buffer.hpp"
#include "api/network/engines/raw_socket_engine.hpp"

// engine should be defined where nic is used

template <typename Engine>
class NIC : private Engine,
            public Conditionally_Data_Observed<Buffer<Ethernet::Frame>, Ethernet::Protocol>,
            public Ethernet
{
public:

    typedef Ethernet::MAC Address;
    typedef Ethernet::Protocol Protocol_Number;
    typedef Ethernet::Frame Frame;
    typedef Buffer<Ethernet::Frame> FrameBuffer;
    typedef Conditionally_Data_Observed<Buffer<Ethernet::Frame>, Protocol_Number> Observed;
    typedef typename Observed::Observer Observer;

    NIC() : _running(true) {
        // the engine constructor should initialize the hardware and set the MAC address

        std::cout << "NIC initialized. MAC: " << this->address() << std::endl;

        // create the receiving thread that will execute the _receiver_thread method
        _receiver = std::thread(&NIC::_receiver_thread, this);
        std::cout << "NIC's receiving thread initialized." << std::endl;
    }

    ~NIC() {
        // thread should stop
        _running = false;

        // maybe force the Engine to unblock the receive call?

        // wait for the thread to finish
        if (_receiver.joinable()) {
            _receiver.join();
        }
        std::cout << "Thread receptora da NIC finalizada." << std::endl;
    }

    /**
     * @brief Obtains the MAC address of the NIC.
     */
    const Address& address() {

        return Engine::address();
    }

    /**
     * @brief Adds an observer for a specific protocol.
     */
    void attach(Observer* obs, Protocol_Number prot) {
        Observed::attach(obs, prot);
    }

    /**
     * @brief Removes an observer for a specific protocol.
     */
    void detach(Observer* obs, Protocol_Number prot) {
        Observed::detach(obs, prot);
    }

    /**
     * @brief Returns statistics about the NIC's activity.
     */
    const Statistics& statistics() {
        return _statistics;
    }

    /**
     * @brief Allocates memory and set parameters of the frame header
     */
    FrameBuffer* alloc(const Address& dst, Protocol_Number prot, unsigned int size) {
        if (size > Ethernet::MTU) {
            std::cerr << "Requested size exceeds MTU." << std::endl;
            return nullptr;
        }
        
        FrameBuffer* new_buffer = new FrameBuffer(FrameBuffer::alloc());

        // getting the pointer of the frame inside the buffer
        Frame* frame = new_buffer->data();

        frame->header.shost = this->address();  // src MAC
        frame->header.dhost = dst;  // dst MAC
        frame->header.type = prot;  // ether type
        frame->data_length = size;  // data length

        return new_buffer;
    }

    void free(FrameBuffer* buf) {
        delete buf;
    }

    /**
     * @brief Sends data to a specified destination address using a given protocol number.
     * @param dst The destination address.
     * @param prot The protocol number.
     * @param data Pointer to the data to be sent.
     * @param size Size of the data in bytes.
     * @return Number of bytes sent, or -1 on error.
     */
    int send(const Address& dst, Protocol_Number prot, const void * data, unsigned int size) {
        
        std::cout << "\n\nENVIANDO PARA O MAC" << dst << std::endl;

        int bytes_sent = Engine::send(reinterpret_cast<const unsigned char*>(dst.addr),
                                      prot, data, size);

        if (bytes_sent > 0) {
            // these statistics are temporary, to be implemented properly later
            _statistics.tx_packets++;
            _statistics.tx_bytes += bytes_sent;
        }

        return bytes_sent;
    }

    /**
     * @brief Sends a pre-constructed Ethernet frame contained within a zero copy buffer
     * @param buf Pointer to a Buffer containing the Ethernet frame to be sent.
     * @return Number of bytes sent, or -1 on error.
     */
    int send(FrameBuffer* buf) {
        if (!buf) return -1;

        Frame* frame = buf->data();

        // protocol in host byte order needed
        Protocol_Number proto = frame->header.type;

        // debug_frame(*frame);

        std::this_thread::sleep_for(std::chrono::seconds(1)); // small, artificial delay to make it easier to debug logging

        return Engine::send(frame->header.dhost.addr,
                            proto, 
                            frame->data, 
                            frame->data_length);
    }    

private:

    inline void debug_frame(const Ethernet::Frame& frame) {
        std::cout << "--- Ethernet Frame Debug ---" << std::endl;
        std::cout << "  Destination MAC: " << frame.header.dhost << std::endl;
        std::cout << "  Source MAC:      " << frame.header.shost << std::endl;

        // Use ntohs() to convert the EtherType from network byte order to host
        // byte order, which is standard for printing and comparison.
        std::cout << "  EtherType:       0x" << std::hex << std::setw(4) << std::setfill('0')
                  << ntohs(frame.header.type) << std::dec << std::endl;

        std::cout << "  Data Length:     " << frame.data_length << " bytes" << std::endl;

        // Print a sample of the payload data (e.g., the first 16 bytes)
        // to avoid flooding the console.
        unsigned int bytes_to_print = std::min(16u, frame.data_length);
        if (bytes_to_print > 0) {
            std::cout << "  Payload Sample:  ";
            for (unsigned int i = 0; i < bytes_to_print; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') 
                        << static_cast<int>(frame.data[i]) << " ";
            }
            std::cout << std::dec << std::endl; // Reset stream to decimal
        }

        std::cout << "----------------------------" << std::endl;
    }

    /**
     * @brief Core method, executed by the receiving thread.
     */
    void _receiver_thread() {
        while (_running) {
            Frame received_frame;
            
            // we mustn't fill the entire frame (due to the length field)
            const int buffer_size = sizeof(received_frame.header) + sizeof(received_frame.data);

            // Engine::receive() should block until a frame is received
            int bytes_received = Engine::receive(reinterpret_cast<void*>(&received_frame), buffer_size);
            
            constexpr bool is_raw_socket_engine = std::is_same<Engine, RawSocketEngine>::value;

            // Need to verify the Engine being used cause SharedMemory will have same MAC between components
            if (received_frame.header.shost == address() && is_raw_socket_engine) {
                continue;
            }

            // // Uncomment to debug. Let's display info only at the End-Point
            // std::cout << "NIC received " << bytes_received << " bytes." << std::endl;
            // std::cout << "Source MAC: " << Address(received_frame.header.shost) << std::endl;
            // std::cout << "Destiny MAC: " << Address(received_frame.header.dhost) << std::endl;
            // std::cout << "EtherType: 0x" << std::hex << ntohs(received_frame.header.type) << std::dec << std::endl;
            
            if (bytes_received > 0) {

                Protocol_Number proto = ntohs(received_frame.header.type);
                // these statistics are temporary, to be implemented properly later
                _statistics.rx_packets++;
                _statistics.rx_bytes += bytes_received;

                std::cout << "ENGINE [" << Engine::name() << "] has received packet with protocol 0x" << std::hex << ntohs(received_frame.header.type) << std::dec << std::endl;

                // actual payload of the message (does include Protocol::PortHeader though)
                received_frame.data_length = bytes_received - sizeof(received_frame.header);

                // Buffer build
                FrameBuffer* buffer = new FrameBuffer(FrameBuffer::alloc());
                Frame* frame = buffer->data();
                *frame = received_frame;

                // notifies all observers interested in this protocol
                this->notify(proto, buffer);

            } else if (bytes_received <= 0 && !_running) {
                std::cout << "NIC receiver thread stopping as requested." << std::endl;
                break;
            }
        }
    }

    Statistics _statistics;
    std::thread _receiver;      // receiving thread
    std::atomic<bool> _running; // flag to control whether the thread should keep running
};

#endif // NIC_HPP