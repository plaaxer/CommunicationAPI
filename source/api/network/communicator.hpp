#ifndef COMMUNICATOR_HPP
#define COMMUNICATOR_HPP

#include "api/observer/concurrent_observer.h"
#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"
// #include "utils/profiler.cpp"

/**
 * @brief End-Point for the components. It creates a communication channel with the Protocol Handler,
 * an instance of Protocol typenamed Channel here.
 */

template <typename Channel>
class Communicator : public Concurrent_Observer<typename Channel::Observer::Observed_Data,
                                               typename Channel::Observer::Observing_Condition>
{
    typedef Concurrent_Observer<typename Channel::Observer::Observed_Data,
                               typename Channel::Observer::Observing_Condition> Observer;
public:
    typedef typename Channel::Buffer Buffer;

    typedef typename uint32_t Type_ID;

public:
    Communicator(Channel * channel, Address address)
        : _channel(channel), _address(address)
    {

        _channel->attach(this, address.port());
    }

    ~Communicator()
    {
        _channel->detach(this, _address.port());
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
     * @brief Blocking receive method. Waits for a message to arrive and fills the provided Message object.
     */
    bool receive(Message * message)
    {
        Buffer * buf = Observer::updated(); // block until a notification is triggered
        Address from;
        int size = _channel->receive(buf, &from, message->data(), message->size());
        message->set_source(from);
        
        _channel->free(buf);
        if (size > 0)
            return true;

        return false;
    }

    /**
     * @brief Subscribes to a specific data type by attaching to the type-based routing port.
     * @param obs Pointer to the observer that will handle incoming data of the specified type.
     * @param type_id The unique identifier for the data type to subscribe to.
     */
    void subscribe_to_type(Observer * obs, Type_ID type_id)
    {
        _channel->attach(obs, TYPE_BASED_ROUTING_PORT);
        
        // todo: store this in some list to manage later
    }

    /**
     * @brief Facility to fetch configured address in the application end-point (Component class)
     * TODO: Verify if is it allowed to do
     */
    const Address& address() const {
        return _address;
    }

    //inline static Profiler* _prof = nullptr;

private:
    Channel * _channel;
    Address _address;
};


#endif  // COMMUNICATOR_HPP