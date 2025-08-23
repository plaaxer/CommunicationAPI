#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

/**
 * @brief Protocol Handler. Observes the NIC for incoming Ethernet frames.
 * Also responsible to repass the frames to subscribed Communicators.
 */

#include "address.hpp"
#include "traits.hpp"

template <typename NIC>
class Protocol : private typename NIC::Observer
{
public:
    // deve-se definir o número do protocolo Ethernet em algum lugar
    static const typename NIC::Protocol_Number PROTO =
        Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER;


    typedef typename NIC::Buffer Buffer;
    typedef typename NIC::Address Physical_Address;
    // não há tipo para Port ainda, devemos definir
    typedef XXX Port;
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
    static int send(Address from, Address to, const void * data, unsigned int size) 
    {
        // To do
    }

    static int receive(Buffer * buf, Address from, void * data, unsigned int size)
    {
        // To do
    }

    static void attach(Observer * obs, Address address)
    {
        // To do
    }

    static void detach(Observer * obs, Address address);
    {
        // To do
    }

private:

    void update(typename NIC::Observed * obs, NIC::Protocol_Number prot, Buffer * buf)
    {
        // observed is responsible for notifying the right observers
        if(!_observed.notify(buf)) // to call receive(...);
            _nic->free(buf);
    }

private:
    NIC * _nic;
    // Channel protocols are usually singletons
    static Observed _observed;  // static member to manage observers
};


#endif  // PROTOCOL_HPP