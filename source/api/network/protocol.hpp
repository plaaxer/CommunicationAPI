#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "api/network/definitions/address.hpp"
#include "api/network/definitions/traits.hpp"
#include "api/network/nic.hpp"
#include "api/network/definitions/buffer.hpp"
#include "api/network/definitions/protocol_definitions.hpp"
#include "api/observer/conditionally_data_observed.h"
#include "api/observer/conditional_data_observer.hpp"
#include "api/network/definitions/teds.hpp"
#include "api/observer/observers.hpp"
#include "api/network/ptp/slave_synchronizer.hpp"
#include "api/network/ptp/ptp_timer_thread.hpp"
#include "api/network/ptp/i_synchronizer.hpp"
#include "api/network/ptp/master_synchronizer.hpp"
#include "api/network/ptp/ptp_roles.hpp"
#include "api/utils/clock.hpp"
#include "api/network/groups/group_roles.hpp"
#include "api/network/groups/i_group_handler.hpp"
#include "api/network/groups/group_member_handler.hpp"
#include "api/network/groups/group_leader_handler.hpp"
#include "api/network/definitions/quadrant.hpp"

#include <bitset>


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

    typedef SlaveSynchronizer<LocalNIC, ExternalNIC> SlaveSync;
    
    typedef Address::Port Port;

    typedef Conditionally_Data_Observed<Buffer, Port> Observed;

private:
    // Meyers' Singleton pattern in Protocol

    Protocol() : _local_nic(nullptr), _external_nic(nullptr), _synchronizer(nullptr), _ptp_timer_thread(nullptr) {}
    
    ~Protocol() {
        if (_local_nic) {
            _local_nic->detach(&instance(), PROTO); 
        }
        if constexpr (!std::is_void_v<ExternalNIC>) {
            if (_external_nic) {
                _external_nic->detach(&instance(), PROTO);
            }
        }
        if (_ptp_timer_thread) {
            _ptp_timer_thread->stop();
        }
    }
    
    inline static Conditionally_Data_Observed<Buffer, Address::Port> _port_observed;
    inline static Conditionally_Data_Observed<Buffer, TEDS::Type> _type_observed;
    LocalNIC* _local_nic;
    ExternalNIC* _external_nic;

    std::unique_ptr<ISynchronizer> _synchronizer;
    std::unique_ptr<PtpTimerThread<SlaveSync>> _ptp_timer_thread;

    std::unique_ptr<IGroupHandler<LocalNIC, ExternalNIC>> _group_handler;
    
public:
    
    // static method to get the single instance
    static Protocol& instance() {
        static Protocol inst;
        return inst;
    }

    /**
     * @brief Initializes the RCU (Gateway) of a vehicle or roadside unit.
     */
    static void init_gateway(LocalNIC* local_nic, ExternalNIC* external_nic, Quadrant quadrant, PtpRole ptp_role = PtpRole::SLAVE, GroupRole group_role = GroupRole::MEMBER) {
        auto& p = instance();
        if (p._local_nic == nullptr) {
            p._local_nic = local_nic;
            p._external_nic = external_nic;
            p._local_nic->attach(&p, Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER);
            p._external_nic->attach(&p, Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER);
            p.update_quadrant(quadrant);
        }

        init_clock_synchronization(ptp_role);

        init_group_role(group_role);

    }
    /**
     * @brief Initializes a regular component of a vehicle. It has no external communication capabilities.
     */
    static void init_component(LocalNIC* local_nic) {
        auto& p = instance();
        if (p._local_nic == nullptr) {
            p._local_nic = local_nic;
            p._external_nic = nullptr;
            p._local_nic->attach(&p, Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER);
        }
    }

    static void init_clock_synchronization(PtpRole ptp_role) {
        auto& p = instance();

        std::cout << "[Protocol] Initial system clock: " << Clock::getCurrentTimeString() << std::endl;

        // Clock::desynchronize(); no need to desynchronize at the start anymore.

        switch (ptp_role) {
            case PtpRole::SLAVE:
                std::cout << "[Protocol] Initializing as PTP SLAVE." << std::endl;

                p._synchronizer = std::make_unique<SlaveSync>(p);
                p._ptp_timer_thread = std::make_unique<PtpTimerThread<SlaveSync>>();
            
                // runtime downcast
                p._ptp_timer_thread->start(dynamic_cast<SlaveSync*>(p._synchronizer.get()), p.get_external_address());
                break;

            case PtpRole::MASTER:
                std::cout << "[Protocol] Initializing as PTP MASTER." << std::endl;

                p._synchronizer = std::make_unique<MasterSynchronizer<LocalNIC, ExternalNIC>>(p);
                break;

            default:
                std::cerr << "ERROR: Invalid PTP role passed to initialization of protocol!" << std::endl;
                break;
        }
    }

    static void init_group_role(GroupRole group_role) {
        auto& p = instance();

        switch (group_role) {
            case GroupRole::MEMBER:
                std::cout << "[Protocol] Initializing as GROUP MEMBER." << std::endl;
                p._group_handler = std::make_unique<GroupMemberHandler<LocalNIC, ExternalNIC>>(p);
                // Being a member means being a car, so the quadrant changes over time. This lets the external NIC know it belongs to a car, and can thus proceed with changing the car's quadrant.
                p._external_nic->set_fixed_location(false); 
                break;

            case GroupRole::LEADER:
                std::cout << "[Protocol] Initializing as GROUP LEADER." << std::endl;
                p._group_handler = std::make_unique<GroupLeaderHandler<LocalNIC, ExternalNIC>>(p);
                // Being a leader means being an RSU, so the quadrant never changes. This lets the external NIC know it belongs to an RSU, and thus will never change the RSU's quadrant.
                p._external_nic->set_fixed_location(false); 
                break;

            default:
                std::cerr << "ERROR: Invalid Group Role passed to initialization of protocol!" << std::endl;
                break;
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
        unsigned int data_size_in_packet = frame->data_length - PACKET_HEADER_SIZE; 
        unsigned int bytes_to_copy = std::min(size, data_size_in_packet);

        /*
        We just return the actual payload without the PortHeader as the addressing is saved
        on the Address* from parameter.
        */ 

        std::memcpy(data, packet->template data<void>(), bytes_to_copy);
        
        return bytes_to_copy;
    }

    // Port-based observers
    void attach_port_listener(PortObserver* obs, Address::Port port)
    {
        _port_observed.attach(obs, port);
    }

    void detach_port_listener(PortObserver* obs, Address::Port port)
    {
        _port_observed.detach(obs, port);
    }

    // TEDS Type-based (broadcast) observers
    void attach_type_listener(TypeObserver* obs, TEDS::Type type)
    {
        _type_observed.attach(obs, type);
    }

    void detach_type_listener(TypeObserver* obs, TEDS::Type type)
    {
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

    Address get_external_address() {
        if constexpr (!std::is_void_v<ExternalNIC>) {
            return _external_nic->address();
        }
        throw std::runtime_error("get_external_address() called in a non-gateway protocol");
    }

    void set_session_key(SessionKey key) {
        _local_nic->set_session_key(key);
    }

    SessionKey get_session_key() {
        return _local_nic->get_session_key();
    }

    void update_quadrant(Quadrant q)
    {
        using GroupMemberHandler = GroupMemberHandler<LocalNIC, ExternalNIC>;
        auto& p = instance();

        p._external_nic->set_quadrant(q);
        
        // If it is a Vehicle moving quadrant, it should notify the Handler
        if(GroupMemberHandler* member_handler = dynamic_cast<GroupMemberHandler*>(p._group_handler.get())) {
            p._group_handler->notify_location_change();
        }

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

        unsigned int total_size = PACKET_HEADER_SIZE + size;

        Buffer* buf = nic->alloc(from.paddr(), to.paddr(), Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER, total_size);
        if (!buf) return -1;

        void* packet_memory_location = buf->data()->data;

        Packet* packet = new (packet_memory_location) Packet(
            nic->location(),
            from.port(),
            to.port()
        );

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

            const void* raw_segment_data = packet->template data<void>();

            const Segment::Header* seg_header = static_cast<const Segment::Header*>(raw_segment_data);
            const void* seg_payload = static_cast<const char*>(raw_segment_data) + sizeof(Segment::Header);
            unsigned int seg_payload_size = frame->data_length - PACKET_HEADER_SIZE - sizeof(Segment::Header);

            TEDS::Type t = TEDS::extract_type(seg_header, seg_payload, seg_payload_size);

            if (t != TEDS::INVALID) {
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

    /**
     * @brief This method is responsible for filtering out system messages that are not supposed to get to the end application,
     * but rather serve as means to coordinate vehicles, such as through groups or clock synchronization.
     * @details System messages are generally sent via unicast, hence the need to filter them out here. 
     */
    bool filter_system_messages(Packet* packet, size_t packet_length, const Address& source_address, const Address& dest_address) {

        if (_external_nic->address() == dest_address.paddr() || Ethernet::MAC(Ethernet::BROADCAST_ADDR) == dest_address.paddr()) {
            
            const void* raw_segment = packet->template data<void>();
            size_t segment_size = packet_length - PACKET_HEADER_SIZE;
            
            const char* segment_payload = static_cast<const char*>(raw_segment) + sizeof(Segment::Header);
            size_t payload_size = segment_size - sizeof(Segment::Header);
    
            const Segment::Header* seg_header = reinterpret_cast<const Segment::Header*>(raw_segment);
            Segment::MsgType final_type = seg_header->type;

            switch (final_type) {

                case Segment::MsgType::PTP:
                    _synchronizer.get()->handle_ptp_message(static_cast<const void*>(segment_payload), payload_size, source_address, dest_address);
                    return true;
                
                case Segment::MsgType::GROUP:
                    _group_handler->handle_group_message(segment_payload, payload_size, source_address);
                
                default:
                    return false;
            }
        }
        
        return false;

    }


    template<typename T>
    void print_bits(const T& value, const std::string& label = "") {
        // Get the raw memory of the value
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&value);
        
        if (!label.empty()) {
            std::cout << label << " ";
        }

        // Print the bits for each byte in order
        for (size_t i = 0; i < sizeof(T); ++i) {
            std::cout << std::bitset<8>(bytes[i]) << " ";
        }
        std::cout << std::endl;
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

    if constexpr (!std::is_void_v<ExternalNIC>) {  // Gateway

        bool is_external = (to.paddr() != p._external_nic->address() && 
            to.paddr() != p._local_nic->address());

        if (is_external) {
            // std::cout << "[PROTOCOL] Remote send called" << std::endl;
            return p.send_remote_frame(from, to, data, size);
        }

        // std::cout << "[PROTOCOL] Local send called" << std::endl;
        return p.send_local_frame(from, to, data, size);
        // check @details
        
    } else {  // Any other component
        
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
                    unsigned int payload_size = buf->data()->data_length - PACKET_HEADER_SIZE;
                    // std::cout << "[GATEWAY] Routing INTERNAL packet EXTERNALLY." << std::endl;
                    // std::cout << "[Source]: " << from << std::endl
                    //           << "[destination]: " << to << std::endl;
                    Protocol::send(from, to, payload, payload_size);
                }
            }

            if (dest_port == TYPE_BASED_ROUTING_PORT) {
                notify_communicator(dest_port, buf);
                return;
            }
            _local_nic->free(buf);

        } else {

            notify_communicator(dest_port, buf);

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
            Address from(frame->header.shost, packet->portheader()->sport());
            Address to(frame->header.dhost, packet->portheader()->dport());
            
            
            try {
                bool is_system_message = filter_system_messages(packet, frame->data_length, from, to);
                
                if (is_system_message) {
                    _external_nic->free(buf);
                    return;
                }
            } catch (const std::exception& e) {
                std::cerr << "[Protocol] ERROR in system filter: " << e.what() << std::endl;
                _external_nic->free(buf);
                return;
             }
            
            // re-sending the packet locally. The message won't be external anymore.
            Address local_dest(Ethernet::MAC(Ethernet::LOCAL_ADDR), dest_port);

            Protocol::send(from, local_dest, packet->template data<void>(), buf->data()->data_length - PACKET_HEADER_SIZE);

            _external_nic->free(buf);
        }
    }
}


#endif  // PROTOCOL_HPP