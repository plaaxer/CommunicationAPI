#ifndef BUFFER_HPP
#define BUFFER_HPP

template<typename T>
class Buffer {
public:
    // Wrapper basic. To be implemented later
    Buffer(unsigned int size) : _size(size) {
        _data = new T[size];
    }

    ~Buffer() {
        delete[] _data;
    }

    T* data() {
        return _data;
    }

    unsigned int size() const {
        return _size;
    }

private:
    T* _data;
    unsigned int _size;
};

#endif