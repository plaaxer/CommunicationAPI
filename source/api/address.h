// #ifndef ADDRESS_H
// #define ADDRESS_H

// class Address
// {
// public:
//     enum Null;
// public:
//     Address();
//     Address(const Null & null);
//     Address(Physical_Address paddr, Port port);

//     // faz o override da conversão implícita para bool
//     // se o endereço for nulo, retorna false, caso contrário, retorna true
//     // tipo if (address) { ... }
//     operator bool() const { return (_paddr || _port); }

//     // override para operador de comparação para verificar se o endereço é igual a outro endereço
//     // tipo if (address1 == address2) { ... }
//     bool operator==(Address a) { return (_paddr == a._paddr) && (_port == a._port); }
    
// private:
//     Physical_Address _paddr;
//     Port _port;
// };

// #endif // ADDRESS_H