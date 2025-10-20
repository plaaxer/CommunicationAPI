#include "vehicle/components/component.hpp"
#include "vehicle/smartdata/transducer.hpp"
#include "vehicle/smartdata/local_smartdata.hpp"
#include "vehicle/smartdata/smart_data.hpp"
#include "vehicle/gateway.hpp"
#include "utils/profiler.cpp"
#include "api/network/engines/smh_engine.hpp"

#include <vector>
#include <memory>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>

#include <unistd.h>
#include <sys/wait.h>

/**
 * Holder e wrapper blueprint to easy instantiating
 */
struct ComponentHolder {
    virtual ~ComponentHolder() = default;
};

template<TEDS::Type UnitTag>
struct ConcreteHolder : ComponentHolder {
    using C = Component<LocalSmartData<Transducer<UnitTag>>>;

    std::unique_ptr<C> comp;
    
    ConcreteHolder(const std::string& name, TransducerType transducer_type, TEDS::Period interval_ms) {
    
        comp = std::make_unique<C>(name, transducer_type, interval_ms); // TODO: arrumar argumentos do construtor de component
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


    /*
    IT'S GOOD PRACTICE TO SET THE MASK RULE FOR SIGNALS AS SOON AS POSSIBLE.
    THIS WAY, ALL NEW PROCESSES (AND THREADS) WILL INHERIT THE PATTERN.    
    */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGIO);
    if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) {
        perror("FATAL: Unable to set the signal mask for the parent process");
        return 1;
    }

    // Run the smh_engine's static shared memory IPC bootstrap function
    // This runs only in the parent process, which after creating all the structures, then detaches itself from the shm
    if (!ShmEngine::bootstrapIPC()) {
        std::cerr << "Failed to bootstrap shared memory IPC" << std::endl;
        return 1;
    }

    std::vector<pid_t> child_pids;

    std::cout << "--- Spawning Gateway RCU process... ---" << std::endl;
    pid_t gateway_pid = fork();

    if (gateway_pid < 0) {
        std::cerr << "Failed to fork gateway process!" << std::endl;
        return 1;
    } 
    
    if (gateway_pid == 0) {
        Gateway gateway;
        gateway.run();
        return 0;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 1);

    // Randomly choose if the sequence starts with SENSOR (0) or ACTUATOR (1)
    int random_start_offset = distrib(gen);

    child_pids.push_back(gateway_pid);
    std::cout << "--- Gateway RCU process spawned with PID: " << gateway_pid << " ---" << std::endl;

    // Available units for now. WILL INCREASE LATER**
    enum UnitType { TEMPERATURE };
    std::vector<UnitType> unit_types = { TEMPERATURE };

    for (int i = 0; i < N; ++i) {

        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Failed to fork process!" << std::endl;
            return 1;
            
        } else if (pid == 0) {

            // we randomly choose which one we start with and then alternate with the other.
            TransducerType transducer_type = ((i + random_start_offset) % 2 == 0) ? TransducerType::SENSOR : TransducerType::ACTUATOR;

            UnitType type = unit_types[i % unit_types.size()];
            std::string name;
            std::unique_ptr<ComponentHolder> component;

            switch (type) {
                case TEMPERATURE:
                    name = "Temperature " + std::string(transducer_type == TransducerType::SENSOR ? "Sensor " : "Actuator ") + std::to_string(i);
                    component = std::make_unique<ConcreteHolder<TEDS::TEMPERATURE>>(name,
                        transducer_type,
                        2000
                    );
                    break;
                // case PRESSURE:
                //     name = "Pressure " + std::string(transducer_type == TransducerType::SENSOR ? "Sensor " : "Actuator ") + std::to_string(i);
                //     component = std::make_unique<ConcreteHolder<TEDS::PRESSURE>>(name,
                //         transducer_type,
                //         2000
                //     );
                //     break;
                // case DENSITY:
                //     name = "Density " + std::string(transducer_type == TransducerType::SENSOR ? "Sensor " : "Actuator ") + std::to_string(i);
                //     component = std::make_unique<ConcreteHolder<TEDS::DENSITY>>(name,
                //         transducer_type,
                //         2000
                //     );
                //     break;
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