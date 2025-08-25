#ifndef MESSAGE_HPP
#define MESSAGE_HPP
#include <string>

class Message {
    public: 
        std::string data;
        size_t size;
        Message(std::string d) {
            data = d;
            size = sizeof(d);
        }
};
#endif // MESSAGE_HPP