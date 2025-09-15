#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "api/network/engines/smh_engine.hpp"
#include "api/network/communicator.hpp"
#include "api/network/protocol.hpp"
#include "api/network/nic.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"

#include "vehicle/smartdata/smart_data.hpp"
#include "vehicle/smartdata/data_generator.hpp"
#include "vehicle/smartdata/local_smartdata.hpp"

#include "utils/random.cpp"

using LocalNIC = NIC<ShmEngine>;
using LocalProtocol = Protocol<NIC<ShmEngine>>;

/**
 * @brief Component blueprint with common routines
 */
template<typename LocalSmartData>
class Component : SmartData
{

// Network defs
public:
    typedef typename SmartData::Packet Packet;

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
            while (_running) {

                // std::cout << "ACTIVE SEND WAITING" << std::endl;
                
                // we do this to avoid, as much as possible, overlapping of logging between processes
                std::this_thread::sleep_for(std::chrono::seconds(random_between(1, 3)));

                // 1. Collect the data from the SmartData API
                ValueType value = _smart_component;
                
                // 2. Building the final custom protocol Message (payload)
                
                // 2.1 Packet build
                SmartData::Header header(_device_name, "int");
                Packet* packet = new Packet(header, value);
                
                Message message_to_send(static_cast<int>(packet->size()));

                // 2.3 Copying the packet inside the Message payload
                std::memcpy(message_to_send.data(), packet, packet->size());

                // 2.4 Setting Message addressing
                message_to_send.set_source(_communicator.address());
                /*
                The destiny adressing should have an external outgoing redirect later.
                What it does mean: a component should can send data for an interested external
                font (other AV/VM), but obviously is the gateway that is charged to do this.
                Is also obvious that we will need fields and commands to allow this logic.
                FUTURE TASKS!!!  
                */

                unsigned int port = 1001;

                message_to_send.set_destiny(Address::broadcast(port));

                // Only to debug.
                // std::cout << "[Component " << _device_id << "] Sending: \n" << *packet
                //           << "[Source MAC]: " << message_to_send.source().paddr() << std::endl
                //           << "[Destiny MAC]: " << message_to_send.destiny().paddr() << "\n\n";

                // Use this resumed version for real and clean log
                std::cout << "[Component " << _device_name << "] sending packet to port " << port << "..." << std::endl; 

                _communicator.send(&message_to_send);

                std::this_thread::sleep_for(std::chrono::seconds(15));

                }
        } catch (const std::runtime_error& e) {
            std::cerr << "Error during sending: " << e.what() << std::endl;
        }
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
            Message received_message(1000);  // TO STANDARTIZE THE LENGHT
            
            // 1. Listen in the communicator end-point
            if (_communicator.receive(&received_message)) {
                // 1.1 Unpacking the Message
                Address src_addr = received_message.source();
                Packet *sd_packet = reinterpret_cast<Packet*>(received_message.data());
                
                // 1.2 Printing
                print_received_packet(&src_addr, sd_packet);
            }

            // Further treatment..? Replys? -> We will need plus API implementation
        }
    }

    /**
     * @brief Log SmartData packet received. This logging is not safe, other processes might interfere on it.
     * THIS WILL CONSTANLY BE IMPROVED TO ADD NEW INFORMATION
     */
    void print_received_packet(Address *src_addr, Packet *sd_packet)
    {
        std::cout << "----- <<<" << _device_name << ">>>" << " has received a packet! -----" << std::endl;

        std::cout << "[MAC]: " << src_addr->paddr() << std::endl
                  << "[Port]: " << src_addr->port() << std::endl
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