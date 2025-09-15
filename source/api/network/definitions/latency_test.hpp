#ifndef LATENCY_TEST
#define LATENCY_TEST

#include <cstdint>
#include <iostream>
#include <string>

class LatencyTest
{

public:
    typedef uint64_t Timestamp;
    typedef uint64_t SenderId;

    enum Type {
        PING = 1,
        ECHO = 2,
        UNDEFINED = 3
    };

public:

    struct Header {
        Type type;
        SenderId sender_id;


        Header(Type t, SenderId sid = 0)
            : type(t), sender_id(sid) {}


        Header() : type(UNDEFINED), sender_id(0) {}   // default constructor
    
    }; // __attribute__((packed));  // enum could have align issues in this case**

    struct Packet {
        Header header;
        Timestamp timestamp;

        Packet(const Header& h, Timestamp ts = 0)
            : header(h), timestamp(ts) {}

        size_t size() const { return sizeof(Header) + sizeof(Timestamp); }

        template<typename T>
        T get_data() const {
            T value;
            std::memcpy(&value, &timestamp, sizeof(T));
            return value;
        }

        Header get_header() { return header; }
    }; // __attribute__((packed));  // int have align issues in this case**

};

#endif