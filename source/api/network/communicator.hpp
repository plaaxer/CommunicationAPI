#ifndef COMMUNICATOR_HPP
#define COMMUNICATOR_HPP

#include "api/observer/observers.hpp"
#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/teds.hpp"
#include "api/network/definitions/segment.hpp"
#include <set>

/**
 * @brief End-Point for the components. It creates a communication channel with the Protocol Handler,
 * an instance of Protocol typenamed Channel here.
 */

template <typename Channel>
class Communicator : public PortObserver, public TypeObserver
{

public:

    typedef typename Channel::Buffer Buffer;

    Communicator(Channel * channel, Address address)
        : _channel(channel), _address(address)
    {

        _channel->attach_port_listener(this, address.port());
    }

    ~Communicator()
    {
        _channel->detach_port_listener(this, _address.port());

        for (const auto& type : _subscribed_types) {
            _channel->detach_type_listener(this, type);
        }
    }

    /**
     * @brief Send a message to the destination address specified in the Message object.
     */
    bool send(const Message* message) 
    {

    Segment segment(message->get_type(), message->get_payload());

    std::vector<char> serialized_segment = segment.get_bytes();

    return _channel->send(
        _address,
        message->destination(),
        serialized_segment.data(),
        serialized_segment.size()
    ) > 0;
}

    /**
     * @brief Blocking receive method. Waits for a message and fills the Message object.
     */
    bool receive(Message * message)
    {

        _semaphore.p();

        Buffer* buf = nullptr;
        
        // retrieves the buffer from the internal queue
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_data.empty()) return false; // safety check
            buf = _data.front();
            _data.pop_front();
        }

        Address from;
    
        // developer's note: for now the entire segment will be placed on the message payload,
        // later being overwritten by the actual payload after parsing.
        // this is to avoid multiple allocations and copies.
        int total_segment_size = _channel->receive(buf, &from, message->data(), message->capacity());

        _channel->free(buf);
        
        if (total_segment_size <= 0) {
            return false;
        }

        const char* raw_bytes_start = static_cast<const char*>(message->data());
        std::vector<char> segment_bytes(raw_bytes_start, raw_bytes_start + total_segment_size);

        Segment seg = Segment::from_bytes(segment_bytes);

        message->set_payload(seg.get_payload());
        message->set_type(seg.get_type());
        message->set_source(from);

        return true;
    }

    void subscribe_to_requests(TEDS::Type type_id, TEDS::Period interval_ms = 1000)
    {
        TEDS::Type request = TEDS::make_request_type(type_id);
        subscribe_to_type(request, interval_ms);
    }

    void subscribe_to_responses(TEDS::Type type_id, TEDS::Period interval_ms = 1000)
    {
        TEDS::Type response = TEDS::make_response_type(type_id);
        subscribe_to_type(response, interval_ms);
        send_interest_message(type_id, interval_ms);
    }

    const Address& address() const {
        return _address;
    }

    void update(Conditionally_Data_Observed<Buffer, Address::Port>* obs, Address::Port c, Buffer* d) override {
        queue_incoming_buffer(d);
    }

    void update(Conditionally_Data_Observed<Buffer, TEDS::Type>* obs, TEDS::Type c, Buffer* d) override {
        queue_incoming_buffer(d);
    }

private:

    /**
     * @brief Subscribes to a specific data type by attaching to the type-based routing port.
     * @param obs Pointer to the observer that will handle incoming data of the specified type.
     * @param type_id The unique identifier for the data type to subscribe to.
     */
    void subscribe_to_type(TEDS::Type type_id, TEDS::Period interval_ms = 1000)
    {
        _channel->attach_type_listener(this, type_id);
        _subscribed_types.insert(type_id);
    }

    void queue_incoming_buffer(Buffer* buf) {
        std::lock_guard<std::mutex> lock(_mutex); // RAII
        _data.push_back(buf);
        _semaphore.v(); // signal that a new message is available
    }

    /**
     * @brief Sends an interest message to the network to subscribe to a specific data type.
     * @param type_id The unique identifier for the data type.
     * @param interval_ms The interval in milliseconds for how often to receive updates of this type
     */
    void send_interest_message(TEDS::Type base_type_id, TEDS::Period interval_ms) {

        std::vector<char> teds_payload = TEDS::create_request_payload(base_type_id, interval_ms);

        Segment segment(Segment::MsgType::TEDS, teds_payload);

        std::vector<char> serialized_segment = segment.get_bytes();

        _channel->send(
            _address,
            Address::broadcast(Protocol::TYPE_BASED_ROUTING_PORT),
            serialized_segment.data(),
            serialized_segment.size()
        );
    }

    Channel * _channel;
    Address _address;
    Semaphore _semaphore;
    std::list<Buffer*> _data;
    std::mutex _mutex;
    std::set<TEDS::Type> _subscribed_types;
};


#endif  // COMMUNICATOR_HPP