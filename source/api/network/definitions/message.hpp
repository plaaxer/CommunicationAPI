#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/ethernet.hpp"
#include "api/network/definitions/segment.hpp"
#include "api/network/definitions/teds.hpp"

class Message 
{
public:
    using Port = Address::Port;
    using MAC  = Ethernet::MAC;

private:
    Address _source;
    Address _destiny;
    Segment::MsgType _type;
    std::vector<char> _payload;

public:

    // Constructors for SENDING messages

    /**
     * @brief Smart constructor for a TEDS data response message.
     */
    Message(const Address& dest, TEDS::Type teds_type, float value)
        : _destiny(dest), _type(Segment::MsgType::TEDS) {
        _payload = TEDS::create_response_payload(teds_type, value);
    }

    /**
     * @brief Smart constructor for a generic control message.
     */
    Message(const Address& dest, const std::vector<char>& control_data)
        : _destiny(dest), _type(Segment::MsgType::CONTROL), _payload(control_data) {}
    
    
    // Constructors for RECEIVING messages

    /**
     * @brief Creates an empty message with a pre-allocated buffer, ready for receiving.
     * @param buffer_size The maximum number of bytes this message can hold.
     */
    Message(size_t buffer_size)
        : _type(Segment::MsgType::CONTROL) { // Default type
        // Pre-allocates memory. The size will be adjusted by receive().
        _payload.resize(buffer_size); 
    }

    /**
     * @brief Default constructor. Creates an empty message.
     */
    Message() : _type(Segment::MsgType::CONTROL) {}


    // Provides a writeable pointer to the internal buffer's data.
    void* data() { return _payload.data(); }
    
    // Provides a read-only pointer to the internal buffer's data.
    const void* data() const { return _payload.data(); }

    // Returns the current size of the valid data in the payload.
    size_t size() const { return _payload.size(); }
    
    //  Returns the total allocated capacity of the buffer. 
    // This is used by Communicator::receive to prevent buffer overflows.
    size_t capacity() const { return _payload.capacity(); }
    
    // Resizes the internal vector. 
    // This is used by Communicator::receive after unpacking the payload.
    void resize(size_t new_size) { _payload.resize(new_size); }

    Segment::MsgType get_type() const { return _type; }
    void set_type(Segment::MsgType type) { _type = type; }
    
    const std::vector<char>& get_payload() const { return _payload; }
    void set_payload(const std::vector<char>& payload) { _payload = payload; }

    const Address& source() const { return _source; }
    void set_source(const Address& source) { _source = source; }
    
    const Address& destiny() const { return _destiny; }
    void set_destiny(const Address& destiny) { _destiny = destiny; }
};

#endif // MESSAGE_HPP