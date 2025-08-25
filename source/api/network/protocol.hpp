#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "api/network/protocol_address.hpp"
#include "api/network/traits.hpp"
#include "api/network/nic.hpp"
#include "api/network/buffer.hpp"
#include "api/observer/conditionally_data_observed.h"
#include "api/observer/conditional_data_observer.hpp"

/**
 * @brief Protocol Handler. Observes the NIC for incoming Ethernet frames.
 * Also responsible to repass the frames to subscribed Communicators.
 */
template <typename NIC>
class Protocol : private NIC::Observer
{
public:
    // deve-se definir o número do protocolo Ethernet em algum lugar
    static const typename NIC::Protocol_Number PROTO =
        Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER;


    // typedef typename NIC::Buffer Buffer;
    typedef typename NIC::Address Physical_Address;
    // não há tipo para Port ainda, devemos definir! MUDAR O INT DEPOIS
    typedef int Port;
    typedef Conditional_Data_Observer<Buffer<Ethernet::Frame>, Port> Observer;
    typedef Conditionally_Data_Observed<Buffer<Ethernet::Frame>, Port> Observed;

    class Header;

    // Define o tamaho máximo do pacote, que é o tamanho máximo do NIC menos o tamanho do cabeçalho
    // MTU (Maximum Transmission Unit) é o tamanho máximo de um pacote que pode ser transmitido
    static const unsigned int MTU = NIC::MTU - sizeof(Header);
    typedef unsigned char Data[MTU]; // array de bytes de transmissão

    class Packet : public Header
    {
    public:
        Packet();
        Header * header();
        template<typename T>

        // manipulação bem low-level de ponteiro para acessar os dados do pacote
        // basicamente, ela força a interpretação do pacote como um tipo específico (T)
        T * data() { return reinterpret_cast<T *>(&_data); }
    private:
        Data _data;
    } __attribute__((packed)); // structs geralmente têm padding, packed é necessário para
                               // garantir que o tamanho do pacote seja o esperado (remover padding)

protected:
    // inicializa o protocolo com o NIC referente
    Protocol(NIC * nic) : _nic(nic) { _nic->attach(this, PROTO); }

public:
    ~Protocol() { _nic->detach(this, PROTO); }

    /*
    Para o send, a gente deve alocar um buffer no NIC, 
    colocar os dados e o header nele, enviar o buffer e depois liberá-lo.
    */
    static int send(ProtocolAddress from, ProtocolAddress to, const void * data, unsigned int size)
    {
        // To do
        return -1;
    }

    static int receive(Buffer<Ethernet::Frame> * buf, ProtocolAddress from, void * data, unsigned int size)
    {
        // To do
        // receive é um "unpack"
        return -1;
    }

    static void attach(Observer * obs, ProtocolAddress address)
    {
        // To do
    }

    static void detach(Observer * obs, ProtocolAddress address)
    {
        // To do
    }

private:

    void update(typename NIC::Observed * obs, typename NIC::Protocol_Number prot, Buffer<Ethernet::Frame> * buf) override
    {
        // observed is responsible for notifying the right observers
        // REMOVE THIS FALSE AFTER. IS ONLY FOR COMPILE TESTS
        if(!_observed.notify(false, buf)) // to call receive(...);
            _nic->free(buf);
    }

private:
    NIC * _nic;
    // Channel protocols are usually singletons
    static Observed _observed;  // static member to manage observers
};


#endif  // PROTOCOL_HPP