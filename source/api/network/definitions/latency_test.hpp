#ifndef LATENCY_TEST
#define LATENCY_TEST

#include <cstdint>
#include <iostream>
#include <string>

class LatencyTest
{

public:
    typedef uint64_t Timestamp;

private:

    enum Type {
        PING = 1,
        ECHO = 2
    };

public:

    struct Header {
        Type type;

        Header(Type t)
            : type(t) {}
    } // __attribute__((packed));  // enum could have align issues in this case**

    struct Packet {
        Header header;
        Timestamp timestamp;

        Packet(const Header& h = {}, Timestamp ts = 0)
            : header(h), timestamp(ts) {}

        size_t size() const { return sizeof(Header) + sizeof(Timestamp); }

        template<typename T>
        T get_data() const {
            T value;
            std::memcpy(&value, &timestamp, sizeof(T));
            return value;
        }

        Header header() const { return header; }
    }; // __attribute__((packed));  // int have align issues in this case**

};

#endif