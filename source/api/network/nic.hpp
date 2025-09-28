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
#include <csignal>

#include "api/network/statistics.hpp"
#include "api/observer/conditionally_data_observed.h"
#include "api/network/definitions/ethernet.hpp"
#include "api/network/definitions/buffer.hpp"
#include "api/network/engines/raw_socket_engine.hpp"
#include "utils/profiler.hpp"

template <typename Engine>
class NIC : private Engine,
            public Conditionally_Data_Observed<Buffer<Ethernet::Frame>, Ethernet::Protocol>,
            public Ethernet
{
private:

    // Static pointer for our C-style signal handler to access the NIC instance.
    static NIC<Engine>* _nic_instance_for_handler;

    // signal handler function.
    static void _sigio_handler(int signum) {
        if (_nic_instance_for_handler) {
            _nic_instance_for_handler->handle_receive();
        }
    }

public:

    typedef Ethernet::MAC Address;
    typedef Ethernet::Protocol Protocol_Number;
    typedef Ethernet::Frame Frame;
    typedef Buffer<Ethernet::Frame> FrameBuffer;
    typedef Conditionally_Data_Observed<Buffer<Ethernet::Frame>, Protocol_Number> Observed;
    typedef typename Observed::Observer Observer;

    NIC() {
        if constexpr (std::is_same_v<Engine, RawSocketEngine>) {
            // --- RAW SOCKET ENGINE ---
            // static pointer used by the signal handler
            _nic_instance_for_handler = this;

            struct sigaction sa;
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = &_sigio_handler; // assigning the handler function
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = SA_RESTART;

            if (sigaction(SIGIO, &sa, nullptr) == -1) {
                throw std::runtime_error("Failed to register SIGIO signal handler");
            }
            std::cout << "NIC initialized for asynchronous reception." << std::endl;
        } else {
            // --- SMH ENGINE ---
            _running = true;
            _receiver = std::thread(&NIC::_receiver_thread, this);
            std::cout << "NIC<" << typeid(Engine).name() << "> initialized with a receiver thread." << std::endl;
        }
    }

    ~NIC() {
        if constexpr (std::is_same_v<Engine, RawSocketEngine>) {
            // Restore default signal handler for SIGIO
            signal(SIGIO, SIG_DFL);
            _nic_instance_for_handler = nullptr;
        } else {
            _running = false;
            // a mechanism to unblock Engine::receive() might be needed here if it blocks indefinitely.
            if (_receiver.joinable()) {
                _receiver.join();
            }
        }
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
        int bytes_sent = Engine::send(reinterpret_cast<const unsigned char*>(dst.addr),
                                      prot, data, size);

        if (bytes_sent > 0) {
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
        Protocol_Number proto = frame->header.type;

        return Engine::send(frame->header.dhost.addr,
                            proto, 
                            frame->data, 
                            frame->data_length);
    }
    
    /**
     * @brief Asynchronous receive, non-static method called by the SIGIO handler.
     */
    void handle_receive() {
        // Loop to drain all available packets from the kernel buffer
        while (true) {
            Frame received_frame;
            const int buffer_size = sizeof(received_frame.header) + sizeof(received_frame.data);

            // non-blocking call to receive data
            int bytes_received = Engine::receive(reinterpret_cast<void*>(&received_frame), buffer_size);

            if (bytes_received <= 0) {
                break; // Kernel buffer is empty
            }
            
            constexpr bool is_raw_socket_engine = std::is_same<Engine, RawSocketEngine>::value;
            if (received_frame.header.shost == address() && is_raw_socket_engine) {
                continue; // Ignore packets we sent ourselves
            }
            
            _statistics.rx_packets++;
            _statistics.rx_bytes += bytes_received;

            Protocol_Number proto = ntohs(received_frame.header.type);
            received_frame.data_length = bytes_received - sizeof(received_frame.header);
            
            FrameBuffer* buffer = new FrameBuffer(FrameBuffer::alloc());
            *(buffer->data()) = received_frame;

            // notify observers, passing ownership of the buffer
            if (!this->notify(proto, buffer)) {
                // if no observer claimed the buffer, free it.
                delete buffer;
            }
        }
    }


    uint16_t registerComponentService(const std::string& name, uint32_t type_id) {
        return Engine::registerService(name, type_id);
    }

    uint16_t lookupByType(uint32_t type_id) {
        return Engine::lookupServiceByType(type_id);
    }

    Statistics _statistics;
    Profiler * _profiler;

    void setProfiler(Profiler* p) {
        _profiler = p;
    }

private:

    std::thread _receiver;
    std::atomic<bool> _running{false};

    /**
     * @brief Receiving method used by the receiver thread for non-async engines (as shm).
     */
    void _receiver_thread() {
        while (_running) {
            Frame received_frame;
            const int buffer_size = sizeof(received_frame.header) + sizeof(received_frame.data);

            int bytes_received = Engine::receive(reinterpret_cast<void*>(&received_frame), buffer_size);
            
            constexpr bool is_raw_socket_engine = std::is_same<Engine, RawSocketEngine>::value;

            if (received_frame.header.shost == address() && is_raw_socket_engine) {
                continue;
            }
            
            if (bytes_received > 0) {
                Protocol_Number proto = ntohs(received_frame.header.type);
                _statistics.rx_packets++;
                _statistics.rx_bytes += bytes_received;

                received_frame.data_length = bytes_received - sizeof(received_frame.header);

                FrameBuffer* buffer = new FrameBuffer(FrameBuffer::alloc());
                *(buffer->data()) = received_frame;

                this->notify(proto, buffer);

            } else if (bytes_received <= 0 && !_running) {
                std::cout << "NIC receiver thread stopping as requested." << std::endl;
                break;
            }
        }
    }

    inline void debug_frame(const Ethernet::Frame& frame) {
        std::cout << "--- Ethernet Frame Debug ---" << std::endl;
        std::cout << "  Destination MAC: " << frame.header.dhost << std::endl;
        std::cout << "  Source MAC:      " << frame.header.shost << std::endl;
        std::cout << "  EtherType:       0x" << std::hex << std::setw(4) << std::setfill('0')
                << ntohs(frame.header.type) << std::dec << std::endl;
        std::cout << "  Data Length:     " << frame.data_length << " bytes" << std::endl;

        unsigned int bytes_to_print = std::min(16u, frame.data_length);
        if (bytes_to_print > 0) {
            std::cout << "  Payload Sample:  ";
            for (unsigned int i = 0; i < bytes_to_print; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') 
                        << static_cast<int>(frame.data[i]) << " ";
            }
            std::cout << std::dec << std::endl;
        }
        std::cout << "----------------------------" << std::endl;
    }

};

template <typename Engine>
NIC<Engine>* NIC<Engine>::_nic_instance_for_handler = nullptr;

#endif // NIC_HPP