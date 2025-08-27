#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>

class Message 
{
private:
    std::vector<char> _buffer;
    
public:
    Message(size_t size) : _buffer(size) {}
    Message(const std::string& content) : _buffer(content.begin(), content.end()) {}

    void* data() { return _buffer.data(); }
    const void* data() const { return _buffer.data(); }
    size_t size() const { return _buffer.size(); }
};

#endif // MESSAGE_HPP