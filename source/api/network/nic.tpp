#include "nic.hpp"

// AJEITANDO ERROS AQUI AINDA!!

/**
 * @brief Envia dados através da interface de rede (NIC)
 * @param dst Endereço de destino MAC
 * @param prot Número do protocolo Ethernet
 * @param data Ponteiro para os dados a serem enviados
 * @param size Tamanho dos dados em bytes
 * @return int Número de bytes enviados com sucesso
 */
template <typename Engine>
int NIC<Engine>::send(typename NIC<Engine>::Address dst, 
                      typename NIC<Engine>::Protocol_Number prot,
                      const void * data, unsigned int size)
{
    // Implementação do envio de dados
}

/**
 * @brief Recebe dados da interface de rede
 * @param src Ponteiro para armazenar o endereço de origem
 * @param prot Ponteiro para armazenar o protocolo do pacote recebido
 * @param data Buffer para armazenar os dados recebidos
 * @param size Tamanho máximo do buffer de dados
 * @return int Número de bytes recebidos
 */
template <typename Engine>
int NIC<Engine>::receive(typename NIC<Engine>::Address * src, 
                         typename NIC<Engine>::Protocol_Number * prot, 
                         void * data, unsigned int size)
{
    // To do
    return 0;
}

/**
 * @brief Aloca um buffer para transmissão
 * @param dst Endereço de destino
 * @param prot Número do protocolo
 * @param size Tamanho necessário para os dados
 * @return Buffer* Ponteiro para o buffer alocado, ou:
 *                nullptr se não houver buffers disponíveis
 */
template <typename Engine>
typename NIC<Engine>::Buffer * NIC<Engine>::alloc(typename NIC<Engine>::Address dst,
                            typename NIC<Engine>::Protocol_Number prot, 
                            unsigned int size)
{
    // To do
}

/**
 * @brief Envia um buffer previamente alocado
 * @param buf Ponteiro para o buffer a ser enviado
 * @return int Número de bytes enviados
 */
template <typename Engine>
int NIC<Engine>::send(typename NIC<Engine>::Buffer * buf)
{
    // To do
}

/**
 * @brief Libera um buffer alocado
 * @param buf Ponteiro para o buffer a ser liberado
 * @return void
 */
template <typename Engine>
void NIC<Engine>::free(typename NIC<Engine>::Buffer * buf)
{
    // To do
}

/**
 * @brief Processa um buffer recebido
 * @param buf Buffer recebido
 * @param src Endereço de origem
 * @param dst Endereço de destino
 * @param data Dados extraídos
 * @param size Tamanho máximo do buffer de dados
 * @return int Número de bytes copiados para data
 */
template <typename Engine>
int NIC<Engine>::receive(typename NIC<Engine>::Buffer * buf,
                         typename NIC<Engine>::Address * src, 
                         typename NIC<Engine>::Address * dst, 
                         void * data, unsigned int size)
{
    // To do
}

/**
 * @brief Obtém o endereço MAC da NIC
 * @return const Address& Referência constante para o endereço
 */
template <typename Engine>
const typename NIC<Engine>::Address & NIC<Engine>::address()
{
    // To do
}

/**
 * @brief Configura o endereço físico (MAC) da NIC
 * @param address Novo endereço a ser configurado
 * @return void
 */
template <typename Engine>
void NIC<Engine>::address(typename NIC<Engine>::Address address)
{
    // To do
}

/**
 * @brief Obtém estatísticas do tráfego da rede na NIC
 * @return const Statistics& Referência constante para as estatísticas
 */
template <typename Engine>
const Statistics & NIC<Engine>::statistics()
{
    // To do
}

/**
 * @brief Adiciona um observador para um protocolo específico
 * @param obs Ponteiro para o observador
 * @param prot Número do protocolo a ser observado
 * @return void
 */
template <typename Engine>
void NIC<Engine>::attach(typename NIC<Engine>::Observer * obs, 
                         typename NIC<Engine>::Protocol_Number prot)
{
    // To do
}

/**
 * @brief Remove um observador de um protocolo específico
 * @param obs Ponteiro para o observador
 * @param prot Número do protocolo
 * @return void
 */
template <typename Engine>
void NIC<Engine>::detach(typename NIC<Engine>::Observer * obs, 
                         typename NIC<Engine>::Protocol_Number prot)
{
    // To do
}