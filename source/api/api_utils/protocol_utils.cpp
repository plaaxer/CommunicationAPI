#include "protocol_utils.hpp"
#include "protocol.hpp"  // To get full definition of Protocol and Packet

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
                  << " destiny port=" << proto_header->dport()
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

} // namespace ProtocolUtils
