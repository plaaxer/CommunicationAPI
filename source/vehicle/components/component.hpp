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
#include "api/network/definitions/teds.hpp"

#include "api/network/control_envelope.hpp"

#include "handlers/i_handler.hpp"
#include "handlers/teds_handler.hpp"
#include "handlers/latency_test_handler.hpp"


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

// Types of transducer the component can be
public:
    enum TransducerType {
        ACTUATOR,
        SENSOR
    };

// Network defs
public:
    typedef typename LatencyTest::Packet LatencyPacket;

// Unit defs
public:
    typedef typename LocalSmartData::ValueType ValueType;
    typedef unsigned int DeviceId;
    typedef uint64_t SenderId;
    typedef std::string DeviceName;

public:
    Component(std::string name, unsigned int id, TransducerType transducer_type, TEDS::Period interval_ms)
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

        // creates the concrete handlers and pass them their dependencies
        _tedsHandler = std::make_unique<TEDSHandler>(_smart_component);
        _controlHandler = std::make_unique<LatencyTestHandler>(_sender_id);

        switch(transducer_type)
        {
            // Subscribes the component to receive responses of a specific type of data it requests
            case ACTUATOR:
                _communicator.subscribe_to_responses(LocalSmartData::WrappedTransducer::UnitTag, interval_ms);
                break;

            // Subscribes the component to requests, and provides responses to components requesting that specific kind of data
            case SENSOR:
                _communicator.subscribe_to_requests(LocalSmartData::WrappedTransducer::UnitTag);
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
     * @brief Waits for messages and redirects for the right handler
     */
    void receiver_loop()
    {
        // std::cout << "[" << _device_name << " Thread] Receiver loop started." << std::endl;

        while (_running) {
            Message msg(1024); // Allocate buffer for receiving

            if (_communicator.receive(&msg)) {
                
                std::cout << "\n[Component] Received packet (Type: " 
                          << static_cast<int>(msg.get_type()) 
                          << ", Size: " << msg.size() << ")" << std::endl;

                switch (msg.get_type()) {
                  case Segment::MsgType::TEDS:
                        _tedsHandler->handleTEDSMessage(_communicator, msg);
                        break;
                    
                    case Segment::MsgType::CONTROL:
                        _controlHandler->handleControlMessage(_communicator, msg);
                        break;
                    
                    default:
                        std::cout << "[Component] Received unknown message type: " 
                                  << static_cast<int>(msg.get_type()) << std::endl;
                }
            }
        }
    }

    /**
     * @brief Send routine: periodical PINGs and INTEREST commands
     */
    void active_send() {
        // FOR NOW, WE ARE ASSIGNING THE PORT FOR ONE COMPONENT MANUALLY
        int control_port = 1000;
        Address control_address = Address::broadcast(control_port);

        // Address type_msg_address = Address:broadcast(LocalProtocol::TYPE_BASED_ROUTING_PORT);

        while (_running) {
            std::this_thread::sleep_for(std::chrono::seconds(random_between(2, 5)));

            if (packets_sent_count > 0 && packets_sent_count % latency_test_freq == 0) {
                // all components send PINGs
                std::cout << "\n[Component] Active Send: Sending PING." << std::endl;
                _controlHandler->send_ping(_communicator, broadcast_addr);
            
            } 
            // THE INTEREST IS ALREADY CALLED BY THE COMPONENT CONSTRUCTOR.
            // IN THE FUTURE, WE CAN MAKE ACTUATORS SEND INTERESTS PERIODICALLY IF NEEDED.
            // else if (_transducer_type == ACTUATOR) {
                // only Actuators send INTERESTs
            //     std::cout << "\n[Component] Active Send: Sending TEDS INTEREST." << std::endl;

            //     _tedsHandler->send_interest(_communicator, type_msg_address, 
            //                                 <TYPE HERE>, <INTERVALE HERE>);
            
            // } 

            packets_sent_count++;
        }
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
    LocalNIC _nic;
    Communicator<LocalProtocol> _communicator;  // network API end-point
    LocalSmartData _smart_component;  // component w/ SmartData API

    uint16_t _port;

    std::thread _receiver_thread;
    std::thread _send_thread;
    std::atomic<bool> _running; // flag to control whether the thread should keep running

    std::unique_ptr<ITEDSHandler> _tedsHandler;
    std::unique_ptr<IControlHandler> _controlHandler;

};

#endif  // COMPONENT_HPP