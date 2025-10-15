#ifndef PAYLOAD_HPP
#define PAYLOAD_HPP

class Segment {
    private:
        unsigned int _type;
        void* _data; // the payload

    public:
        Segment (void* data, unsigned int size, unsigned int type)
            : _data(data), _type(type) {}

        template <typename T>
        T* data() {return reinterpret_cast<T*>(_data);}

} __attribute__((packed));

#endif // PAYLOAD_HPP