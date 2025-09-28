#include "profiler.hpp"

Profiler::Profiler(const std::string& filename)
    : outfile(filename, std::ios::app) {
        startTime = std::time(nullptr);
    }

void Profiler::profile(int message) {
    std::lock_guard<std::mutex> lock(mu);
    outfile << timestamp() << messageType(message) << "\n";
    std::cout << timestamp() << messageType(message) << "\n";
    outfile.flush();
}

std::string Profiler::timestamp() {
    std::time_t now = std::time(nullptr);
    double elapsedTime = std::difftime(now, startTime) * 1000;
    return std::to_string(elapsedTime);
}

std::string Profiler::messageType(int type) {
    switch (type) {
        case 0: return "Profiler initialized";
        case 1: return "NIC: Packet received from ShmEngine";
        case 2: return "NIC: Packet received from RawSocketEngine";
        case 3: return "NIC: Packet sent via ShmEngine";
        case 4: return "NIC: Packet sent RawSocketEngine";
        case 5: return "Protocol: Update called by NIC (Shared Memory)";
        case 6: return "Protocol: Update called by NIC (Raw Socket)";
        case 7: return "Protocol: Frame sent to NIC (Shared Memory)";
        case 8: return "Protocol: Frame sent to NIC (Raw Socket)";
        case 9: return "Communicator: Woke up to process message";
        case 10: return "Communicator: Finished processing message";
        case 11: return "Communicator: Sending";
    }
    return "UNKNOWN";
}