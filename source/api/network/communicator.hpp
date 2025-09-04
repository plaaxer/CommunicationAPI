#ifndef COMMUNICATOR_HPP
#define COMMUNICATOR_HPP

#include "api/observer/concurrent_observer.h"
#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"

/**
 * End-Point for the components. It creates a communication channel with the Protocol Handler,
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

    bool send(const Message * message)
    {
        return (_channel->send(_address, Address::broadcast(), message->data(),
                                message->size()) > 0);
    }

    bool receive(Message * message)
    {
        Buffer * buf = Observer::updated(); // block until a notification is triggered
        Address from;
        int size = _channel->receive(buf, &from, message->data(), message->size());
        
        _channel->free(buf); // free the buffer after processing
        if (size > 0)
            return true;

        return false;
    }

private:

    // attempt to talk with protocol
    
    // void update(typename Channel::Observed * obs,
    //             typename Channel::Observer::Observing_Condition c,
    //             Buffer * buf)
    // {
    //     Observer::update(c, buf); // releases the thread waiting for data
    // }

private:
    Channel * _channel;
    Address _address;
};


#endif  // COMMUNICATOR_HPP