#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>

class Profiler
{
public:
    enum Type { INIT, NICSMH_Recieve, NICSMH_Send, NICRSE_Recieve, NICRSE_Send, PROTOCOLSMH_Recieve, PROTOCOLSMH_Send, 
        PROTOCOLRSE_Recieve, PROTOCOLRSE_Send, COMMUNICATOR_Wake, COMMUNICATOR_Done, COMMUNICATOR_Send};

    Profiler(const std::string& filename = "app.log")
        : outfile(filename, std::ios::app) {
            startTime = std::time(nullptr);
        }

    void profile(Type message) {
        std::lock_guard<std::mutex> lock(mu);
        outfile << timestamp() << messageType(message) << "\n";
        std::cout << timestamp() << messageType(message) << "\n";
        outfile.flush();
    }
private:
    std::ofstream outfile;
    std::mutex mu;
    std::time_t startTime;

    std::string timestamp() {
        std::time_t now = std::time(nullptr);
        double elapsedTime = std::difftime(now, startTime)/1000;
        return std::to_string(elapsedTime);
    }

    std::string messageType(Type type) {
        switch (type) {
            case INIT: return "Profiler initialized";
            case NICSMH_Recieve: return "NIC: Packet received from ShmEngine";
            case NICRSE_Recieve: return "NIC: Packet received from RawSocketEngine";
            case NICSMH_Send: return "NIC: Packet sent via ShmEngine";
            case NICRSE_Send: return "NIC: Packet sent RawSocketEngine";
            case PROTOCOLSMH_Recieve: return "Protocol: Update called by NIC (Shared Memory)";
            case PROTOCOLRSE_Recieve: return "Protocol: Update called by NIC (Raw Socket)";
            case PROTOCOLSMH_Send: return "Protocol: Frame sent to NIC (Shared Memory)";
            case PROTOCOLRSE_Send: return "Protocol: Frame sent to NIC (Raw Socket)";
            case COMMUNICATOR_Wake: return "Communicator: Woke up to process message";
            case COMMUNICATOR_Done: return "Communicator: Finished processing message";
            case COMMUNICATOR_Send: return "Communicator: Sending";
        }
        return "UNKNOWN";
    }
};