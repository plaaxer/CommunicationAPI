#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "api/network/definitions/address.hpp"
#include "api/network/definitions/traits.hpp"
#include "api/network/nic.hpp"
#include "api/network/definitions/buffer.hpp"
#include "api/observer/conditionally_data_observed.h"
#include "api/observer/conditional_data_observer.hpp"


template<typename Engine> class NIC;

/**
 * @brief Protocol Handler. A template over a mandatory LocalNIC and an
 * optional ExternalNIC. The ExternalNIC is only used by the Gateway.
 * 
 * Meyers' Singleton pattern. Use Protocol<NIC>::init(&nic);
 * Thread-safe btw.
 */
template <typename LocalNIC, typename ExternalNIC = void>
class Protocol : private LocalNIC::Observer
{
public:

    static const typename LocalNIC::Protocol_Number PROTO =
        Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER;


    typedef typename LocalNIC::FrameBuffer Buffer;
    typedef typename LocalNIC::Address Physical_Address;
    
    typedef Address::Port Port;

    typedef Conditional_Data_Observer<Buffer, Port> Observer;
    typedef Conditionally_Data_Observed<Buffer, Port> Observed;


    // Protocol Header (named as PortHeader, it only contains ports) ------------------------
    class PortHeader
    {
    public:
        PortHeader(Port sport = 0, Port dport = 0) : _sport(sport), _dport(dport) {}

        // source port and destination port
        Port sport() const { return _sport; }
        Port dport() const { return _dport; }

    private:
        Port _sport;
        Port _dport;
    } __attribute__((packed));


    // max packet size. Nic MTU size minus header size
    static const unsigned int MTU = LocalNIC::MTU - sizeof(PortHeader);
    typedef unsigned char Data[MTU]; // represents a byte array

    // PACKET -------------------------
    class Packet : public PortHeader
    {
    public:
        Packet();
        PortHeader * portheader() { return this; }
        template<typename T>
        T * data() { return reinterpret_cast<T *>(&_data); }
    private:
        Data _data;
    } __attribute__((packed)); // removing padding


private:
    // Meyers' Singleton pattern in Protocol

    Protocol() : _local_nic(nullptr), _external_nic(nullptr) {}
    
    ~Protocol() {
        if (_local_nic) {
            _local_nic->detach(&instance(), PROTO); 
        }
        if constexpr (!std::is_void_v<ExternalNIC>) {
            if (_external_nic) {
                _external_nic->detach(&instance(), PROTO);
            }
        }
    }
    
    inline static Observed _observed;
    LocalNIC* _local_nic;
    ExternalNIC* _external_nic;
    
public:
    
    // static method to get the single instance
    static Protocol& instance() {
        static Protocol inst;
        return inst;
    }


    // protocol initialization for the RCU
    static void init_gateway(LocalNIC* local_nic, ExternalNIC* external_nic) {
        auto& p = instance();
        if (p._local_nic == nullptr) {
            p._local_nic = local_nic;
            p._external_nic = external_nic;
            p._local_nic->attach(&p, Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER);
            p._external_nic->attach(&p, Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER);
        }
    }

    // initialization for a simple component (provides only local NIC)
    static void init_component(LocalNIC* local_nic) {
        auto& p = instance();
        if (p._local_nic == nullptr) {
            p._local_nic = local_nic;
            p._external_nic = nullptr;
            p._local_nic->attach(&p, Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER);
        }
    }

    Protocol(Protocol const&) = delete;
    void operator=(Protocol const&) = delete;

    /**
     * @brief Fills the payload data (including ports header) and sends it via NIC
     * after allocating a buffer.
     */

    static int send(Address from, Address to, const void * data, unsigned int size);

    /**
     * @brief Receives data from a buffer, extracts the source address and copies
     * the payload into the provided data pointer.
     */
    static int receive(Buffer * buf, Address* from, void* data, unsigned int size)
    {
        if (!buf || !from || !data) {
            return -1;
        }

        // payload structures
        Ethernet::Frame* frame = buf->data();
        Packet* packet = reinterpret_cast<Packet*>(frame->data);
        PortHeader* proto_header = packet->portheader();

        *from = Address(frame->header.shost, proto_header->sport());

        // Frame data includes PortHeader
        unsigned int data_size_in_packet = frame->data_length - sizeof(PortHeader); 
        unsigned int bytes_to_copy = std::min(size, data_size_in_packet);

        /*
        We just return the actual payload without the PortHeader as the addressing is saved
        on the Address* from parameter.
        */ 

        std::memcpy(data, packet->template data<void>(), bytes_to_copy);
        
        return bytes_to_copy;
    }

    static void attach(Observer * obs, Address::Port port)
    {
        _observed.attach(obs, port);
    }

    static void detach(Observer * obs, Address::Port port)
    {
        _observed.detach(obs, port);
    }

    static void free(Buffer * buf)
    {
        if (instance()._local_nic) {
            instance()._local_nic->free(buf);
        }
        else {
            // if this happens we're cooked
            std::cerr << "Protocol::free(buffer) error: no NIC initialized." << std::endl;
            // we can still delete it tho lol
            delete buf;
        }
    }

    /*
    We have two update methods. Each one is called by the corresponding NIC; i.e a packet arriving intra or inter vehicles.
    */

    // LocalNIC update.
    void update(typename LocalNIC::Observed* obs, typename LocalNIC::Protocol_Number prot, typename LocalNIC::FrameBuffer* buf);

    // ExternalNIC update.
    template <typename T = ExternalNIC>
    std::enable_if_t<!std::is_void_v<T>> // std::enable_if_t ensures this method only exists when ExternalNIC is not void.
    update(typename T::Observed* obs, typename T::Protocol_Number prot, typename T::FrameBuffer* buf) {

        // should not happen
        if (prot != PROTO) {
            std::cout << "Protocol mismatch: expected " << PROTO << ", got " << prot << std::endl;
            _external_nic->free(buf);
            return;
        }
        
        Ethernet::Frame* frame = buf->data();
        
        // small check to prevent hidden bugs
        Ethernet::Header* header = frame->header;
        if (header->dhost != Ethernet::MAC(Ethernet::BROADCAST_ADDR)) {
            std::cout << "Warning of invalid state: Raw Socket Nic has received a message that is not broadcast" << std::endl;
        }

        Packet* packet = reinterpret_cast<Packet*>(frame->data);

        Port dest_port = packet->portheader()->dport();

        std::cout << "Gateway: Routing external packet to local port " << dest_port << std::endl;
        
        // re-sending the packet locally. We replace the broadcast address with a local one.
        Address local_dest(Ethernet::MAC(Ethernet::LOCAL_ADDR), dest_port);
        Address from(header->shost, packet->portheader()->sport());

        Protocol::send(from, local_dest, packet->template data<void>(), buf->data()->data_length - sizeof(PortHeader));

        _external_nic->free(buf);
    }

private:

    /**
     * @brief A generic frame sender that operates on any NIC type.
     * @details This private helper encapsulates the common logic for allocating,
     * building, and sending a packet to avoid code duplication. It can send stuff locally
     * or remotely, and should NOT be called directly!
     */
    template <typename NIC_Type>
    int _send_frame(NIC_Type* nic, const Address& from, const Address& to, const void* data, unsigned int size) {

        if (!nic) {
            return -1;
        }

        unsigned int total_size = sizeof(PortHeader) + size;

        Buffer* buf = nic->alloc(to.paddr(), Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER, total_size);
        if (!buf) return -1;

        Packet* packet = reinterpret_cast<Packet*>(buf->data()->data);
        *packet->portheader() = PortHeader(from.port(), to.port());
        std::memcpy(packet->template data<void>(), data, size);

        return nic->send(buf);
    }

    /**
     * @brief Sends a frame via the local (shared memory) NIC.
     */
    int send_local_frame(const Address& from, const Address& to, const void* data, unsigned int size) {
        auto& p = instance();
        return _send_frame(p._local_nic, from, to, data, size);
    }

    /**
     * @brief Sends a frame via the external (physical) NIC.
     * @details This should only be called when an ExternalNIC is available.
     */
    int send_remote_frame(const Address& from, const Address& to, const void* data, unsigned int size) {
        auto& p = instance();
        return _send_frame(p._external_nic, from, to, data, size);
    }
};

/**
* @brief Fills the payload data (including ports header) and sends it via the correct NIC
* after allocating a buffer.
*
* @details This could be called in two instances: 
* (1) Gateway (RCU) redirecting traffic.
* (2) Regular component sending out stuff.
* If there's no external NIC, then we are a component. We must then send it to the gateway so that it can redirect said message to the network.
* A very easy way to do that, and why this architecture is so concise, is to just write on the shared memory, aka do a regular send, but locally.
* Components shall ignore this message, as its destination is external. The gateway/rcu shall reroute it. We can keep the destination ports.
* The repeated send_local_frame() are there for clarity. Top one will be redirected by the gateway. Bottom one shall be sent directly locally.
*/

template <typename LocalNIC, typename ExternalNIC>
int Protocol<LocalNIC, ExternalNIC>::send(Address from, Address to, const void* data, unsigned int size) {

    auto& p = instance();

    bool is_external = (to.paddr() == Ethernet::MAC(Ethernet::BROADCAST_ADDR));

    // std::cout << "-----protocol::send()-----" << std::endl;
    // std::cout << "From: " << from << std::endl;
    // std::cout << "To: " << to << std::endl;
    // std::cout << "-----end send-----" << std::endl;

    if (is_external) {

        if constexpr (!std::is_void_v<ExternalNIC>) {

            if (p._external_nic) {

                p.send_remote_frame(from, to, data, size);
            }
        }
        
        return p.send_local_frame(from, to, data, size);
        // check @details
        
    } else {

        return p.send_local_frame(from, to, data, size);

    }
    return -1;
}

/**
 * @brief Update method called by the LocalNIC, aka NIC<ShmEngine>. Means that a package/frame has arrived.
 * 
 * @details 
 * A package arriving locally can be both external or internal. That is checked by is_external_dest. 
 * If the package came from a local component and we are the RCU, then we reroute it outside.
 * External packages - that came from another vm - won't ever update the RCU through the LocalNIC, so
 * there's no need to check out for those. That means that external packages - in this specific method
 * at least - ALWAYS originate from the local vehicle/vm.
 */

template <typename LocalNIC, typename ExternalNIC>
void Protocol<LocalNIC, ExternalNIC>::update(typename LocalNIC::Observed* obs, typename LocalNIC::Protocol_Number prot, typename LocalNIC::FrameBuffer* buf) {

    Ethernet::Frame* frame = buf->data();
    Packet* packet = reinterpret_cast<Packet*>(frame->data);
    Port dest_port = packet->portheader()->dport();

    bool is_external_dest = (frame->header.dhost == Ethernet::MAC(Ethernet::BROADCAST_ADDR));

    if (is_external_dest) {
        
        // compiler does not complain about not using "if contesxpr[...]" here as we are not
        // calling anything from the _external_nic.

        if (frame->header.shost != _local_nic->address()) {
            // todo: change dummy address to actual MAC address or this will always pop up. IMPORTANT!
            // godly preemptive programming?
            std::cout << "Warning of invalid state: external destiny received through local nic coming from another vehicle" << std::endl;
            std::cout << "Source of frame: " << frame->header.shost << std::endl;
            std::cout << "Local NIC address: " << _local_nic->address() << std::endl;
            // (if you have inverted local and remote nics, this will pop up every time)
        }

        if (_external_nic) {

            std::cout << "Gateway: Routing local packet to outside world..." << std::endl;
            
            Address from(buf->data()->header.shost, packet->portheader()->sport());
            Address to(buf->data()->header.dhost, packet->portheader()->dport());
            const void* payload = packet->template data<void>();
            unsigned int payload_size = buf->data()->data_length - sizeof(PortHeader);
            
             // static send method that will allocate the buffer and call the correct nic.
            Protocol::send(from, to, payload, payload_size);

            _local_nic->free(buf);

        } else { // not the RCU

            _local_nic->free(buf);
        }

    } else {

        if (!_observed.notify(dest_port, buf)) {
            _local_nic->free(buf);
        }
    }
}


#endif  // PROTOCOL_HPP