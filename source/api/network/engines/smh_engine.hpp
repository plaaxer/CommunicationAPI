#ifndef SHM_ENGINE_HPP
#define SHM_ENGINE_HPP

#include "../definitions/ethernet.hpp"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <cerrno>
#include <iostream>
#include <cstring>
#include <unistd.h>

/**
 * Most of them are easily accessible through the overall link.
 * overall: https://man7.org/linux/man-pages/man7/svipc.7.html
 * [1] ftok: https://man7.org/linux/man-pages/man3/ftok.3.html
 * [2] shmget: https://man7.org/linux/man-pages/man2/shmget.2.html
 * [3] shmat: https://man7.org/linux/man-pages/man2/shmat.2.html
 * [4] semget: https://man7.org/linux/man-pages/man2/semget.2.html
 * [5] semctl: https://man7.org/linux/man-pages/man2/semctl.2.html
 * 
 * Other references:
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_ipc.h.html
 * 
 */


/**
 * @class ShmEngine
 * @brief An Engine for intra-VM communication using System V Shared Memory.
 *
 * This class implements the Engine interface required by the NIC, but uses
 * shared memory and semaphores for communication between processes running on
 * the same machine. It follows a Producer-Consumer model.
 */
class ShmEngine {



public:
    /**
     * @brief Constructor for the Shared Memory Engine.
     */
    ShmEngine() {

        FILE* fp = fopen(SHM_KEY_FILE, "a"); // creating/touching the file if it doesn't exist (required by ftok)
        if (fp) {
            fclose(fp);
        } else {
            perror("fopen for IPC key file");
            throw std::runtime_error("Failed to create IPC key file");
        }

        // generating the key for shared memory access. See [1]
        key_t shared_key = ftok(SHM_KEY_FILE, PROJ_ID);

        if (shared_key == -1) {
            std::cerr << "Error generating IPC key: " << strerror(errno) << std::endl;
            throw std::runtime_error("Failed to generate IPC key");
        }

        // allocating or getting the shared memory segment. See [2]
        _shared_id = shmget(shared_key, sizeof(Ethernet::Frame), 0666 | IPC_CREAT);

        if (_shared_id == -1) {
            std::cerr << "Error creating/getting shared memory segment: " << strerror(errno) << std::endl;
            throw std::runtime_error("Failed to create/get shared memory segment");
        }

        // attaching the shared memory segment to this process's address space. See [3]
        _shared_buffer = shmat(_shared_id, nullptr, 0);

        if (_shared_buffer == (void*)-1) { // cool way to check for errors on low-level functions hah
            std::cerr << "Error attaching shared memory segment: " << strerror(errno) << std::endl;
            throw std::runtime_error("Failed to attach shared memory segment");
        }
        
        // creating or getting the semaphore set. See [4]
        _sem_id = semget(shared_key, 2, IPC_CREAT | IPC_EXCL | 0666); // 2 semaphores: empty and full
        // IPLC_EXCL ensures it fails if already exists

        if (_sem_id != -1) {
            // this should only happen for a single process

            std::cout << "Semaphore set created. Initializing values..." << std::endl;

            union semun arg;

            // Initialize the semaphores. See [5]

            // Initialize the EMPTY semaphore to 1 (the buffer is initially empty).
            arg.val = 1;
            if (semctl(_sem_id, EMPTY_SEM, SETVAL, arg) == -1) {
                perror("semctl (EMPTY_SEM) initialization failed");
                // Handle error: perhaps cleanup and exit.
            }

            // Initialize the FULL semaphore to 0 (the buffer is initially not full).
            arg.val = 0;
            if (semctl(_sem_id, FULL_SEM, SETVAL, arg) == -1) {
                perror("semctl (FULL_SEM) initialization failed");
                // Handle error: perhaps cleanup and exit.
            }
        
        // See [4]
        } else if (errno == EEXIST) {

            _sem_id = semget(shared_key, 2, 0666);
            if (_sem_id == -1) {
                std::cerr << "Error getting existing semaphore set: " << strerror(errno) << std::endl;
                throw std::runtime_error("Failed to get existing semaphore set");
            }

        } else {
            std::cerr << "Error creating/getting semaphore set: " << strerror(errno) << std::endl;
            throw std::runtime_error("Failed to create/get semaphore set");
        }


        std::cout << "ShmEngine initialized." << std::endl;
    }

    /**
     * @brief Destructor for the Shared Memory Engine.
     */
    ~ShmEngine() {
        // --- TODO: Implement IPC Cleanup ---

        // 1. Detach the shared memory segment.
        //    - Use shmdt() with the shared memory pointer.
        //    - Handle errors.

        // 2. (Optional but recommended) Mark the shared memory and semaphores for deletion.
        //    - This should only be done by the "last" process to detach.
        //    - Use shmctl() with IPC_RMID.
        //    - Use semctl() with IPC_RMID.
        //    - A more robust implementation might involve a reference counter in the shared memory.

        std::cout << "ShmEngine destroyed." << std::endl;
    }

    /**
     * @brief Returns a dummy MAC address for local communication.
     * In a shared memory context, a real MAC address is not used, but the NIC
     * interface requires this method.
     */
    const Ethernet::MAC& address() {
        // This is a placeholder. For communication between local processes,
        // a MAC address isn't truly necessary. We return a static, non-zero
        // address to satisfy the NIC's interface.
        return _dummy_mac;
    }

    /**
     * @brief Sends a frame to another process via shared memory.
     * This method will be called by NIC::send.
     * @return Number of bytes written, or -1 on error.
     */
    int send(const unsigned char* dst_mac, unsigned short protocol, const void* data, unsigned int size) {
        // --- TODO: Implement Producer Logic ---

        // 1. Wait for the buffer to be empty.
        //    - Perform a 'wait' (decrement) operation on the 'empty' semaphore.
        //    - Use semop(). This will block if another process is currently writing
        //      or if the consumer has not yet read the previous data.

        // 2. Write the data to the shared memory segment.
        //    - You will need to construct a full Ethernet::Frame structure in memory
        //      and then copy it to the shared buffer.
        //    - The `dst_mac`, `protocol`, and `data`/`size` parameters contain all the
        //      necessary information. The source MAC can be obtained from `this->address()`.
        //    - Example:
        //      Ethernet::Frame frame;
        //      frame.header.shost = this->address();
        //      frame.header.dhost = Ethernet::MAC(dst_mac);
        //      frame.header.type = htons(protocol);
        //      frame.data_length = size;
        //      memcpy(frame.data, data, size);
        //      memcpy(_shared_buffer, &frame, sizeof(Ethernet::Header) + size);
        //      // Also store the total size in a shared variable if needed.

        // 3. Signal that the buffer is now full.
        //    - Perform a 'signal' (increment) operation on the 'full' semaphore.
        //    - Use semop(). This will unblock the consumer process waiting in receive().
        
        // 4. Return the total number of bytes written.
        return -1; // Placeholder
    }

    /**
     * @brief Receives a frame from another process via shared memory.
     * This is a blocking call, intended to be run in the NIC's receiver thread.
     * @return Number of bytes read, or -1 on error.
     */
    int receive(void* frame_buffer, int max_size) {
        // --- TODO: Implement Consumer Logic ---

        // 1. Wait for the buffer to be full.
        //    - Perform a 'wait' (decrement) operation on the 'full' semaphore.
        //    - Use semop(). This will block until a producer process calls send() and
        //      signals the 'full' semaphore.

        // 2. Read the data from the shared memory segment.
        //    - Copy the contents from the shared buffer into the `frame_buffer` provided
        //      by the NIC.
        //    - Respect the `max_size`. You might need to read a size field from
        //      the shared memory first.

        // 3. Signal that the buffer is now empty.
        //    - Perform a 'signal' (increment) operation on the 'empty' semaphore.
        //    - Use semop(). This will unblock a producer process that might be waiting in send().

        // 4. Return the number of bytes read into frame_buffer.
        return -1; // Placeholder
    }

    union semun {
            int val;                // Value for SETVAL
            struct semid_ds *buf;   // Buffer for IPC_STAT, IPC_SET
            unsigned short *array;  // Array for GETALL, SETALL
        };

    enum { EMPTY_SEM = 0, FULL_SEM = 1 };

private:
    // --- TODO: Add private members for IPC management ---

    // A pointer to the attached shared memory segment.
    // Should be cast to a struct representing the shared data layout.
    void* _shared_buffer = nullptr;

    // IDs for the shared memory segment and semaphore set.
    int _shared_id = -1;
    int _sem_id = -1;

    // not static to avoid some defining issues
    const char* SHM_KEY_FILE = "/tmp/shm_key_file"; // though file does not exist yet our code should handle that
    const int PROJ_ID = 100;
    const int NUMBER_OF_SEMS = 2;
    
    // A static dummy MAC address for local IPC.
    static Ethernet::MAC _dummy_mac;
};

// Initialize the static dummy MAC address.
// Using a value from the "Locally Administered Address Ranges".
//Ethernet::MAC ShmEngine::_dummy_mac = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

#endif // SHM_ENGINE_HPP