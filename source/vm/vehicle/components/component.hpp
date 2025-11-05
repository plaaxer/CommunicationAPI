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

#include "vm/vehicle/smartdata/data_generator.hpp"
#include "vm/vehicle/smartdata/local_smartdata.hpp"

#include "vm/vehicle/components/i_component_bridge.hpp"

#include "utils/random.cpp"
#include <algorithm> // for std::max

#include <chrono>

using LocalNIC = NIC<ShmEngine>;
using LocalProtocol = Protocol<NIC<ShmEngine>>;

enum TransducerType {
        ACTUATOR,
        SENSOR
};

/**
 * @brief Component blueprint with common routines
 */
template<typename LocalSmartData>
class Component : public IComponentBridge<LocalSmartData>
{

// Unit defs
public:
    typedef typename TEDS::Period Period;
    typedef typename LocalSmartData::Value Value;
    typedef unsigned int DeviceId;
    typedef uint64_t SenderId;
    typedef std::string DeviceName;

public:
    Component(std::string name, TransducerType transducer_type, TEDS::Period interval_ms)
        : _device_name(name),
          _nic(),
          _communicator(&LocalProtocol::instance(), Address(_nic.address(), registerAndGetPort())),
          _running(true),
          _response_running(false),
          _responses_interval(0)
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
        _tedsHandler = std::make_unique<TEDSHandler<LocalSmartData>>(*this);
        _controlHandler = std::make_unique<LatencyTestHandler>(_sender_id);

        switch(transducer_type)
        {
            // Subscribes the component to receive responses of a specific type of data it requests
            case ACTUATOR:
                _communicator.subscribe_to_responses(LocalSmartData::WrappedTransducer::UnitTagType, interval_ms);
                break;

            // Subscribes the component to requests, and provides responses to components requesting that specific kind of data
            case SENSOR:
                _communicator.subscribe_to_requests(LocalSmartData::WrappedTransducer::UnitTagType);
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

    void apply_value_from_payload(const std::vector<char>& payload) override
    {
        auto* response = reinterpret_cast<const TEDS::ResponsePayload*>(
            payload.data() + sizeof(TEDS::Header)
        );

        Value data = response->value;

        // TOM DEBUG: data not in scope. Was it supposed to be val? And should smart_component be called that?
        _smart_component = data;  // actuator applies data
    }

    // maybe we should return int for status later
    void notify_interest_request(Period requested_interval, TEDS::Type type) override
    {
        if (_responses_interval != requested_interval) {
            // GCD between the new requested and the already setted
            Period new_interval = std::gcd(_responses_interval.load(), requested_interval);
            
            _responses_interval = new_interval;
        }

        // TODO: HANDLE MULTIPLE TYPES TO DO RESPONSES
        _response_type = type;

        // turns on the response_thread
        if (!_response_running) {
          _response_running = true;
          _response_thread = std::thread(&Component::response_thread, this);  // only starts w/ an interest
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
        Address dst, ext_ping_dst, intra_ping_dst;
        
        // FOR NOW, WE ARE ASSIGNING THE PORT FOR ONE COMPONENT MANUALLY
        ext_ping_dst = Address::broadcast(1000);
        intra_ping_dst = Address::local(1000);

        bool external_ping = true;
        while (_running) {
            std::this_thread::sleep_for(std::chrono::seconds(random_between(2, 3)));

            dst = external_ping ? ext_ping_dst : intra_ping_dst;

            if (std::rand() % 6 ) {
                _controlHandler->send_ping(_communicator, dst);
                external_ping = !external_ping;
            } 
        }
    }

    void response_thread()
    {
        // only activates with the first interest/request received
        while (_response_running) {

            _tedsHandler->send_response(_communicator, Address::broadcast(LocalProtocol::TYPE_BASED_ROUTING_PORT), _response_type.load());

            std::this_thread::sleep_for(std::chrono::milliseconds(_responses_interval));
        }
    }

    /**
     * @brief Register a component on the directory at the shared memory and gets a port assigned.
     * @details Currently we access the nic directly to do so (and to do the lookup). Maybe we can 
     * add that to the communicator stack instead.
     */
    uint16_t registerAndGetPort() {
        uint32_t type_id = typeid(Value).hash_code();
        uint16_t dynamic_port = _nic.registerComponentService(_device_name, type_id);
        if (dynamic_port == 0) {
            throw std::runtime_error("Failed to register component " + _device_name);
        }
        // std::cout << "[Component " << _device_name << "] registered with Port: " << dynamic_port << std::endl;

        _port = dynamic_port;

        return dynamic_port;
    }
    
    // temp, for testing.
    // returns the port for the type "int" iirc. For now the only type of component here.
    // this value comes from uint32_t type_id = typeid(Value).hash_code(); on the method above
    uint32_t getTestingType() {
        return 3061177862;
    }

    Value get_value() override
    {
        Value data = _smart_component;  // sensor data through the API
        return data;
    }

private:
    DeviceName _device_name;
    LocalNIC _nic;
    Communicator<LocalProtocol> _communicator;  // network API end-point
    LocalSmartData _smart_component;  // component w/ SmartData API

    uint16_t _port;

    SenderId _sender_id;

    std::thread _receiver_thread;
    std::thread _send_thread;  // send pings
    std::thread _response_thread;  // send interest responses

    std::atomic<bool> _running; // flag to control whether the thread should keep running
    std::atomic<bool> _response_running;  // tell us if we already this thread running

    std::atomic<Period> _responses_interval;

    // TODO: TO MAKE GENERIC TO COMPORT MORE THAN ONE TYPE
    std::atomic<TEDS::Type> _response_type;

    std::unique_ptr<ITEDSHandler> _tedsHandler;
    std::unique_ptr<IControlHandler> _controlHandler;

};

#endif  // COMPONENT_HPP