// Communication Protocol






// O objeto Protocol observa o NIC e notifica quando um pacote Ethernet chega
// O NIC é responsável por enviar e receber pacotes Ethernet

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

    class Address
    {
    public:
        enum Null;
    public:
        Address();
        Address(const Null & null);
        Address(Physical_Address paddr, Port port);

        // faz o override da conversão implícita para bool
        // se o endereço for nulo, retorna false, caso contrário, retorna true
        // tipo if (address) { ... }
        operator bool() const { return (_paddr || _port); }

        // override para operador de comparação para verificar se o endereço é igual a outro endereço
        // tipo if (address1 == address2) { ... }
        bool operator==(Address a) { return (_paddr == a._paddr) && (_port == a._port); }
    private:
        Physical_Address _paddr;
        Port _port;
    };

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
    Métodos públicos de protocol.
    Pela parte comentada, para o send, a gente deve alocar um buffer no NIC, 
    colocar os dados e o header nele, enviar o buffer e depois liberá-lo.
    */


    static int send(Address from, Address to, const void * data, unsigned int size);
    // Buffer * buf = NIC::alloc(to.paddr, PROTO, sizeof(Header) + size)
    // NIC::send(buf)
    static int receive(Buffer * buf, Address from, void * data, unsigned int size);
    // unsigned int s = NIC::receive(buf, &from.paddr, &to.paddr, data, size)
    // NIC::free(buf)
    // return s;
    static void attach(Observer * obs, Address address);
    static void detach(Observer * obs, Address address);

private:

    void update(typename NIC::Observed * obs, NIC::Protocol_Number prot, Buffer * buf) {

        // observed is responsible for notifying the right observers
        if(!_observed.notify(buf)) // to call receive(...);
            _nic->free(buf);
    }

private:
    NIC * _nic;
    // Channel protocols are usually singletons
    static Observed _observed; // static member to manage observers
};