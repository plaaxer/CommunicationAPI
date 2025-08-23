// #ifndef PACKET_H
// #define PACKET_H

// class Packet : public Header
// {
// public:
//     Packet();
//     Header * header();
//     template<typename T>

//     // manipulação bem low-level de ponteiro para acessar os dados do pacote
//     // basicamente, ela força a interpretação do pacote como um tipo específico (T)
//     T * data() { return reinterpret_cast<T *>(&_data); }
// private:
//     Data _data;
// } __attribute__((packed)); // structs geralmente têm padding, packed é necessário para
//                             // garantir que o tamanho do pacote seja o esperado (remover padding)

// #endif // PACKET_H