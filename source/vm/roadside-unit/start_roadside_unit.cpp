#include "vm/gateway.hpp"
#include "utils/profiler.cpp"
#include "api/network/engines/smh_engine.hpp"
#include "api/network/ptp/ptp_roles.hpp"
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

int main() {

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
        Gateway gateway(PtpRole::MASTER);
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