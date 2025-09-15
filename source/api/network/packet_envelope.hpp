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

        // Copy from a buffer into a envelope-packet
        static Packet from_buffer(const void* src, size_t size) {
            
            Packet p;

            // copies the header-bytes from the buffer into the packet's header
            std::memcpy(&p.header, src, sizeof(Header));

            // Determines packet's payload capacity
            size_t packet_payload_size = p.header.payload_len;

            // calculates the actual quantity of bytes are available in the buffer after the header
            // if the buffer is the size of the header (or smaller), then it defaults to 0
            size_t actual_payload_size = (size > sizeof(Header)) ? (size - sizeof(Header)) : 0;
            
            // determines the actual number of bytes which will be copied. 
            // If the number of bytes in the buffer is bigger than the packet's payload capacity, truncate the bytes
            // If the number is <= header size, then there are 0 bytes to copy
            size_t bytes_to_copy = packet_payload_size <= actual_payload_size ? packet_payload_size : actual_payload_size;
            
            p.payload.resize(bytes_to_copy);
            
            if (bytes_to_copy) {
                std::memcpy(p.payload.data(), static_cast<const uint8_t*>(src) + sizeof(Header), bytes_to_copy);
            }

            return p;
        }
    };
};

#endif