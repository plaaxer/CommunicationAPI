#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/ethernet.hpp"

class Message 
{
public:
    using Port = Address::Port;
    using MAC  = Ethernet::MAC;

private:
    Address _source;
    Address _destination;
    Segment::MsgType _type;
    std::vector<char> _payload;

public:

    /**
     * @brief Constructor for a TEDS response message (sending data).
     */
    Message(const Address& dest, TEDS::Type teds_type, float value)
        : _destination(dest), _type(Segment::MsgType::TEDS) {
        _payload = TEDS::create_response_payload(teds_type, value);
    }

    /**
     * @brief Constructor for a control message.
     */
    Message(const Address& dest, const std::vector<char>& control_data)
        : _destination(dest), _type(Segment::MsgType::CONTROL), _payload(control_data) {}
    
    Message() : _type(Segment::MsgType::CONTROL) {}

    void* data() { return _payload.data(); }
    size_t size() const { return _payload.size(); }

    Segment::MsgType get_type() const { return _type; }
    const std::vector<char>& get_payload() const { return _payload; }
    
    void set_payload(const std::vector<char>& payload) { _payload = payload; }
    void set_type(Segment::MsgType type) { _type = type; }
    void set_source(const Address& source) { _source = source; }
};

#endif // MESSAGE_HPP