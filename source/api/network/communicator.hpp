#ifndef COMMUNICATOR_HPP
#define COMMUNICATOR_HPP

#include "api/observer/concurrent_observer.h"
#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"
#include "utils/profiler.hpp"

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
        _profiler->profile(11);
        return (_channel->send(_address, message->destiny(), message->data(),
                                message->size()) > 0);
    }

    bool receive(Message * message)
    {
        Buffer * buf = Observer::updated(); // block until a notification is triggered
        _profiler->profile(9);
        Address from;
        int size = _channel->receive(buf, &from, message->data(), message->size());
        message->set_source(from);
        
        _profiler->profile(10);
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

    Profiler * _profiler;

    void setProfiler(Profiler* p) {
        _profiler = p;
    }
private:
    Channel * _channel;
    Address _address;
};


#endif  // COMMUNICATOR_HPP