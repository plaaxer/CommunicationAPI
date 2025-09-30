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
#include "api/network/engines/smh_engine.hpp"
//#include "utils/profiler.cpp"

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

    NIC() {
        _running = true;
        if constexpr (std::is_same_v<Engine, RawSocketEngine>) {
            // --- RAW SOCKET ENGINE ---

            _signal_thread = std::thread(&NIC::_signal_waiter_thread, this);
            std::cout << "NIC<RawSocketEngine> initialized with a dedicated signal waiter thread." << std::endl;

        } else {
            // --- SMH ENGINE ---
            _receiver = std::thread(&NIC::_receiver_thread, this);
            std::cout << "NIC<" << typeid(Engine).name() << "> initialized with a receiver thread." << std::endl;
        }
    }

    ~NIC() 
    {
        _running = false;
        if constexpr (std::is_same_v<Engine, RawSocketEngine>) {
            if (_signal_thread.joinable()) {
                // To unblock a thread stuck in sigwait(), we send it the signal waited
                pthread_kill(_signal_thread.native_handle(), SIGIO);
                _signal_thread.join();
            }
        } else {
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
        // if (buf->is_view()) {
        //     // If it's a view, we don't deallocate the data.
        //     // Instead, we release the shared memory slot.
        //     if constexpr (!std::is_same_v<Engine, RawSocketEngine>) {
        //         Engine::release_frame(buf->sequence_id());
        //     }
        // }
        // We always delete the Buffer object itself, which is separate from the data it points to.
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
    void handle_receive() 
    {
        /*
        This function is now called by our dedicated signal thread, not a handler.
        It's crucial to loop here to drain all packets from the socket buffer
        before waiting for the next signal.
        */
        while(true) {
            Frame received_frame;
            const int buffer_size = sizeof(received_frame.header) + sizeof(received_frame.data);
            int bytes_received = Engine::receive(reinterpret_cast<void*>(&received_frame), buffer_size);

            if (bytes_received <= 0) {
                return; 
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

            if (!this->notify(proto, buffer)) {
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

private:

    std::thread _signal_thread;  // new thread member for the dedicated signal waiter
    std::thread _receiver;
    std::atomic<bool> _running{false};

    void _signal_waiter_thread() {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGIO);

        while (_running) {
            int sig;
            // The thread blocks here efficiently, waiting for a SIGIO to be delivered.
            if (sigwait(&mask, &sig) == 0) {
                if (sig == SIGIO) {
                    this->handle_receive();
                }
            }
        }
    }

    /**
     * @brief Receiving method used by the receiver thread for non-async engines (as shm).
     */
    // void _receiver_thread() {
    //     if constexpr (!std::is_same_v<Engine, RawSocketEngine>) {
    //         std::cout << "[PID " << getpid() << "] _receiver_thread has started." << std::endl;
    //         while (_running) {
    //             // Use the new zero-copy receive method


    //             auto* slot = Engine::receive_zerocopy();

    //             if (slot) {

    //                 Protocol_Number proto = ntohs(slot->frame.header.type);
    //                 _statistics.rx_packets++;
    //                 // Note: We'd need to calculate bytes_received if it's variable
    //                 _statistics.rx_bytes += sizeof(slot->frame.header) + slot->frame.data_length;

    //                 // Create a non-owning "view" buffer that points directly into shared memory
    //                 FrameBuffer* buffer = new FrameBuffer(&slot->frame, slot->sequence_id);

    //                 // Notify observers. If no one handles it, we must release the frame.
    //                 if (!this->notify(proto, buffer)) {
    //                     // If no observer took ownership, we free it immediately.
    //                     this->free(buffer);
    //                 }

    //             } else if (!_running) {
    //                 std::cout << "NIC receiver thread stopping as requested." << std::endl;
    //                 break;
    //             } else {
    //                 // If receive_zerocopy returned nullptr, someting went wrong or no data was available.
    //                 std::cout << "NIC receiver thread: receive_zerocopy returned nullptr, retrying..." << std::endl;
    //                 std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //             }
    //         }
    //     }
    // }
    void _receiver_thread() {
        if constexpr (std::is_same_v<Engine, ShmEngine>) {
            std::cout << "[PID " << getpid() << "] _receiver_thread (SHM) has started." << std::endl;
            while (_running) {
                // 1. Get a direct pointer to the data in shared memory
                auto* slot = Engine::receive_zerocopy();

                if (slot) {
                    // Prepare to notify observers
                    Protocol_Number proto = ntohs(slot->frame.header.type);
                    _statistics.rx_packets++;
                    _statistics.rx_bytes += sizeof(slot->frame.header) + slot->frame.data_length;

                    // 2. Create the non-owning "view" buffer that points to the SHM slot
                    FrameBuffer* buffer = new FrameBuffer(&slot->frame, slot->sequence_id);

                    // 3. CRITICAL DEBUGGING STEP: Immediately release the frame.
                    // This acknowledges the read right away, ignoring the application thread's status.
                    Engine::release_frame(slot->sequence_id);

                    // 4. Notify the upper layers with the view buffer.
                    // If no observer handles it, we just delete the wrapper object.
                    if (!this->notify(proto, buffer)) {
                        delete buffer;
                    }

                } else if (!_running) {
                    std::cout << "NIC receiver thread stopping as requested." << std::endl;
                    break;
                }
            }
        }
    }
};

#endif // NIC_HPP