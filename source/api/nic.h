#ifndef NIC_H
#define NIC_H

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

    /**
     * @brief Envia dados através da interface de rede (NIC)
     * @param dst Endereço de destino MAC
     * @param prot Número do protocolo Ethernet
     * @param data Ponteiro para os dados a serem enviados
     * @param size Tamanho dos dados em bytes
     * @return int Número de bytes enviados com sucesso
     */
    int send(Address dst, Protocol_Number prot, const void * data, unsigned int size);

    /**
     * @brief Recebe dados da interface de rede
     * @param src Ponteiro para armazenar o endereço de origem
     * @param prot Ponteiro para armazenar o protocolo do pacote recebido
     * @param data Buffer para armazenar os dados recebidos
     * @param size Tamanho máximo do buffer de dados
     * @return int Número de bytes recebidos
     */
    int receive(Address * src, Protocol_Number * prot, void * data, unsigned int size);

    /**
     * @brief Aloca um buffer para transmissão
     * @param dst Endereço de destino
     * @param prot Número do protocolo
     * @param size Tamanho necessário para os dados
     * @return Buffer* Ponteiro para o buffer alocado, ou:
     *                nullptr se não houver buffers disponíveis
     */
    Buffer * alloc(Address dst, Protocol_Number prot, unsigned int size);

    /**
     * @brief Envia um buffer previamente alocado
     * @param buf Ponteiro para o buffer a ser enviado
     * @return int Número de bytes enviados
     */
    int send(Buffer * buf);

    /**
     * @brief Libera um buffer alocado
     * @param buf Ponteiro para o buffer a ser liberado
     * @return void
     */
    void free(Buffer * buf);

    /**
     * @brief Processa um buffer recebido
     * @param buf Buffer recebido
     * @param src Endereço de origem
     * @param dst Endereço de destino
     * @param data Dados extraídos
     * @param size Tamanho máximo do buffer de dados
     * @return int Número de bytes copiados para data
     */
    int receive(Buffer * buf, Address * src, Address * dst, void * data, unsigned int size);

    /**
     * @brief Obtém o endereço MAC da NIC
     * @return const Address& Referência constante para o endereço
     */
    const Address & address();

    /**
     * @brief Configura o endereço físico (MAC) da NIC
     * @param address Novo endereço a ser configurado
     * @return void
     */
    void address(Address address);

    /**
     * @brief Obtém estatísticas do tráfego da rede na NIC
     * @return const Statistics& Referência constante para as estatísticas
     */
    const Statistics & statistics();

    /**
     * @brief Adiciona um observador para um protocolo específico
     * @param obs Ponteiro para o observador
     * @param prot Número do protocolo a ser observado
     * @return void
     */
    void attach(Observer * obs, Protocol_Number prot); // possibly inherited

    /**
     * @brief Remove um observador de um protocolo específico
     * @param obs Ponteiro para o observador
     * @param prot Número do protocolo
     * @return void
     */
    void detach(Observer * obs, Protocol_Number prot); // possibly inherited

private:
    Statistics _statistics; // Estatísticas do tráfego da rede
    Buffer _buffer[BUFFER_SIZE]; // Pool estático de buffers para transmissão e recepção
    // Para manipulação deste array, utilizar os métodos alloc e free (NIC)

};

#endif  // NIC_H