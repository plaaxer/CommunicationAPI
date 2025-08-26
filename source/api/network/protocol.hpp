#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "api/network/address.hpp"
#include "api/network/traits.hpp"
#include "api/network/nic.hpp"
#include "api/network/buffer.hpp"
#include "api/observer/conditionally_data_observed.h"
#include "api/observer/conditional_data_observer.hpp"

/**
 * @brief Protocol Handler. Observes the NIC for incoming Ethernet frames.
 * Also responsible to repass the frames to subscribed Communicators.
 * 
 * Meyers' Singleton pattern. Use Protocol<NIC>::init(&nic);
 */
template <typename NIC>
class Protocol : private NIC::Observer
{
public:

    static const typename NIC::Protocol_Number PROTO =
        Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER;


    typedef typename NIC::Buffer Buffer;
    typedef typename NIC::Address Physical_Address;
    
    typedef Address::Port Port;

    typedef Conditional_Data_Observer<Buffer<Ethernet::Frame>, Port> Observer;
    typedef Conditionally_Data_Observed<Buffer<Ethernet::Frame>, Port> Observed;


    // HEADER ------------------------
    class Header
    {
    public:
        Header(Port sport = 0, Port dport = 0) : _sport(sport), _dport(dport) {}

        // source port and destination port
        Port sport() const { return _sport; }
        Port dport() const { return _dport; }

    private:
        Port _sport;
        Port _dport;
    } __attribute__((packed));


    // max packet size. Nic MTU size minus header size
    static const unsigned int MTU = NIC::MTU - sizeof(Header);
    typedef unsigned char Data[MTU]; // represents a byte array

    // PACKET -------------------------
    class Packet : public Header
    {
    public:
        Packet();
        Header * header();
        template<typename T>
        T * data() { return reinterpret_cast<T *>(&_data); }
    private:
        Data _data;
    } __attribute__((packed)); // removing padding


private:
    // Meyers' Singleton pattern in Protocol

    NIC* _nic;

    Protocol() : _nic(nullptr) {}

    ~Protocol() {
        if (_nic) {
            _nic->detach(&instance(), PROTO); 
        }
    }

    // static method to get the single instance
    static Protocol& instance() {
        static Protocol inst;
        return inst;
    }

    static Observed _observed; 

public:
    
    // protocol initialization with the NIC to be used
    static void init(NIC* nic) {
        if (instance()._nic == nullptr) {
            instance()._nic = nic;
            instance()._nic->attach(&instance(), PROTO);
        }
    }

    Protocol(Protocol const&) = delete;
    void operator=(Protocol const&) = delete;

    static int send(Address from, Address to, const void * data, unsigned int size)
    {
        
        // todo: use buffers here?

        Protocol& protocol = instance();

        if (size > MTU) {
            std::cerr << "Data size exceeds MTU." << std::endl;
            return -1;
        }

        int to_port = to.port();
        if (to_port == 0) {
            std::cerr << "Destination port is zero." << std::endl;
            return -1;~
        }
        
        int from_port = from.port();
        if (from_port == 0) {
            std::cerr << "Source port is zero." << std::endl;
        }

        Header header(from_port, to_port);

        Physical_Address destination_paddr = to.paddr();

        if (!destination_paddr) {
            std::cerr << "Invalid destination physical address." << std::endl;
            return -1;
        }

        if (!source_paddr) {
            std::cerr << "Invalid source physical address." << std::endl;
            return -1;
        }

        Packet packet;
        *packet.header() = header;
        std::memcpy(packet.data<unsigned char>(), data, size);

        int bytes_sent = protocol._nic->send(destination_paddr, PROTO, &packet, size + sizeof(Header));
        
        if (bytes_sent > 0) {
            return bytes_sent; // obs: this includes header size
        }

        return -1;
    }

    static int receive(Buffer<Ethernet::Frame> * buf, Address from, void * data, unsigned int size)
    {
        return -1;
    }

    static void attach(Observer * obs, Address address)
    {
        _observed.attach(obs, address.port());
    }

    static void detach(Observer * obs, Address address)
    {
        _observed.detach(obs, address.port());
    }

private:

    static 

    void update(typename NIC::Observed * obs, typename NIC::Protocol_Number prot, Buffer<Ethernet::Frame> * buf) override
    {
        
        // todo: update() receives a buffer. However currently in NIC we send a frame.
        // either we change the data type there or here.

        if (prot != PROTO) {
            std::cerr << "Protocol::update() received unexpected protocol number." << std::endl;
            _nic->free(buf);
            return;
        }

        // maybe add a if buf.is_empty() or something



        if(!_observed.notify(false, buf)) // to call receive(...);
            _nic->free(buf);
    } // static member to manage observers
};


#endif  // PROTOCOL_HPP