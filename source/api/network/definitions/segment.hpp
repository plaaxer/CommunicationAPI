#ifndef SEGMENT_HPP
#define SEGMENT_HPP

#include <vector>
#include <cstdint>
#include <cstring>   // For std::memcpy
#include <stdexcept> // For std::runtime_error

/**
 * @class Segment
 * @brief Represents the payload of a Packet, containing a header to identify the message
 * type and a variable-length payload.
 */
class Segment {
public:
    /**
     * @enum MsgType
     * @brief Defines the nature of the data within the segment's payload.
     * This allows the receiving end to correctly parse the payload data.
     */
    enum class MsgType : uint8_t {
        CONTROL,
        TEDS
        // Easily extensible for other types like LATENCY_PING, HEARTBEAT, etc.
    };

    // The internal header for the segment, which is part of the serialized data.
    struct Header {
        MsgType type;
    } __attribute__((packed));

    Header _header;
    std::vector<char> _payload;

    /**
     * @brief Default constructor for an empty Segment.
     */
    Segment() : _header{MsgType::CONTROL}, _payload() {}

    /**
     * @brief Constructs a Segment with a specific type and payload.
     * @param type The message type of the segment.
     * @param payload A vector of chars representing the payload data.
     */
    Segment(MsgType type, const std::vector<char>& payload)
        : _header{type}, _payload(payload) {}

    /**
     * @brief Serializes the entire segment (header + payload) into a byte vector for transmission.
     * @return A std::vector<char> containing the raw bytes of the segment.
     */
    std::vector<char> get_bytes() const {
        // Create a vector with enough space for the header and the payload.
        std::vector<char> bytes(sizeof(Header) + _payload.size());

        // Copy the header into the beginning of the vector.
        std::memcpy(bytes.data(), &_header, sizeof(Header));

        // Copy the payload right after the header.
        if (!_payload.empty()) {
            std::memcpy(bytes.data() + sizeof(Header), _payload.data(), _payload.size());
        }

        return bytes;
    }

    /**
     * @brief Deserializes a byte vector back into a Segment object. (Static factory method)
     * @param bytes The raw byte vector received from the network.
     * @return A new Segment object. Throws std::runtime_error on failure.
     */
    static Segment from_bytes(const std::vector<char>& bytes) {
        // Safety check: The byte vector must be at least as large as our header.
        if (bytes.size() < sizeof(Header)) {
            throw std::runtime_error("Invalid segment data: too small to contain a header.");
        }

        Segment seg;
        
        // Copy the header portion from the byte vector.
        std::memcpy(&seg._header, bytes.data(), sizeof(Header));

        // The rest of the bytes constitute the payload.
        size_t payload_size = bytes.size() - sizeof(Header);
        if (payload_size > 0) {
            // Assign the payload by copying from the correct offset.
            const char* payload_start = bytes.data() + sizeof(Header);
            seg._payload.assign(payload_start, payload_start + payload_size);
        }

        return seg;
    }

    /**
     * @brief Gets the type of the message contained in the segment.
     * @return The MsgType enum.
     */
    MsgType get_type() const {
        return _header.type;
    }

    /**
     * @brief Gets the payload of the segment.
     * @return A const reference to the payload byte vector.
     */
    const std::vector<char>& get_payload() const {
        return _payload;
    }
};

#endif // SEGMENT_HPP