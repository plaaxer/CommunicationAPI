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
    Address _destiny;
    std::vector<char> _buffer;
    
public:

    Message(const Address& source, size_t size) 
        : _source(source), _buffer(size) {}

    Message(const Address& source, const std::string& content) 
        : _source(source), _buffer(content.begin(), content.end()) {}

    Message(size_t size) 
        : _source(), _buffer(size) {}

    Message(const std::string& content) 
        : _source(), _buffer(content.begin(), content.end()) {}

    const Address& source() const {
        return _source;
    }

    const Address& destiny() const {
        return _destiny;
    }

    void set_source(const Address& source) {
        _source = source;
    }

    void set_destiny(const Address& destiny) {
        _destiny = destiny;
    }

    // To review this remove. We can directly use the Address::port(). 
    // (<message>.source.port())

    // MAC source_mac() const {
    //     return _source.paddr();
    // }

    // Port source_port() const {
    //     return _source.port();
    // }

    void* data() { return _buffer.data(); }
    const void* data() const { return _buffer.data(); }
    size_t size() const { return _buffer.size(); }
};

#endif // MESSAGE_HPP