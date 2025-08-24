#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <atomic>

class Statistics
{
public:
    Statistics() :
        tx_packets(0),
        tx_bytes(0),
        rx_packets(0),
        rx_bytes(0)
    {}

    Statistics(const Statistics& other) :
        tx_packets(other.tx_packets.load()),
        tx_bytes(other.tx_bytes.load()),
        rx_packets(other.rx_packets.load()),
        rx_bytes(other.rx_bytes.load())
    {}

    Statistics& operator=(const Statistics& other)
    {
        if (this != &other)
        {
            tx_packets.store(other.tx_packets.load());
            tx_bytes.store(other.tx_bytes.load());
            rx_packets.store(other.rx_packets.load());
            rx_bytes.store(other.rx_bytes.load());
        }
        
        return *this;
    }

    std::atomic<uint64_t> tx_packets;
    std::atomic<uint64_t> tx_bytes;
    std::atomic<uint64_t> rx_packets;
    std::atomic<uint64_t> rx_bytes;
};

#endif // STATISTICS_HPP