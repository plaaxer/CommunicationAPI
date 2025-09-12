#include "vehicle/smartdata/component.hpp"
#include "vehicle/smartdata/transducer.hpp"
#include "vehicle/smartdata/local_smartdata.hpp"
#include "vehicle/smartdata/smart_data.hpp"

#include <vector>
#include <memory>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>

// --- HEADERS FOR PROCESS MANAGEMENT ---
#include <unistd.h>      // For fork() and getpid()
#include <sys/wait.h>    // For waitpid()

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
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <number_of_components>" << std::endl;
        return 1;
    }

    int N = std::atoi(argv[1]);
    if (N <= 0) {
        std::cerr << "Number of components must be greater than 0." << std::endl;
        return 1;
    }
    
    std::cout << "--- Starting Vehicle | Parent PID: " << getpid() << " ---" << std::endl;
    std::cout << "--- Spawning " << N << " component processes... ---" << std::endl;

    unsigned int base_port = 9090;
    std::vector<pid_t> child_pids;

    // Available units for now. WILL INCREASE LATER**
    enum UnitType { TEMPERATURE, PRESSURE, HUMIDITY, VOLTAGE };
    std::vector<UnitType> unit_types = { TEMPERATURE, PRESSURE, HUMIDITY, VOLTAGE };

    for (int i = 0; i < N; ++i) {
        // --- CREATE A NEW PROCESS ---
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Failed to fork process!" << std::endl;
            return 1;
        } else if (pid == 0) {
            // --- THIS IS THE CHILD PROCESS ---
            UnitType type = unit_types[i % unit_types.size()];
            std::string name;
            unsigned int device_id = pid; // ease of debugging
            unsigned int port = base_port + i;

            // This unique_ptr will hold the single component for this process
            std::unique_ptr<ComponentHolder> component;

            switch (type) {
                case TEMPERATURE:
                    name = "Temperature Sensor " + std::to_string(i);
                    component = std::make_unique<ConcreteHolder<SmartData::Units::Temperature>>(name, device_id, port);
                    break;
                case PRESSURE:
                    name = "Pressure Sensor " + std::to_string(i);
                    component = std::make_unique<ConcreteHolder<SmartData::Units::Pressure>>(name, device_id, port);
                    break;
                case HUMIDITY:
                    name = "Humidity Sensor " + std::to_string(i);
                    component = std::make_unique<ConcreteHolder<SmartData::Units::Humidity>>(name, device_id, port);
                    break;
                case VOLTAGE:
                    name = "Voltage Sensor " + std::to_string(i);
                    component = std::make_unique<ConcreteHolder<SmartData::Units::Voltage>>(name, device_id, port);
                    break;
            }

            std::cout << "Component process " << name << " (PID: " << getpid() << ") is running.\n";
            
            // Keep the child process alive so the component's threads can run
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }

            // The child process's job is done, it should not continue the loop
            return 0; 
        } else {
            // --- THIS IS THE PARENT PROCESS ---
            // Store the child's PID and continue the loop to spawn more
            child_pids.push_back(pid);
        }
    }

    // --- PARENT PROCESS WAITS FOR ALL CHILDREN ---
    std::cout << "All component processes have been launched. Parent is waiting." << std::endl;
    for (pid_t child_pid : child_pids) {
        waitpid(child_pid, nullptr, 0); // Wait for each child to terminate
    }

    return 0;
}