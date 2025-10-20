#include "protocol_utils.hpp"
#include "api/network/protocol.hpp"

namespace ProtocolUtils {

    template <typename LocalNIC, typename ExternalNIC>
    void log_receiving(typename Protocol<LocalNIC, ExternalNIC>::Buffer* buf)
    {
        if (!buf) return;

        Ethernet::Frame* frame = buf->data();
        if (!frame) return;

        using Packet = typename Protocol<LocalNIC, ExternalNIC>::Packet;
        using PortHeader = typename Protocol<LocalNIC, ExternalNIC>::PortHeader;

        Packet* packet = reinterpret_cast<Packet*>(frame->data);
        if (!packet) return;

        PortHeader* proto_header = packet->portheader();
        if (!proto_header) return;

        Ethernet::Header header = frame->header;

        std::cout << "--------- Protocol Frame Received ---------" << std::endl;
        std::cout << " data_len=" << frame->data_length
                  << " source port=" << proto_header->sport()
                  << " destination port=" << proto_header->dport()
                  << std::endl;
    }

    template <typename LocalNIC, typename ExternalNIC>
    void debug_frame(typename Protocol<LocalNIC, ExternalNIC>::Buffer* buf)
    {
        if (!buf) return;

        Ethernet::Frame* frame = buf->data();
        if (!frame) return;

        using Packet = typename Protocol<LocalNIC, ExternalNIC>::Packet;
        using PortHeader = typename Protocol<LocalNIC, ExternalNIC>::PortHeader;

        Packet* packet = reinterpret_cast<Packet*>(frame->data);
        if (!packet) return;

        PortHeader* proto_header = packet->portheader();
        if (!proto_header) return;

        std::cout << "[Protocol] Receiving frame: data_len=" << frame->data_length
                  << " sport=" << proto_header->sport()
                  << " dport=" << proto_header->dport()
                  << std::endl;

        // Print MACs
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

    inline void debug_frame(const Ethernet::Frame& frame) {
        std::cout << "--- Ethernet Frame Debug ---" << std::endl;
        std::cout << "  Destination MAC: " << frame.header.dhost << std::endl;
        std::cout << "  Source MAC:      " << frame.header.shost << std::endl;
        std::cout << "  EtherType:       0x" << std::hex << std::setw(4) << std::setfill('0')
                << ntohs(frame.header.type) << std::dec << std::endl;
        std::cout << "  Data Length:     " << frame.data_length << " bytes" << std::endl;

        unsigned int bytes_to_print = std::min(16u, frame.data_length);
        if (bytes_to_print > 0) {
            std::cout << "  Payload Sample:  ";
            for (unsigned int i = 0; i < bytes_to_print; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') 
                        << static_cast<int>(frame.data[i]) << " ";
            }
            std::cout << std::dec << std::endl;
        }
        std::cout << "----------------------------" << std::endl;
    }

} // namespace ProtocolUtils
