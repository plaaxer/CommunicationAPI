#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "api/network/definitions/address.hpp"
#include "api/network/definitions/traits.hpp"
#include "api/network/nic.hpp"
#include "api/network/definitions/buffer.hpp"
#include "api/observer/conditionally_data_observed.h"
#include "api/observer/conditional_data_observer.hpp"
#include "api/network/definitions/teds.hpp"
#include "api/observer/observers.hpp"


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

    // A special port used for type-based routing (TEDS).
    static constexpr Address::Port TYPE_BASED_ROUTING_PORT = 0;

    static const typename LocalNIC::Protocol_Number PROTO =
        Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER;


    typedef typename LocalNIC::FrameBuffer Buffer;
    typedef typename LocalNIC::Address Physical_Address;
    
    typedef Address::Port Port;

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
    
    inline static Conditionally_Data_Observed<Buffer, Address::Port> _port_observed;
    inline static Conditionally_Data_Observed<Buffer, TEDS::Type> _type_observed;
    LocalNIC* _local_nic;
    ExternalNIC* _external_nic;
    
public:

    //inline static Profiler* _prof = nullptr;
    
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
    // std::cout << "[" << from.paddr() << "] Send called. Is external: "<< is_external << std::endl;

    // std::cout << "-----protocol::send()-----" << std::endl;
    // std::cout << "From: " << from << std::endl;
    // std::cout << "To: " << to << std::endl;
    // std::cout << "-----end send-----" << std::endl;
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

    // Port-based observers
    void attach_port_listener(PortObserver* obs, Address::Port port) {
        _port_observed.attach(obs, port);
    }
    void detach_port_listener(PortObserver* obs, Address::Port port) {
        _port_observed.detach(obs, port);
    }

    // TEDS Type-based (broadcast) observers
    void attach_type_listener(TypeObserver* obs, TEDS::Type type) {
        _type_observed.attach(obs, type);
    }
    void detach_type_listener(TypeObserver* obs, TEDS::Type type) {
        _type_observed.detach(obs, type);
    }

    static void free(Buffer * buf)
    {
        if (instance()._local_nic) {
            instance()._local_nic->free(buf);
        }
        else {
            // if this happens we're cooked
            std::cerr << "Protocol::free(buffer) error: no NIC initialized." << std::endl;
            delete buf;
        }
    }

    /*
    We have two update methods. Each one is called by the corresponding NIC; i.e a packet arriving intra or inter vehicles.
    */

    void update(typename LocalNIC::Observed* obs, typename LocalNIC::Protocol_Number prot, typename LocalNIC::FrameBuffer* buf);

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

        Buffer* buf = nic->alloc(from.paddr(), to.paddr(), Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER, total_size);
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

    /**
     * @brief Notifies the appropriate communicator based on the port or TEDS type.
     * @details If the port is TYPE_BASED_ROUTING_PORT, it extracts the TEDS type and notifies
     * the type-based observers. Otherwise, it notifies the port-based observers.
     */

    void notify_communicator(Address::Port port, Buffer* buf) {

        if (port == TYPE_BASED_ROUTING_PORT) {
            Ethernet::Frame* frame = buf->data();
            Packet* packet = reinterpret_cast<Packet*>(frame->data);
            
            const void* raw_segment_data = packet->data();

            const Segment::Header* seg_header = static_cast<const Segment::Header*>(raw_segment_data);
            const void* seg_payload = static_cast<const char*>(raw_segment_data) + sizeof(Segment::Header);
            unsigned int seg_payload_size = frame->data_length - sizeof(PortHeader) - sizeof(Segment::Header);

            TEDS::Type t = TEDS::extract_type(seg_header, seg_payload, seg_payload_size);

            if (t != TEDS::INVALID && TEDS::is_digital(t)) {
                if (!_type_observed.notify(t, buf)) {
                    _local_nic->free(buf);
                }
            } else {
                std::cerr << "WARNING: Received a type-based message with an invalid TEDS type!" << std::endl;
                _local_nic->free(buf);
            }
            return;
        }

        // Fallback for port-based routing
        if (!_port_observed.notify(port, buf)) {
            _local_nic->free(buf);
        }
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

    if (is_external) {

        if constexpr (!std::is_void_v<ExternalNIC>) {

            if (p._external_nic) {
                // std::cout << "[PROTOCOL] Remote send called" << std::endl;
                return p.send_remote_frame(from, to, data, size);

            }
        }
        
        // std::cout << "[PROTOCOL] Local send called" << std::endl;
        return p.send_local_frame(from, to, data, size);
        // check @details
        
    } else {
        // std::cout << "[PROTOCOL] Local send called" << std::endl;
        return p.send_local_frame(from, to, data, size);

    }
    return -1;
}

/**
 * @brief Update method called by the LocalNIC or ExternalNIC, NIC<ShmEngine> and NIC<RawSocketEngine> respectively. Means that a package/frame has arrived.
 * 
 * @details 
 * A package arriving locally can be both external or internal. That is checked by is_external_dest. 
 * If the package came from a local component, is external and we are the gateway, then we reroute it outside. If the
 * package is external and we are the gateway (RCU) then we redirect it to a local port. Us being the gateway is
 * defined by the absence or presence of ExternalNIC. Some other cases are:
 * - If we are a regular component and the message is external, we ignore it. If it is internal, it goes up the stack.
 * - If we are the gateway and the message is internal, we ignore it.
 * The message being *internal* or *external* defines where it is *supposed* to go, not its origin itself.
 * The gateway sometimes overwrites the MAC destination and source addresses to ensure proper reforwarding.
 */

    template <typename LocalNIC, typename ExternalNIC>
    void Protocol<LocalNIC, ExternalNIC>::update(typename LocalNIC::Observed* obs, typename LocalNIC::Protocol_Number prot, typename LocalNIC::FrameBuffer* buf)
    {
        // update from shared memory engine.
        if (obs == _local_nic)
        {

            //std::cout << "[PID " << getpid() << "] Protocol::update called from LocalNIC." << std::endl;

            Ethernet::Frame* frame = buf->data();
            Packet* packet = reinterpret_cast<Packet*>(frame->data);
            Port dest_port = packet->portheader()->dport();
            bool is_external_dest = (frame->header.dhost == Ethernet::MAC(Ethernet::BROADCAST_ADDR));

            if (is_external_dest) {
                
                // we should reforward the message outside.
                if constexpr (!std::is_void_v<ExternalNIC>) {

                    if (_external_nic) {

                        Address from(_external_nic->address(), packet->portheader()->sport());
                        Address to(buf->data()->header.dhost, packet->portheader()->dport());
                        const void* payload = packet->template data<void>();
                        unsigned int payload_size = buf->data()->data_length - sizeof(PortHeader);
                        // std::cout << "[GATEWAY] Routing INTERNAL packet EXTERNALLY." << std::endl;
                        // std::cout << "[Source]: " << from << std::endl
                        //           << "[destination]: " << to << std::endl;
                        Protocol::send(from, to, payload, payload_size);
                    }
                }
                _local_nic->free(buf);

            } else {

                if (!_port_observed.notify(dest_port, buf)) {
                    _local_nic->free(buf);
                }
            }
        }
        
        else if constexpr (!std::is_void_v<ExternalNIC>)
        {   
            // update from raw socket engine.
            if (obs == _external_nic)
            {
                Ethernet::Frame* frame = buf->data();
                Packet* packet = reinterpret_cast<Packet*>(frame->data);
                Port dest_port = packet->portheader()->dport();

                //std::cout << "[" << frame->header.shost << "]" << "[GATEWAY] Routing EXTERNAL packet INTERNALLY." << std::endl;
                
                // re-sending the packet locally. The message won't be external anymore.
                Address local_dest(Ethernet::MAC(Ethernet::LOCAL_ADDR), dest_port);
                Address from(frame->header.shost, packet->portheader()->sport());

                Protocol::send(from, local_dest, packet->template data<void>(), buf->data()->data_length - sizeof(PortHeader));

                _external_nic->free(buf);
            }
        }
    }


#endif  // PROTOCOL_HPP