#include "api/network/communicator.hpp"
#include "api/network/protocol.hpp"
#include "api/network/nic.hpp"

#include "smartdata/component.hpp"
#include "smartdata/transducer.hpp"
#include "smartdata/local_smartdata.hpp"
#include "smartdata/smart_data.hpp"

#include <vector>
#include <memory>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <string>

/**
 * Holder e wrapper blueprint to easy instantiating
 */
struct ComponentHolder {
    virtual ~ComponentHolder() = default;
};

template<typename T>
struct ConcreteHolder : ComponentHolder {
    std::unique_ptr<Component<LocalSmartData<Transducer<T>>>> comp;
    ConcreteHolder(const std::string& name, unsigned int id, unsigned int port) {
        comp = std::make_unique<Component<LocalSmartData<Transducer<T>>>>(name, id, port);
    }
};

int main(int argc, char* argv[]) {
    std::cout << "--- Starting Vehicle  ---" << std::endl;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <number_of_components>" << std::endl;
        return 1;
    }

    int N = std::atoi(argv[1]);
    if (N <= 0) {
        std::cerr << "Number of components must be greater than 0." << std::endl;
        return 1;
    }

    // System clock verification to later we calculate the latency
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::cout << "Kernel/VM time: " << std::ctime(&now_time);

    unsigned int base_port = 9090;

    std::vector<std::unique_ptr<ComponentHolder>> components;

    // Available units for now. WILL INCREASE LATER**
    enum UnitType { TEMPERATURE, PRESSURE, HUMIDITY, VOLTAGE };
    std::vector<UnitType> unit_types = { TEMPERATURE, PRESSURE, HUMIDITY, VOLTAGE };

    for (int i = 0; i < N; ++i) {
        UnitType type = unit_types[i % unit_types.size()];  // rotate
        std::string name;
        unsigned int device_id = i + 1;
        unsigned int port = base_port + i;

        switch (type) {
            case TEMPERATURE:
                name = "Temperature Sensor " + std::to_string(i);
                std::cout << "Creating: " << name << " (ID: " << device_id << ", Port: " << port << ")\n";
                components.emplace_back(std::make_unique<ConcreteHolder<SmartData::Units::Temperature>>(name, device_id, port));
                break;
            case PRESSURE:
                name = "Pressure Sensor " + std::to_string(i);
                std::cout << "Creating: " << name << " (ID: " << device_id << ", Port: " << port << ")\n";
                components.emplace_back(std::make_unique<ConcreteHolder<SmartData::Units::Pressure>>(name, device_id, port));
                break;
            case HUMIDITY:
                name = "Humidity Sensor " + std::to_string(i);
                std::cout << "Creating: " << name << " (ID: " << device_id << ", Port: " << port << ")\n";
                components.emplace_back(std::make_unique<ConcreteHolder<SmartData::Units::Humidity>>(name, device_id, port));
                break;
            case VOLTAGE:
                name = "Voltage Sensor " + std::to_string(i);
                std::cout << "Creating: " << name << " (ID: " << device_id << ", Port: " << port << ")\n";
                components.emplace_back(std::make_unique<ConcreteHolder<SmartData::Units::Voltage>>(name, device_id, port));
                break;
        }
    }

    std::cout << "All components started. Press Ctrl+C to exit.\n";
    // to safely keep alive the main thread
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    return 0;
}





// #include "api/network/communicator.hpp"
// #include "api/network/protocol.hpp"
// #include "api/network/nic.hpp"

// #include "smartdata/component.hpp"
// #include "smartdata/transducer.hpp"
// #include "smartdata/local_smartdata.hpp"
// #include "smartdata/smart_data.hpp"


// int main() {
//     std::cout << "--- Starting Vehicle  ---" << std::endl;

//     // getting current time_point
//     auto now = std::chrono::system_clock::now();

//     // converting to time_t to use with std::ctime
//     std::time_t now_time = std::chrono::system_clock::to_time_t(now);

//     // To compare VM clocks
//     std::cout << "Kernel/VM time: " << std::ctime(&now_time);

//     unsigned int PORT = 9090;  // ONLY FOR TESTS!!
//     // Initialize component
//     Component<LocalSmartData<Transducer<SmartData::Units::Humidity>>> thermomether("Thermomether Sensor", 9, PORT);
//     Component<LocalSmartData<Transducer<SmartData::Units::Temperature>>> humiditys("Humidity Sensor 1", 2, PORT+1);


//  }
