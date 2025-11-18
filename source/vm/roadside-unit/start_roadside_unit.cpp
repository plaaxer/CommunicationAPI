#include "vm/gateway.hpp"
#include "utils/profiler.cpp"
#include "api/network/engines/smh_engine.hpp"
#include "api/network/ptp/ptp_roles.hpp"
#include "api/network/groups/group_roles.hpp"
#include "api/network/definitions/quadrant.hpp"
#include "api/network/crypto/i_crypto_provider.hpp"
#include "api/network/crypto/xor_crypto_provider.hpp"
#include "api/network/crypto/crypto_service.hpp"

#include <vector>
#include <memory>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>

#include <unistd.h>
#include <sys/wait.h>

// ** IMPORTANT **: For now, passing arguments through terminal for start_roadsite_unit to be able to create 4 different instances of RSUs, each in a different QUADRANT. Makefile needs to be changed if this ends up being our final plan.
int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << "<quadrant>" << std::endl;
        std::cerr << "Quadrants: 0=NORTH, 1=EAST, 2=SOUTH, 3=WEST" << std::endl;
        return 1;
    }

    int quadrant_num = std::atoi(argv[1]);
    if (quadrant_num < 0 || quadrant_num > 3) { std::cerr << "Invalid quadrant. Must be a whole number from 0 to 3" << std::endl; return 1; }
    Quadrant quadrant = static_cast<Quadrant>(quadrant_num);

    CryptoService::init(std::make_unique<XorCryptoProvider>());

    std::cout << "--- Starting Roadside Unit | Parent PID: " << getpid() << " ---" << std::endl;

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

    /** 
     * RUN THE SHM_ENGINE'S STATIC SHARED MEMORY IPC BOOTSTRAP FUNCTION
     * This runs only in the parent process, which after creating all the structures, then detaches itself from the shm 
     * */
    if (!ShmEngine::bootstrapIPC()) {
        std::cerr << "Failed to bootstrap shared memory IPC" << std::endl;
        return 1;
    }

    std::vector<pid_t> child_pids;

    // --- CREATE GATEWAY ---
    std::cout << "--- Spawning Gateway RCU process... ---" << std::endl;
    pid_t gateway_pid = fork();

    if (gateway_pid < 0) {
        std::cerr << "Failed to fork gateway process!" << std::endl;
        return 1;
    } 
    if (gateway_pid == 0) {
        Gateway gateway(PtpRole::MASTER, GroupRole::LEADER, quadrant);
        gateway.run();
        return 0;
    }
    child_pids.push_back(gateway_pid);
    std::cout << "--- Gateway RCU process spawned with PID: " << gateway_pid << " ---" << std::endl;

    // --- PARENT PROCESS WAITS FOR ALL CHILDREN ---
    std::cout << "All component processes have been launched. Parent is waiting." << std::endl;
    for (pid_t child_pid : child_pids) {
        waitpid(child_pid, nullptr, 0); // Wait for each child to terminate
    }

    return 0;
}