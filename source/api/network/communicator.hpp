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
        return (_channel->send(_address, message->destiny(), message->data(),
                                message->size()) > 0);
    }

    bool receive(Message * message)
    {
        Buffer * buf = Observer::updated(); // block until a notification is triggered
        printf("\nCOMMUNICATOR RECEBEU!!\n");
        Address from;
        int size = _channel->receive(buf, &from, message->data(), message->size());
        message->set_source(from);
        
        _channel->free(buf);
        if (size > 0)
            return true;

        return false;
    }

    // obs: update() in communicator already calls the concurrent update, releasing the waiting thread.

    /**
     * @brief Facility to fetch configured address in the application end-point (Component class)
     * TODO: Verify if is it allowed to do
     */
    const Address& address() const {
        return _address;
    }

private:
    Channel * _channel;
    Address _address;
};


#endif  // COMMUNICATOR_HPP