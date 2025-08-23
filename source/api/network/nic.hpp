#ifndef NIC_HPP
#define NIC_HPP

template <typename Engine>
// Heranças
class NIC : public Ethernet,
            public Conditional_Data_Observed<Buffer<Ethernet::Frame>, Ethernet::Protocol>,
            private Engine
{
public:
    /* BUFFER_SIZE controla quantos quadros a NIC pode manter 
     simultaneamente em seus buffers de envio/recebimento */

    static const unsigned int BUFFER_SIZE =
        Traits<NIC>::SEND_BUFFERS * sizeof(Buffer<Ethernet::Frame>) +
        Traits<NIC>::RECEIVE_BUFFERS * sizeof(Buffer<Ethernet::Frame>);

    typedef Ethernet::Address Address; // Endereço físico (MAC)
    typedef Ethernet::Protocol Protocol_Number; // Número do protocolo
    typedef Buffer<Ethernet::Frame> Buffer; // Buffer com frame Ethernet
    typedef Conditional_Data_Observer<Buffer<Ethernet::Frame>, Ethernet::Protocol> Observer; // Sujeitos observadores (Observam NIC)
    typedef Conditionally_Data_Observed<Buffer<Ethernet::Frame>, Ethernet::Protocol> Observed; // Sujeito observado (NIC)

protected:
    // Construtor protegido para evitar instâncias diretas de NIC
    NIC();

public:
    // Libera recuross alocados pela NIC
    ~NIC();

    int send(Address dst, Protocol_Number prot, const void * data, unsigned int size);
    int receive(Address * src, Protocol_Number * prot, void * data, unsigned int size);

    Buffer * alloc(Address dst, Protocol_Number prot, unsigned int size);

    int send(Buffer * buf);
    void free(Buffer * buf);

    int receive(Buffer * buf, Address * src, Address * dst, void * data, unsigned int size);

    const Address & address();
    void address(Address address);

    const Statistics & statistics();

    void attach(Observer * obs, Protocol_Number prot); // possibly inherited
    void detach(Observer * obs, Protocol_Number prot); // possibly inherited

private:
    Statistics _statistics; // Estatísticas do tráfego da rede
    Buffer _buffer[BUFFER_SIZE]; // Pool estático de buffers para transmissão e recepção
    // Para manipulação deste array, utilizar os métodos alloc e free (NIC)

};

#include "nic.tpp"

#endif  // NIC_HPP