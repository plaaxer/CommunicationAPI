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
    using MSG_TYPE = Segment::MsgType;

private:
    Address _source;
    Address _destination;

    std::vector<char> _buffer; // the buffer contains the message payload

    MSG_TYPE _type;
    
public:

    Message(const Address& source, size_t size) 
        : _source(source), _buffer(size) {}

    Message(const Address& source, const std::string& content) 
        : _source(source), _buffer(content.begin(), content.end()) {}

    Message(size_t size)
        : _source(), _buffer(size) {}

    Message(const std::string& content) 
        : _source(), _buffer(content.begin(), content.end()) {}

    const Address& source() const { return _source; }

    const Address& destination() const { return _destination; }

    void set_source(const Address& source) { _source = source; }

    void set_destination(const Address& destination) { _destination = destination; }

    const MSG_TYPE type() const { return _type; }

    void set_type(MSG_TYPE t) { _type = t; }

    void* data() { return _buffer.data(); }
    const void* data() const { return _buffer.data(); }

    size_t size() const { return _buffer.size(); }

    void resize(size_t new_size) {
        _buffer.resize(new_size);
    }

};

#endif // MESSAGE_HPP