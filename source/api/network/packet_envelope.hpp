#ifndef PACKET_ENVELOPE_HPP
#define PACKET_ENVELOPE_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>

class PacketEnvelope {

public:
    enum MessageType : uint8_t {
        LATENCY = 1,
        SMART_DATA = 2,
        UNDEFINED = 3
    };

    // Fixed-size header for wire format
    struct Header { 
        uint8_t  msg_type;     // MessageType
        uint32_t payload_len;  // number of bytes following

        Header(MessageType type = MessageType::UNDEFINED, uint32_t len = 0)
            : msg_type(static_cast<uint8_t>(type)), payload_len(len) {}

        MessageType get_msg_type() const { return static_cast<MessageType>(msg_type); }
        void set_type(MessageType type) { msg_type = static_cast<uint8_t>(type); }
    } __attribute__((packed));

    struct Packet
    {
        Header header;

        // contiguous payload bytes (SmartData::Packet or LatencyTest::Packet serialized)
        std::vector<uint8_t> payload;

        void* get_data() { return payload.data(); }
        const void* get_data() const { return payload.data(); }

        // Total size on the wire
        size_t total_size() const { return sizeof(Header) + payload.size(); }

        // Serialize into a contiguous buffer of bytes
        void serialize(void* dst) const {
            std::memcpy(dst, &header, sizeof(Header));
            std::memcpy(static_cast<uint8_t*>(dst) + sizeof(Header), payload.data(), payload.size());
        }

        // Parse from a buffer
        static Packet from_buffer(const void* src, size_t size) {
            
            // creates instance of packet
            Packet p;
            
            // copies the Header's size in bytes from the buffer to the packet
            std::memcpy(&p.header, src, sizeof(Header));

            // determines the size of the payload by extracting the size of Header from the buffer size
            size_t payload_bytes_size = size - sizeof(Header);
            
            // resizes the payload to fit precisely the 
            p.payload.resize(payload_bytes_size);


            std::memcpy(p.payload.data(), static_cast<const uint8_t*>(src) + sizeof(Header), to_copy);
            
            return p;
        }
    };
};

#endif