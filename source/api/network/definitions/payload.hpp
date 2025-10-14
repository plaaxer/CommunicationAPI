#ifndef PAYLOAD_HPP
#define PAYLOAD_HPP

#include "api/network/definitions/teds.hpp"

class Payload {
    private:
        void* _data;
        unsigned int _size;
        unsigned int _type;

    public:
        Payload(void* data, unsigned int size, unsigned int type)
            : _data(data), _size(size), _type(type) {}

        template <typename T>
        T* data() {return reinterpret_cast<T*>(_data);}

} __attribute__((packed));

#endif // PAYLOAD_HPP