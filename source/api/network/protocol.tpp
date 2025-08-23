#include "protocol.hpp"

template <typename NIC>
int Protocol<NIC>::send(Address from, Address to, const void * data, unsigned int size)
{
    // Buffer * buf = NIC::alloc(to.paddr, PROTO, sizeof(Header) + size)
    // NIC::send(buf)

    // To do
}

template <typename NIC>
int Protocol<NIC>::receive(Buffer * buf, Address from, void * data, unsigned int size)
{
    // unsigned int s = NIC::receive(buf, &from.paddr, &to.paddr, data, size)
    // NIC::free(buf)
    // return s;

    // To do
}


/**
 * @brief Attach a Communicator to the desired Protocol Handler
 */
template <typename NIC>
void Protocol<NIC>::attach(Observer * obs, Address address)
{
    // To do
}

/**
 * @brief Detach a Communicator
 */
template <typename NIC>
void Protocol<NIC>::detach(Observer * obs, Address address)
{
    // To do
}