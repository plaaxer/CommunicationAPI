#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "api/network/engines/smh_engine.hpp"
#include "api/network/communicator.hpp"
#include "api/network/protocol.hpp"
#include "api/network/nic.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/latency_test.hpp"

#include "api/network/packet_envelope.hpp"


#include "vehicle/smartdata/smart_data.hpp"
#include "vehicle/smartdata/data_generator.hpp"
#include "vehicle/smartdata/local_smartdata.hpp"

#include "utils/random.cpp"

#include <chrono>

using LocalNIC = NIC<ShmEngine>;
using LocalProtocol = Protocol<NIC<ShmEngine>>;

/**
 * @brief Component blueprint with common routines
 */
template<typename LocalSmartData>
class Component : SmartData, LatencyTest
{

// Component types
public:
    enum TransducerType {
        ACTUATOR,
        SENSOR
    };

// Network defs
public:
    typedef typename SmartData::Packet SmartPacket;
    typedef typename LatencyTest::Packet LatencyPacket;


// Unit defs
public:
    typedef typename LocalSmartData::ValueType ValueType;
    typedef unsigned int DeviceId;
    typedef uint64_t SenderId;
    typedef std::string DeviceName;

public:
    Component(std::string name, unsigned int id, TransducerType trans_type, TEDS::Period interval_ms)
        : _device_name(name),
          _device_id(id),
          _nic(),
          _communicator(&LocalProtocol::instance(), Address(_nic.address(), registerAndGetPort())),
          _running(true)
          
    {
        std::cout << "--- Starting Component: " << _device_name << " ---" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Setting sender id with random number + device_name (do not pick only device_name!!)
        uint64_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count() + getpid();
        uint64_t name_hash = std::hash<std::string>{}(_device_name);
        _sender_id = name_hash ^ seed;

        LocalProtocol::instance().init_component(&_nic);
        _receiver_thread = std::thread(&Component::receiver_loop, this);
        _send_thread = std::thread(&Component::active_send, this);

        switch(type)
        {
            // Subscribes the component to receive responses of a specific type of data it requests
            case ACTUATOR:
                Communicator::subscribe_to_responses(TEDS::Type type, TEDS::Period interval_ms);
                break;

            // Subscribes the component to requests, and provides responses to components requesting that specific kind of data
            case SENSOR:
                Communicator::subscribe_to_requests();
                break;
        }



    }



    ~Component()
    {
        _running = false;

        if (_receiver_thread.joinable()) {
            _receiver_thread.join();
        }

        if (_send_thread.joinable()) {
            _send_thread.join();
        }
    }

private:

    /**
     * @brief Encapsulate an application packet inside a Packet Envelope
     */
    template<typename PacketType>
    PacketEnvelope::Packet create_envelope(const PacketType& packet, PacketEnvelope::MessageType type)
    {
        PacketEnvelope::Packet envelope;

        uint32_t sz = static_cast<uint32_t>(packet.size());

        envelope.header = PacketEnvelope::Header(type, sz);

        envelope.payload.resize(sz);

        std::memcpy(envelope.payload.data(), &packet, sz);

        return envelope;
    }

    /**
     * @brief Sends the Packet Envelope with the application packet using the communication API
     */    
    void send_envelope(const PacketEnvelope::Packet& envelope, Address dst)
    {
        Message msg(static_cast<int>(envelope.total_size()));
        envelope.serialize(msg.data());

        msg.set_source(_communicator.address());
        msg.set_destination(dst);

        // std::cout << "[Component " << _device_name << "] sending packet to port "
        //           << dst.port() << "..." << std::endl;

        _communicator.send(&msg);
    }


    void receiver()
    {
        while(_running) {
            Message msg(1024);
            
            if (_communicator.receive(&received_message)) {
                Segment::MsgType type = msg.get_type();
            }
            

        }

    }


    void subscribe_to_responses(TEDS::Type type_id, TEDS::Period interval_ms = 1000)
        subscribe_to_responses()

    }


    /**
     * @brief Active shared memory send routine. Also has a ping routine to comport
     * future statistics.
     */
    void active_send()
    {
        try {
            // Counter for determining the number of the packet being sent. Every latency_test_freq packets sent, one needs to be a latency_test packet
            int packets_sent_count = 0;
            int latency_test_freq = 3;

            while (_running) {
                {


                    
                    // 1. Fetches the current time
                    auto now = std::chrono::steady_clock::now();

                    // 2. Casts the current time into a uint64_t timestamp
                    uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch()).count();

                    // 3. Builds Latency Test packet of type PING: this component will ping another component, and wait for an ECHO from it
                    LatencyTest::Header latency_header(LatencyTest::Type::PING, _sender_id);
                    LatencyPacket latency_packet(latency_header, timestamp);



                }

            
                // // Developers note!!
                // // port lookup only works locally for now. You can't and won't try finding out the port
                // // of a component of another vm before sending the message. Wouldn't even make sense.
                // // inter vm broadcasting needs to be done with static ports (as commented just below):
                // message_to_send.set_destiny(Address::broadcast(1000));

                // // summing up: if Address::local(), use the lookup. If not, it must be "static"

                // uint16_t port = _nic.lookupByType(getTestingType());

                // if (port == 0) {
                //     std::cout << "[ERROR] PORT NOT FOUND FOR TYPE" << getTestingType() << std::endl;
                // }

                // Address dst = Address::local(port);

                uint16_t port = 1000;  // read above. comment this piece of code if wanted, uncommenting the above.

                Address dst = Address::broadcast(port);

                send_envelope(envelope_packet, dst);
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Error during sending: " << e.what() << std::endl;
        }
    }

    /**
     * @details
     * Receives application messages from the Communicator. Currently receives data from
     * SmartData core API and from LatencyTest packets.
     */
    void receiver_loop()
    {
        // std::cout << "[" << _device_name << " Thread] Receiver loop started." << std::endl;

        // always listening the shared memory channel
        while (_running) {

            Message received_message(1000);  // TO STANDARDIZE THE LENGTH
            
            // 1. Listen in the communicator end-point
            if (_communicator.receive(&received_message)) {
                
                // 1.1 Unpacking the source address
                Address src_addr = received_message.source();
                
                // Parse the envelope from raw bytes; avoid treating raw bytes as a C++ object
                PacketEnvelope::Packet envelope_packet = PacketEnvelope::Packet::from_buffer(
                    received_message.data(), received_message.size());

                PacketEnvelope::MessageType message_type = envelope_packet.header.get_msg_type();

                // latency test received
                if (message_type == PacketEnvelope::MessageType::LATENCY) {
                    // std::cout << "[DEBUG]: Received Latency-Test message" << std::endl;

                    LatencyTest::Header lhdr{};
                    std::memcpy(&lhdr, envelope_packet.get_data(), sizeof(LatencyTest::Header));

                    // receives the type of latency packet, which can be a PING or an ECHO
                    LatencyTest::Type l_packet_type = lhdr.type;
                    
                    SenderId s_id = lhdr.sender_id;  // ping's sender

                    // 1.3.1 Option 1: Component is receiving a PING, so it has to send back an ECHO to confirm it received the message
                    if (l_packet_type == LatencyTest::Type::PING) {
                        //std::cout << "[DEBUG]: Received Latency-Test message of type PING" << std::endl;
                        
                        // extract timestamp from payload and send echo back
                        if (envelope_packet.payload.size() >= sizeof(LatencyTest::Header) + sizeof(LatencyTest::Timestamp)) {
                            LatencyTest::Timestamp ts = 0;

                            // // TO DEBBUG!!
                            // LatencyTest::Packet latency_packet(lhdr);
                            // std::memcpy(&latency_packet, envelope_packet.get_data(), sizeof(LatencyTest::Packet));
                            // print_full_latency_packet(received_message, latency_packet);

                            // accesses the timestamp from the latency_test packet, by converting the payload of envelope_packet (which, in this case, contains a latency_test packet) into bytes, and skipping the latency_test packet's header, accessing the Timestamp, which is the other portion of the latency_test packet
                            std::memcpy(&ts, static_cast<const uint8_t*>(envelope_packet.get_data()) + sizeof(LatencyTest::Header), sizeof(LatencyTest::Timestamp));
                            
                            send_echo(src_addr, ts, s_id);
                        } 
                        else {
                            std::cerr << "[ERROR] Latency PING payload missing timestamp" << std::endl;
                        }
                    }

                    // 1.3.1 Option 2: Component is receiving an ECHO, so it can compare the timestamp of when it received the ECHO, with the timestamp within the packet, which marks when the PING packet was first sent
                    // also verifies the sender id, not computing RTT if itself do not has sent the echo related ping
                    else if (l_packet_type == LatencyTest::Type::ECHO &&
                             s_id == _sender_id) {

                        //std::cout << "[DEBUG]: Received Latency-Test message of type ECHO" << std::endl;
                        
                        // extract timestamp and compute RTT
                        if (envelope_packet.payload.size() >= sizeof(LatencyTest::Header) + sizeof(LatencyTest::Timestamp)) {
                            LatencyTest::Timestamp ts = 0;

                            // TO DEBBUG!!
                            // LatencyTest::Packet latency_packet(lhdr);
                            // std::memcpy(&latency_packet, envelope_packet.get_data(), sizeof(LatencyTest::Packet));
                            // print_full_latency_packet(received_message, latency_packet);

                            std::memcpy(&ts, static_cast<const uint8_t*>(envelope_packet.get_data()) + sizeof(LatencyTest::Header), sizeof(LatencyTest::Timestamp));
                            
                            compute_rtt(ts);

                        } else {
                            std::cerr << "[ERROR] Latency ECHO payload missing timestamp" << std::endl;
                        }
                    }
                }
                // smart data received
                else if (message_type == PacketEnvelope::MessageType::SMART_DATA)
                {
                    // std::cout << "[DEBUG] Component has received a SmartData packet" << std::endl;
                    SmartPacket sd_packet{};

                    std::memcpy(&sd_packet, envelope_packet.get_data(), sizeof(SmartPacket));

                    //print_received_packet(&src_addr, &sd_packet);
    
                    // Further treatment..? Replies? -> We will need plus API implementation
                }
            }
        }

    }

    /**
     * @brief sends an echo latency-test message back to the source_address which it received the latency-test request from
     */ 
    void send_echo(Address dst_addr, LatencyTest::Timestamp ts, SenderId s_id)
    {
        // build an echo response with the same timestamp
        LatencyPacket echo_pkt(LatencyTest::Header(LatencyTest::ECHO, s_id), ts);
        auto envelope = create_envelope(echo_pkt, PacketEnvelope::MessageType::LATENCY);

        Message message_to_send(static_cast<int>(envelope.total_size()));
        envelope.serialize(message_to_send.data());

        // setting Message addressing
        message_to_send.set_source(_communicator.address());
        
        // like in active_send(), the destination address is currently the broadcast address. If we want to test shared memory communication (and consequently shared memory latency), we need to set the address to local.
        message_to_send.set_destination(Address::broadcast(dst_addr.port()));  // Broadcast with specific port? Or specific mac w/ specific port?

        _communicator.send(&message_to_send);
    }

    /**
    * @details
    * Calculates the RTT with the timestamp inside the packet received (the one that was previously sent).
    * We will use this method to make a better statistics handling later.
    */
    void compute_rtt(LatencyTest::Timestamp payload_timestamp) 
    {
        // 1. Fetches the current time
        auto now = std::chrono::steady_clock::now();

        // 2. Casts the current time into a uint64_t timestamp
        uint64_t current_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
            
        // std::cout << "[RTT DEBUG] Current Timestamp: " << current_timestamp << " ns" << std::endl;
        // std::cout << "[RTT DEBUG] Payload Timestamp: " << payload_timestamp << " ns" << std::endl;

        // 3. Calculates the difference between timestamps
        uint64_t rtt_ns = current_timestamp - payload_timestamp;  // rtt in nanoseconds

        double rtt_ms = rtt_ns / 1e6;  // double is used because now it is not a integer
        
        // 4. Prints out a log. This will change in favor of a statistics-oriented approach. For now, just logging.
        std::cout << "[Computed Latency]: " << rtt_ms << " ms!" << std::endl;
    }

    /**
     * @brief Log SmartData packet received. This logging is not safe, other processes might interfere with it.
     * THIS WILL CONSTANLY BE IMPROVED TO ADD NEW INFORMATION
     */
    void print_received_packet(Address *src_addr, SmartPacket *sd_packet)
    {
        std::cout << "----- <<<" << _device_name << ">>>" << " has received a packet! -----" << std::endl;

        std::cout << "[Origin MAC]: " << src_addr->paddr() << std::endl
                  << "[Origin Port]: " << src_addr->port() << std::endl
                  << "[Our Port]: " << _port << std::endl;

        // operator << already overriden
        std::cout << *sd_packet;

        std::cout << "--------------end received packet--------------" << std::endl;

    }

    /**
     * @brief Prints the full frame details, including MAC addresses from the Message
     * and the payload from the LatencyTest::Packet.
     */
    void print_full_latency_packet(const Message& msg, const LatencyTest::Packet& packet)
    {
        std::cout << "--- Full Latency Packet Received ---" << std::endl;
        std::cout << "[Source MAC]:      " << msg.source().paddr() << std::endl;
        std::cout << "[Destination MAC]: " << msg.destination().paddr() << std::endl;
        std::cout << "--- Packet Payload ---" << std::endl;
        // This reuses the operator<< we already defined for LatencyTest::Packet
        std::cout << packet << std::endl; 
        std::cout << "------------------------------------" << std::endl;
    }


    /**
     * @brief Register a component on the directory at the shared memory and gets a port assigned.
     * @details Currently we access the nic directly to do so (and to do the lookup). Maybe we can 
     * add that to the communicator stack instead.
     */
    uint16_t registerAndGetPort() {
        uint32_t type_id = typeid(ValueType).hash_code();
        uint16_t dynamic_port = _nic.registerComponentService(_device_name, type_id);
        if (dynamic_port == 0) {
            throw std::runtime_error("Failed to register component " + _device_name);
        }
        std::cout << "[Component " << _device_name << "] registered with Port: " << dynamic_port << std::endl;

        _port = dynamic_port;

        return dynamic_port;
    }
    
    // temp, for testing.
    // returns the port for the type "int" iirc. For now the only type of component here.
    // this value comes from uint32_t type_id = typeid(ValueType).hash_code(); on the method above
    uint32_t getTestingType() {
        return 3061177862;
    }

private:
    DeviceName _device_name;
    DeviceId _device_id;
    SenderId _sender_id;
    LocalNIC _nic;
    Communicator<LocalProtocol> _communicator;  // network API end-point
    LocalSmartData _smart_component;  // component w/ SmartData API

    uint16_t _port;

    std::thread _receiver_thread;
    std::thread _send_thread;
    std::atomic<bool> _running; // flag to control whether the thread should keep running

};

#endif  // COMPONENT_HPP