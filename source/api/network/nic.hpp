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

#include "api/observer/conditionally_data_observed.h"
#include "api/network/definitions/ethernet.hpp"
#include "api/network/definitions/buffer.hpp"
#include "api/network/engines/raw_socket_engine.hpp"
#include "api/network/engines/smh_engine.hpp"
#include "api/network/definitions/quadrant.hpp"
#include "api/network/definitions/protocol_definitions.hpp"
#include "api/network/crypto/i_crypto_provider.hpp"

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
            _signal_t = std::thread(&NIC::_signal_waiter, this);

        } else {
            // --- SMH ENGINE ---
            _receiver = std::thread(&NIC::_receiver_thread, this);
            // std::cout << "NIC<" << typeid(Engine).name() << "> initialized with a receiver thread." << std::endl;
        }

        _quadrant = Quadrant::SOUTH; // for now everyone starts at the south
    }

    ~NIC() 
    {
        _running = false;
        if constexpr (std::is_same_v<Engine, RawSocketEngine>) {
            if (_signal_t.joinable()) {
                // To unblock a thread stuck in sigwait(), we send it the signal waited
                pthread_kill(_signal_t.native_handle(), SIGIO);
                _signal_t.join();
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
     * @brief Allocates memory and set parameters of the frame header
     */
    FrameBuffer* alloc(const Address& src, const Address& dst, Protocol_Number prot, unsigned int size) {
        if (size > Ethernet::MTU) {
            std::cerr << "Requested size exceeds MTU." << std::endl;
            return nullptr;
        }
        
        FrameBuffer* new_buffer = new FrameBuffer(FrameBuffer::alloc());

        // getting the pointer of the frame inside the buffer
        Frame* frame = new_buffer->data();

        frame->header.shost = src;  // src MAC
        frame->header.dhost = dst;  // dst MAC
        frame->header.type = prot;  // ether type
        frame->data_length = size;  // data length

        return new_buffer;
    }

    void free(FrameBuffer* buf) {
        if (buf->is_view()) {
            if constexpr (std::is_same_v<Engine, ShmEngine>) {
            
                // std::cout << "[PID " << getpid() << "] FREEING slot: " << buf->sequence_id() 
                //       << " at " << us << " us" << std::endl;
                Engine::release_frame(buf->sequence_id());
            }
        }
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
    int send(const Address& dst, Protocol_Number prot, const void * data, unsigned int size)
    {
        int bytes_sent = Engine::send(reinterpret_cast<const unsigned char*>(dst.addr),
                                      prot, data, size);

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

        // std::cout << "[PID " << getpid() << "] Sending frame from " 
        //           << frame->header.shost << " to " 
        //           << frame->header.dhost  
        //           << " of size " << frame->data_length + sizeof(frame->header) 
        //           << " bytes." << std::endl;

        return Engine::send(frame->header.shost.addr,
                            frame->header.dhost.addr,
                            proto, 
                            frame->data, 
                            frame->data_length);
    }

    /**
     * @brief Asynchronous receive, non-static method called by the SIGIO handler.
     * @details This function is called when handling a signal by the RawSocketEngine.
        It's crucial to loop here to drain all packets from the socket buffer
        before waiting for the next signal.
     */
    void handle_receive() 
    {
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

    void set_session_key(SessionKey session_key) {
        if constexpr(std::is_same_v<Engine, ShmEngine>) {
            Engine::set_session_key(session_key);
        }
        throw std::runtime_error("Session key should be located at NIC<ShmEngine>");
    }

    SessionKey get_session_key() {
        if constexpr(std::is_same_v<Engine, ShmEngine>) {
            return Engine::get_session_key();
        }
        throw std::runtime_error("Session key should be located at NIC<ShmEngine");
    }

private:

    Quadrant _quadrant;

    std::thread _signal_t;
    std::thread _receiver;
    std::atomic<bool> _running{false};

    void _signal_waiter() {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGIO);

        while (_running) {
            int sig;
            if (sigwait(&mask, &sig) == 0) {
                if (sig == SIGIO) {
                    this->handle_receive();
                }
            }
        }
    }

    /**
     * @brief Receiving method used by the receiver thread for non-async engines (as shm).
     * @details It has been updated to use zero-copy semantics. The FrameBuffer returned is a "view"
     * pointing directly to the shared memory slot; we never allocate/copy the data
     * (except for when filling the Application's Message wrapper).
     */
    void _receiver_thread() {

        if constexpr (std::is_same_v<Engine, ShmEngine>) {
            // std::cout << "[PID " << getpid() << "] _receiver_thread (SHM) has started." << std::endl;

            uint64_t next_sequence_to_read = 1;

            while (_running) {

                auto* slot = Engine::receive_zerocopy(next_sequence_to_read);

                if (slot) {

                    Protocol_Number proto = ntohs(slot->frame.header.type);

                    next_sequence_to_read++;

                    // Create the non-owning "view" buffer that points to the SHM slot
                    FrameBuffer* buffer = new FrameBuffer(&slot->frame, slot->sequence_id);
                    
                    
                    // Filters out messages that are not from the same quadrant.
                    if (filter_location(buffer)) {
                        free(buffer);
                        return;
                    }

                    // Notify the upper layers with the view buffer.
                    if (!this->notify(proto, buffer)) {
                        // If no observer handles it, we just delete the wrapper object. (we don't need to worry about the data).
                        free(buffer);
                    }

                } else if (!_running) {
                    std::cout << "NIC receiver thread stopping as requested." << std::endl;

                } else {
                    std::cout << "NIC receiver thread: receive_zerocopy returned null, retrying..." << std::endl;
                }
            }
        }
    }

    /**
     * @brief Filters the message location according to its quadrant. We only read messages from the same quadrants as ourselves.
     */
    bool filter_location(FrameBuffer* buffer) {

        Ethernet::Frame* frame = buffer->data();
        Packet* packet = reinterpret_cast<Packet*>(frame->data);
        uint packet_length = frame->data_length;

        return false;
        // todo: where is the location going to be (the quadrant?) -> Packet? Segment? what makes the most sense?
        // and then do something like if (message_quadrant != _quadrant) {return true;} return false;

    }

};

#endif // NIC_HPP