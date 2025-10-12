#ifndef COMMUNICATOR_HPP
#define COMMUNICATOR_HPP

#include "api/observer/observers.hpp"
#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/teds.hpp"
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
    }

    /**
     * @brief Send a message to the destination address specified in the Message object.
     */
    bool send(const Message * message)
    {   
        return (_channel->send(_address, message->destiny(), message->data(),
                                message->size()) > 0);
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
        
        // fills the message object with the data from the buffer
        Address from;
        int size = _channel->receive(buf, &from, message->data(), message->size());
        message->set_source(from);

        _channel->free(buf);
        return (size > 0);
    }

    /**
     * @brief Subscribes to a specific data type by attaching to the type-based routing port.
     * @param obs Pointer to the observer that will handle incoming data of the specified type.
     * @param type_id The unique identifier for the data type to subscribe to.
     */
    void subscribe_to_type(TEDS::Type type_id, TEDS::Period interval_ms = 1000)
    {
        _channel->attach_type_listener(this, type_id);
        _subscribed_types.insert(type_id);
        send_interest_message(type_id, interval_ms);
    }

    /**
     * @brief Facility to fetch configured address in the application end-point (Component class)
     * TODO: Verify if is it allowed to do
     */
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
    void send_interest_message(TEDS::Type type_id, TEDS::Period interval_ms) {
        // todo: implement interest message sending.
        // We should probably have an overlapping structure over the Packet that specifies the type of the payload.
        // There we can have a specific type for interest messages - a type for subscribing to a data type - and a type for control (port) messages.
    }

    Channel * _channel;
    Address _address;
    Semaphore _semaphore;
    std::list<Buffer*> _data;
    std::mutex _mutex;
    std::set<TEDS::Type> _subscribed_types;
};


#endif  // COMMUNICATOR_HPP