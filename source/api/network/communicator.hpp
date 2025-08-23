#ifndef COMMUNICATOR_HPP
#define COMMUNICATOR_HPP

#include "message.hpp"

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
    typedef typename Channel::Address Address;

public:
    Communicator(Channel * channel, Address address);
    ~Communicator();

    bool send(const Message * message);
    bool receive(Message * message);

private:
    void update(typename Channel::Observed * obs,
                typename Channel::Observer::Observing_Condition c,
                Buffer * buf)
    {
        Observer::update(c, buf); // releases the thread waiting for data
    }

private:
    Channel * _channel;
    Address _address;
};

#include "communicator.tpp"

#endif  // COMMUNICATOR_HPP