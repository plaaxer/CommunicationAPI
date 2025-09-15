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

// Network defs
public:
    typedef typename SmartData::Packet SmartPacket;
    typedef typename LatencyTest::Packet LatencyPacket;


// Unit defs
public:
    typedef typename LocalSmartData::ValueType ValueType;
    typedef unsigned int DeviceId;
    typedef std::string DeviceName;

public:
    Component(std::string name, unsigned int id)
        : _device_name(name),
          _device_id(id),
          _nic(),
          _communicator(&LocalProtocol::instance(), Address(_nic.address(), registerAndGetPort())),
          _running(true)
    {
        std::cout << "--- Starting Component: " << _device_name << " ---" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        LocalProtocol::instance().init_component(&_nic);
        _receiver_thread = std::thread(&Component::receiver_loop, this);
        _send_thread = std::thread(&Component::active_send, this);
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
     * @brief Active shared memory send routine
     */
    void active_send()
    {
        try {

            // Counter for determining the number of the packet being sent. Every latency_test_freq packets sent, one needs to be a latency_test packet
            int packets_sent_count = 0;
            int latency_test_freq = 5;

            while (_running) {

                // std::cout << "ACTIVE SEND WAITING" << std::endl;
                
                // We do this to avoid an overlap of processes being logged (as much as possible)
                std::this_thread::sleep_for(std::chrono::seconds(random_between(1, 3))); 
                
                // The envelope_packet will serve as a capsule for an actual data packet: Smart Data, or Latency Test. The header of this 
                // envelope_packet is used to indicate which of the two kinds of packet it contains. 
                PacketEnvelope::Packet envelope_packet;
                
                // Logic for deciding to send a Smart Data, or Latency Test packet:

                // Every latency_test_freq packets sent by this component, one will be a Latency Test. All the others will be Smart Data packets.
                if (packets_sent_count % latency_test_freq != 0) 
                {
                    // 1. Fetches the current time
                    auto now = std::chrono::steady_clock::now();

                    // 2. Casts the current time into a uint64_t timestamp
                    uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch()).count();

                    // 3. Builds Latency Test packet of type PING: this component will ping another component, and wait for an ECHO from it
                    LatencyTest::Header latency_header(LatencyTest::Type::PING);
                    LatencyPacket latency_packet(latency_header, timestamp);
                    
                    // 4. Builds the Packet Envelope
                    
                    // 4.1. Builds the Envelope's header, which will say the packet being sent is a Latency Test, not a Smart Data packet
                    envelope_packet.header = PacketEnvelope::Header(PacketEnvelope::MessageType::LATENCY);
                    
                    // 4.2. Copies the Latency Packet to the Envelope's payload
                    std::memcpy(envelope_packet.get_data(), &latency_packet, latency_packet.size());
                }

                // Normal case, where a SmartData Packet is sent, and not a Latency Test
                else {                     
                    // 1. Creates the SmartData Packet's header, and obtains the value from the smart component (transducer)
                    SmartData::Header smart_header(_device_name, "int");
                    ValueType smart_value = _smart_component;
                    
                    // 2. Creates SmartData packet (smart_packet) with the header and value obtained previously
                    SmartPacket smart_packet(smart_header, smart_value);
                    
                    // 3. Builds the Envelope's header, which will say the packet being sent is a Smart Data Packet, not a Latency Test
                    envelope_packet.header = PacketEnvelope::Header(PacketEnvelope::MessageType::SMART_DATA);

                    // 4. Copies the SmartData packet inside the Message data payload (PacketEnvelope)
                    std::memcpy(envelope_packet.get_data(), &smart_packet, smart_packet.size());
                }
                
                Message message_to_send(static_cast<int>(envelope_packet.size()));

                // 2.3 Copies the packet inside the Message payload
                std::memcpy(message_to_send.data(), &envelope_packet, envelope_packet.size());

                // 2.4 Set Message addressing
                message_to_send.set_source(_communicator.address());
                /*
                The destination adressing should have an external outgoing redirect later.
                What that means: a component should be able to send data to an interested external
                source (other AV/VM), but obviously, the gateway is charged with doing this.
                It's also obvious that we will need fields and commands to allow this logic.
                FUTURE TASKS!!!  
                */

                // ATTENTIIONNNNNNNNNNNNNNNNNNNNNNNNN
                // port lookup only works locally for now. You can't and won't try finding out the port
                // of a component of another vm before sending the message. Wouldn't even make sense.

                // inter vm broadcasting needs to be done with static ports (as commented just below):

                message_to_send.set_destiny(Address::broadcast(1000));

                // summing up: if Address::local(), use the lookup. If not, it must be "static"

                uint16_t port = _nic.lookupByType(getTestingType());

                if (port == 0) {
                    std::cout << "[ERROR] PORT NOT FOUND FOR TYPE" << getTestingType() << std::endl;
                }

                //message_to_send.set_destiny(Address::local(port));

                // Only to debug.
                // std::cout << "[Component " << _device_id << "] Sending: \n" << *packet
                //           << "[Source MAC]: " << message_to_send.source().paddr() << std::endl
                //           << "[Destiny MAC]: " << message_to_send.destiny().paddr() << "\n\n";

                // Use this summarized version for real and clean log
                std::cout << "[Component " << _device_name << "] sending packet to port " << port << "..." << std::endl; 

                _communicator.send(&message_to_send);

                packets_sent_count++;

                std::this_thread::sleep_for(std::chrono::seconds(15));
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Error during sending: " << e.what() << std::endl;
        }
    }

    template<typename PacketType>
    void send_packet(const PacketType& packet)
    {
        Message msg(static_cast<int>(packet.size()));
        std::memcpy(msg.data(), &packet, packet.size());
        msg.set_source(_communicator.address());
        msg.set_destiny(Address::broadcast(9090));
        std::cout << "[Component " << _device_name << "] sending packet..." << std::endl;
        _communicator.send(&msg);
    }

    /**
     * @details
     * TODO: Maybe it is better to leave the definition of this method
     * for the concrete instances. To think about what the Component must
     * do with the data received. Reply a received request? Maybe.
     * For now, we will only print as a receive log.
     */
    void receiver_loop()
    {
        // std::cout << "[" << _device_name << " Thread] Receiver loop started." << std::endl;

        // Always listening the shared memory channel
        while (_running) {

            Message received_message(1000);  // TO STANDARDIZE THE LENGTH
            
            // 1. Listen in the communicator end-point
            if (_communicator.receive(&received_message)) {
                
                // 1.1 Unpacking the source address
                Address src_addr = received_message.source();
                
                PacketEnvelope::Packet *envelope_packet = 
                    reinterpret_cast<PacketEnvelope::Packet*>(received_message.data());

                PacketEnvelope::MessageType message_type = envelope_packet->header.get_msg_type();

                // latency test received
                if (message_type == PacketEnvelope::MessageType::LATENCY) {
                    
                    LatencyPacket *l_packet = reinterpret_cast<LatencyPacket*>(envelope_packet->get_data());

                    // Receives the type of latency packet, which can be a PING or an ECHO
                    LatencyTest::Type l_packet_type = l_packet->get_header().type;

                    std::cout << "[DEBUG]: Received Latency-Test message" << std::endl;

                    // 1.3.1 Option 1: Component is receiving a PING, so it has to send back an ECHO to confirm it received the message
                    if (l_packet_type == LatencyTest::Type::PING) {
                        std::cout << "DEBUG: Received Latency-Test message of type PING" << std::endl;
                        
                        // sending echo to the source
                        send_echo(src_addr, envelope_packet);
                    }

                    // 1.3.1 Option 2: Component is receiving an ECHO, so it can compare the timestamp of when it received the ECHO, with the timestamp within the packet, which marks when the PING packet was first sent
                    else if (l_packet_type == LatencyTest::Type::ECHO) {
                        std::cout << "DEBUG: Received Latency-Test message of type ECHO" << std::endl;
                        
                        // computing RTT and printing
                        compute_rtt(l_packet);
                        
                    }
                }
                // smart data received
                else if (message_type == PacketEnvelope::MessageType::SMART_DATA)
                {
                    SmartPacket *sd_packet = reinterpret_cast<SmartPacket*>(envelope_packet->get_data());

                    // 1.2 Printing
                    print_received_packet(&src_addr, sd_packet);
    
                    // Further treatment..? Replies? -> We will need plus API implementation
                }
            }
        }

    }

    /**
     * @brief
     * sends an echo latency-test message back to the source_address which it received the latency-test request from
     */ 
    void send_echo(Address dst_addr, PacketEnvelope::Packet* env_packet)
    {
        LatencyTest::Packet* latency_payload = reinterpret_cast<LatencyTest::Packet*>(env_packet->get_data());

        // echo is the ping response
        latency_payload->header.type = LatencyTest::ECHO;

        Message message_to_send(static_cast<int>(env_packet->size()));

        // Copying the Envelope Packet inside the Message payload
        std::memcpy(message_to_send.data(), env_packet, env_packet->size());

        // Setting Message addressing
        message_to_send.set_source(_communicator.address());
        
        // Like in active_send(), the destination address is currently the broadcast address. If we want to test shared memory communication (and consequently shared memory latency), we need to set the address to local.
        message_to_send.set_destiny(Address::broadcast(dst_addr.port()));  // Broadcast with specific port? Or specific mac w/ specific port?

        // Only to debug.
        // std::cout << "[Component " << _device_id << "] Sending: \n" << *packet
        //           << "[Source MAC]: " << message_to_send.source().paddr() << std::endl
        //           << "[Destiny MAC]: " << message_to_send.destiny().paddr() << "\n\n";

        // Use this resumed version for real and clean log
        std::cout << "[Component " << _device_name << "] sending packet..." << std::endl; 

        _communicator.send(&message_to_send);
    }

    /**
    * @brief Calculates the RTT with the timestamp inside the packet received (the one that was previously sent)
    */
    void compute_rtt(LatencyPacket* l_packet) 
    {
        // 1. Fetches the current time
        auto now = std::chrono::steady_clock::now();

        // 2. Casts the current time into a uint64_t timestamp
        uint64_t current_timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
            
        // 3. Calculates the difference between timestamps
        uint64_t payload_timestamp = l_packet->timestamp;

        uint64_t rtt_ns = current_timestamp - payload_timestamp;  // rtt in nanoseconds

        double rtt_seconds = rtt_ns / 1e9;  // double is used because now it is not a integer
        
        // 4. Prints out a log. This will change in favor of a statistics-oriented approach. For now, just logging.
        std::cout << "[Latency Test]: " << rtt_seconds << " s!" << std::endl;
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
    LocalNIC _nic;
    Communicator<LocalProtocol> _communicator;  // network API end-point
    LocalSmartData _smart_component;  // component w/ SmartData API

    uint16_t _port;

    std::thread _receiver_thread;
    std::thread _send_thread;
    std::atomic<bool> _running; // flag to control whether the thread should keep running


};

#endif  // COMPONENT_HPP