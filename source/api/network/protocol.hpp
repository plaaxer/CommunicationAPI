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
 * Thread-safe btw.
 */
template <typename NIC>
class Protocol : private NIC::Observer
{
public:

    static const typename NIC::Protocol_Number PROTO =
        Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER;


    typedef typename NIC::FrameBuffer Buffer;
    typedef typename NIC::Address Physical_Address;
    
    typedef Address::Port Port;

    typedef Conditional_Data_Observer<Buffer, Port> Observer;
    typedef Conditionally_Data_Observed<Buffer, Port> Observed;


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
        Header * header() { return this; }
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


    inline static Observed _observed; 

public:
    
    // static method to get the single instance
    static Protocol& instance() {
        static Protocol inst;
        return inst;
    }


    // protocol initialization with the NIC to be used
    static void init(NIC* nic) {
        if (instance()._nic == nullptr) {
            instance()._nic = nic;
            instance()._nic->attach(&instance(), PROTO);
        }
    }

    Protocol(Protocol const&) = delete;
    void operator=(Protocol const&) = delete;

    /**
     * @brief Fills the payload data (including ports header) and sends it via NIC
     * after allocating a buffer.
     */

    static int send(Address from, Address to, const void * data, unsigned int size)
    {
        
        Protocol& protocol = instance();

        if (protocol._nic == nullptr) {
            std::cerr << "Protocol not initialized!" << std::endl;
            return -1;
        }

        if (size > MTU) {
            std::cerr << "Data size exceeds MTU." << std::endl;
            return -1;
        }

        // 1. allocating a buffer from the NIC
        unsigned int total_size = sizeof(Header) + size;
        Buffer* buf = protocol._nic->alloc(to.paddr(), PROTO, total_size);

        if (!buf) {
            std::cerr << "Failed to allocate buffer from NIC." << std::endl;
            return -1;
        }

        // 2. filling the ports Header (from Protocol, not Ethernet)
        // todo: we are going to alter the way buffers work, so the way that 
        // we access the frame inside the buffer may change
        Packet* packet = reinterpret_cast<Packet*>(buf->data()->data);

        *packet->header() = Header(from.port(), to.port());
        std::memcpy(packet->template data<void>(), data, size);

        // 3. send the entire buffer via NIC
        // NIC should take care of the Ethernet header (MAC and protocol)!!
        int bytes_sent = protocol._nic->send(buf);
        
        // debug_frame(buf);

        return bytes_sent;

        // old direct implementation without buffers

        // if (size > MTU) {
        //     std::cerr << "Data size exceeds MTU." << std::endl;
        //     return -1;
        // }

        // int to_port = to.port();
        // if (to_port == 0) {
        //     std::cerr << "Destination port is zero." << std::endl;
        //     return -1;~
        // }
        
        // int from_port = from.port();
        // if (from_port == 0) {
        //     std::cerr << "Source port is zero." << std::endl;
        // }

        // Header header(from_port, to_port);

        // Physical_Address destination_paddr = to.paddr();

        // if (!destination_paddr) {
        //     std::cerr << "Invalid destination physical address." << std::endl;
        //     return -1;
        // }

        // if (!source_paddr) {
        //     std::cerr << "Invalid source physical address." << std::endl;
        //     return -1;
        // }

        // Packet packet;
        // *packet.header() = header;
        // std::memcpy(packet.data<unsigned char>(), data, size);

        // int bytes_sent = protocol._nic->send(destination_paddr, PROTO, &packet, size + sizeof(Header));
        
        // if (bytes_sent > 0) {
        //     return bytes_sent; // obs: this includes header size
        // }

        // return -1;
    }

    // Reminder to check different header definitions... from protocol and ethernet

    /*
    Reminder of Ethernet::Header: struct Header {
        MAC dhost; // Destination MAC address (6 bytes)
        MAC shost; // Source MAC address (6 bytes)
        Protocol type; // EtherType/Protocol (2 bytes)
    };
    Reminder of Protocol::Header: class Header
    {
    public:
        Header(Port sport = 0, Port dport = 0) : _sport(sport), _dport(dport) {}
    */

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
    Header* proto_header = packet->header();

    *from = Address(frame->header.shost, proto_header->sport());

    // careful with Header != Ethernet::Header. Frame data includes Header
    unsigned int data_size_in_packet = frame->data_length - sizeof(Header); 
    unsigned int bytes_to_copy = std::min(size, data_size_in_packet);

    std::memcpy(data, packet->template data<void>(), bytes_to_copy);

    // Communicator should free the buffer!!!!!!

    // debug_frame(buf);
    
    // copied bytes
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
        if (instance()._nic) {
            instance()._nic->free(buf);
        }
        else {
            // if this happens we're cooked
            std::cerr << "Protocol::free(buffer) error: NIC not initialized." << std::endl;
        }
    }

private:

    // important: this method is NOT static. NIC calls it directly.
    /**
     * @brief Update method called by NIC when a frame with matching protocol is received.
     */

void update(typename NIC::Observed * obs, typename NIC::Protocol_Number prot, Buffer * buf) override
{   
    //should not happen?
    if (prot != PROTO) {
        std::cout << "Protocol mismatch: expected " << PROTO << ", got " << prot << std::endl;
        _nic->free(buf);
        return;
    }

    Ethernet::Frame* frame = buf->data();

    // frame data includes Protocol::Header at the start
    Packet* packet = reinterpret_cast<Packet*>(frame->data);

    // selecting who is the listener based on destination port
    Port dest_port = packet->header()->dport();

    Address dest_addr(frame->header.dhost, dest_port);

    bool was_notified = _observed.notify(dest_port, buf);

    log_receiving(buf);

    // freeing if no listeners
    if (!was_notified) {
        _nic->free(buf);
    }
}

    static void log_receiving(Buffer* buf)
    {
        if (!buf) return;

        Ethernet::Frame* frame = buf->data();
        if (!frame) return;

        Packet* packet = reinterpret_cast<Packet*>(frame->data);
        Header* proto_header = packet->header();

        Ethernet::Header header = frame->header;
        Ethernet::MAC shost = header.shost;
        Ethernet::MAC dhost = header.dhost;

        std::cout << "--------- Protocol Frame Received ---------" << std::endl;
        std::cout << " data_len=" << frame->data_length
                  << " source port=" << proto_header->sport()
                  << " destiny port=" << proto_header->dport() << std::endl;
    }

    /**
     * @brief Debug utility to print frame info (MACs, ports, etc)
     */
    static void debug_frame(Buffer* buf)
    {
        if (!buf) return;

        Ethernet::Frame* frame = buf->data();
        if (!frame) return;

        Packet* packet = reinterpret_cast<Packet*>(frame->data);
        Header* proto_header = packet->header();

        std::cout << "[Protocol] Receveing frame: data_len=" << frame->data_length
                  << " sport=" << proto_header->sport()
                  << " dport=" << proto_header->dport() << std::endl;

        // Print MAC addresses as hex (assumes MAC is a POD of contiguous bytes)
        const unsigned char* sh = reinterpret_cast<const unsigned char*>(&frame->header.shost);
        const unsigned char* dh = reinterpret_cast<const unsigned char*>(&frame->header.dhost);
        size_t maclen = sizeof(frame->header.shost);

        std::cout << "[Protocol] SMAC=";
        for (size_t i = 0; i < maclen; ++i) {
            std::printf("%02x", sh[i]);
            if (i + 1 < maclen) std::printf(":");
        }
        std::cout << " DMAC=";
        for (size_t i = 0; i < maclen; ++i) {
            std::printf("%02x", dh[i]);
            if (i + 1 < maclen) std::printf(":");
        }
        std::cout << std::endl;
    }
};

#endif  // PROTOCOL_HPP