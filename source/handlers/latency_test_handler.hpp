#ifndef LATENCY_TEST_HANDLER_HPP
#define LATENCY_TEST_HANDLER_HPP

#pragma once
#include "handlers/i_handler.hpp"
#include "api/network/definitions/latency_test.hpp"
#include "api/network/control_envelope.hpp"
#include "api/network/definitions/ethernet.hpp"


#include <iostream>
#include <chrono>

class LatencyTestHandler : public IControlHandler {
public:
    using SenderId = uint64_t;

    LatencyTestHandler(SenderId my_sender_id)
        : _my_sender_id(my_sender_id)
    {}

    virtual void handleControlMessage(Communicator<LocalProtocol>& comm, const Message& msg) override
    {

        ControlEnvelope::Packet envelope_packet = ControlEnvelope::Packet::from_buffer(
            msg.data(), msg.size());
        
        ControlEnvelope::ControlType message_type = envelope_packet.header.get_type();

        if (message_type == ControlEnvelope::ControlType::LATENCY) {
            LatencyTest::Header lhdr{};
            std::memcpy(&lhdr, envelope_packet.get_data(), sizeof(LatencyTest::Header));

            LatencyTest::Type l_packet_type = lhdr.type;
            SenderId s_id = lhdr.sender_id;

            if (l_packet_type == LatencyTest::Type::PING) {
                std::cout << "\n[Latency Handler] Received PING from " 
                          << msg.source() << std::endl;

                LatencyTest::Timestamp ts = 0;
                std::memcpy(&ts, static_cast<const uint8_t*>(envelope_packet.get_data()) + sizeof(LatencyTest::Header), sizeof(LatencyTest::Timestamp));
                
                Address dst;
                if (msg.source().paddr() == Ethernet::LOCAL_ADDR) {
                    dst = Address::local(1000);
                } else {
                    dst = Address::broadcast(1000);
                }

                send_echo(comm, dst, ts, s_id);

            } else if (l_packet_type == LatencyTest::Type::ECHO && s_id == _my_sender_id) {
                std::cout << "\n[Latency Handler] Received ECHO from " 
                          << msg.source() << std::endl;
                LatencyTest::Timestamp ts = 0;
                std::memcpy(&ts, static_cast<const uint8_t*>(envelope_packet.get_data()) + sizeof(LatencyTest::Header), sizeof(LatencyTest::Timestamp));
                
                compute_rtt(ts, msg.source());
            } // else  {
                // std::cout << "[Latency Handler] Received LATENCY message with unknown type or mismatched sender ID!" << std::endl;
            // }
        }
        // else: handle other CONTROL message types...
    }


    /**
     * @brief Sends a PING message.
     */
    void send_ping(Communicator<LocalProtocol>& comm, Address dest) override
    {
        auto now = std::chrono::steady_clock::now();

        LatencyTest::Timestamp ts = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();

        std::cout << "[Latency Handler] Sending PING to " << dest << std::endl;

        using LatencyPacket = LatencyTest::Packet;

        LatencyPacket ping_pkt(LatencyTest::Header(LatencyTest::PING, _my_sender_id), ts);
        
        auto envelope = ControlEnvelope::create_envelope(
            ping_pkt, ControlEnvelope::ControlType::LATENCY);

        // Serialize envelope into a vector
        std::vector<char> control_payload(envelope.total_size());
        envelope.serialize(control_payload.data());

        // Use the new Message constructor
        Message ping_msg(dest, control_payload);
        ping_msg.set_source(comm.address());
        
        comm.send(&ping_msg);
    }

private:

    void send_echo(Communicator<LocalProtocol>& comm, Address dst_addr, LatencyTest::Timestamp ts, SenderId s_id)
    {
        std::cout << "[Latency Handler] Sending ECHO to " << dst_addr << std::endl;
        using LatencyPacket = LatencyTest::Packet;

        LatencyPacket echo_pkt(LatencyTest::Header(LatencyTest::ECHO, s_id), ts);

        auto envelope = ControlEnvelope::create_envelope(
                echo_pkt, ControlEnvelope::ControlType::LATENCY);

        // Serialize envelope into a vector
        std::vector<char> control_payload(envelope.total_size());
        envelope.serialize(control_payload.data());

        // setting Message addressing
        Message message_to_send(dst_addr, control_payload);
        message_to_send.set_source(comm.address());

        comm.send(&message_to_send);
    }


    /**
    * @details
    * Calculates the RTT with the timestamp inside the packet received (the one that was previously sent).
    * We will use this method to make a better statistics handling later.
    */
    void compute_rtt(LatencyTest::Timestamp payload_timestamp, Address source) 
    {
        // 1. Fetches the current time
        auto now = std::chrono::steady_clock::now();
                
        // 2. Casts the current time into a uint64_t timestamp
        uint64_t current_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();

        // std::cout << "[RTT DEBUG] Current Timestamp: " << current_timestamp << " ns" << std::endl;
        // std::cout << "[RTT DEBUG] Payload Timestamp: " << payload_timestamp << " ns" << std::endl;

        // 3. Calculates the difference between timestamps
        uint64_t rtt_ns = current_timestamp - payload_timestamp;
        double rtt_ms = rtt_ns / 1e6;
        std::cout << "[Computed Latency]: " << rtt_ms 
                  << " ms! | Echo origin: " << source << std::endl;
    }

    SenderId _my_sender_id;

};

#endif  // LATENCY_TEST_HANDLER.HPP