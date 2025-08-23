#include "communicator.hpp"
#include "message.hpp"

/**
 * @brief Creates the Communicator attaching the specified Channel/Protocol Handler
 * @param channel Protocol Handler / Channel
 * @param address Communication End-Point Address
 */
template <typename Channel>
Communicator<Channel>::Communicator(Channel * channel, typename Channel::Address address)
    : _channel(channel), _address(address)
{
    _channel->attach(this, address);
}

/**
 * @brief Destroys the Communicator and detaches 
 */
template <typename Channel>
Communicator<Channel>::~Communicator()
{
    _channel->detach(this, _address);
}

/** 
 * @brief 
*/
template <typename Channel>
bool Communicator<Channel>::send(const Message * message)
{
    return (_channel->send(_address, Channel::Address::BROADCAST, message->data(),
                            message->size()) > 0);
}

/** 
 * @brief 
*/
template <typename Channel>
bool Communicator<Channel>::receive(Message * message)
{
    Buffer * buf = Observer::updated(); // block until a notification is triggered
    Channel::Address from;
    int size = _channel->receive(buf, &from, message->data(), message->size());
    // . . .
    if (size > 0)
        return true;

    return false;
}