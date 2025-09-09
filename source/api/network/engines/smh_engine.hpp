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
#include <arpa/inet.h>

/**
 * Most of them are easily accessible through the overall link.
 * overall: https://man7.org/linux/man-pages/man7/svipc.7.html
 * [1] ftok: https://man7.org/linux/man-pages/man3/ftok.3.html
 * [2] shmget: https://man7.org/linux/man-pages/man2/shmget.2.html
 * [3] shmat: https://man7.org/linux/man-pages/man2/shmat.2.html
 * [4] semget: https://man7.org/linux/man-pages/man2/semget.2.html
 * [5] semctl: https://man7.org/linux/man-pages/man2/semctl.2.html
 * [6] semop: https://man7.org/linux/man-pages/man2/semop.2.html
 * 
 * Other references:
 * https://hildstrom.com/projects/2015/08/ipc_sysv_posix/index.html
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_ipc.h.html
 */

static const uint8_t DUMMY_MAC_BYTES[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

/**
 * @class ShmEngine
 * @brief An Engine for intra-VM communication using System V Shared Memory.
 */
class ShmEngine {

/*
TODO: this current class assumes a very straightforward producer consumer model, in which, when a message is added in the shared block, the first process
to read it will remove it from the message queue. That means that it doesn't work as a broadcast in essence. It shall be fine for the first testing between
two vms, but we must redesign this eventually.

One may argue that i'm lazy and afraid that not even this will work; i call it being pragmatic. FM
*/

public:
    /**
     * @brief Constructor for the Shared Memory Engine.
     */
    ShmEngine() {

        FILE* fp = fopen(SHM_KEY_FILE, "a"); // creating/touching the file if it doesn't exist (required by ftok)
        if (fp) {
            fclose(fp);
        } else {
            _cleanupAndThrow("Failed to create IPC key file");
        }

        // generating the key for shared memory access. See [1]
        key_t shared_key = ftok(SHM_KEY_FILE, PROJ_ID);

        if (shared_key == -1) {
            _cleanupAndThrow("Failed to generate IPC key");;
        }

        // allocating or getting the shared memory segment. See [2]
        _shared_id = shmget(shared_key, sizeof(SharedBlock), 0666 | IPC_CREAT);

        if (_shared_id == -1) {
            _cleanupAndThrow("Failed to create/get shared memory segment");
        }

        // attaching the shared memory segment to this process's address space. See [3]
        void* shared_pointer = shmat(_shared_id, nullptr, 0);

        if (shared_pointer == (void*)-1) { // cool way to check for errors on low-level functions hah
            _cleanupAndThrow("Failed to attach shared memory segment");
        }

        _shared_block = static_cast<SharedBlock*>(shared_pointer);
        
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
                _cleanupAndThrow("semctl (EMPTY_SEM) initialization failed");
            }

            // Initialize the FULL semaphore to 0 (the buffer is initially not full).
            arg.val = 0;
            if (semctl(_sem_id, FULL_SEM, SETVAL, arg) == -1) {
                _cleanupAndThrow("semctl (FULL_SEM) initialization failed");
            }
        
        // See [4]
        } else if (errno == EEXIST) {

            _sem_id = semget(shared_key, 2, 0666);
            if (_sem_id == -1) {
                _cleanupAndThrow("Failed to get existing semaphore set");
            }

        } else {
            _cleanupAndThrow("Failed to create/get semaphore set");
        }


        std::cout << "ShmEngine initialized." << std::endl;
    }

    /**
     * @brief Destructor for the Shared Memory Engine.
     */
    ~ShmEngine() {

        if (!_shared_block) {
            std::cout << "Warning: destructor of ShmEngine called with no attached shared memory";
            return;
        }

        // decrement client counter and check if we are the last one --> MUST BE ATOMIC
        int remaining_clients = --(_shared_block->client_count);
        std::cout << "Detaching from shared memory. Clients remaining: " << remaining_clients << std::endl;

        // --- 1. Detach the shared memory segment ---
        if (shmdt(_shared_block) == -1) {
            std::cerr << "ShmEngine warning: shmdt failed: " << strerror(errno) << std::endl;
        }

        // --- 2. If we are the last process, remove IPC objects from the system ---
        if (remaining_clients == 0) {
            std::cout << "Last client detached. Removing IPC resources..." << std::endl;

            // Remove the shared memory segment
            if (shmctl(_shared_id, IPC_RMID, nullptr) == -1) {
                std::cerr << "ShmEngine warning: shmctl(IPC_RMID) failed: " << strerror(errno) << std::endl;
            } else {
                std::cout << "Shared memory segment removed successfully." << std::endl;
            }

            // Remove the semaphore set
            if (semctl(_sem_id, 0, IPC_RMID) == -1) {
                std::cerr << "ShmEngine warning: semctl(IPC_RMID) failed: " << strerror(errno) << std::endl;
            } else {
                std::cout << "Semaphore set removed successfully." << std::endl;
            }
        }

        std::cout << "ShmEngine destroyed." << std::endl;
    }

    /**
     * @brief Returns a dummy MAC address for local communication.
     * In a shared memory context, a real MAC address is not used, but the NIC
     * interface requires this method.
     */
    const Ethernet::MAC& address() {
        // Isn't necessary for intra-vm comms.
        // I feel like for when the gateway is redirecting outside traffic to another component it'll just overwrite the source address anyways
        return _dummy_mac;
    }

    /**
     * @brief Sends a frame to another process via shared memory.
     * This method will be called by NIC::send.
     * @return Number of bytes written, or -1 on error.
     */
    int send(const unsigned char* dst_mac, unsigned short protocol, const void* data, unsigned int size) {

        struct sembuf acquire_empty = {EMPTY_SEM, -1, SEM_UNDO};

        // See [6]
        if (semop(_sem_id, &acquire_empty, 1) == -1) {
            perror("semop (wait on EMPTY_SEM) failed in send");
            return -1; //
        }

        // PROTECTED ZONE

        SharedBlock* block = static_cast<SharedBlock*>(_shared_block);

        block->frame_buffer.header.shost = this->address(); // though not important in same vm comms
        block->frame_buffer.header.dhost = Ethernet::MAC(dst_mac); // todo: revise if we should send it like this
        block->frame_buffer.header.type = htons(protocol); // reminder that it is engine's responsibility to do htons() on sends (receive is NIC's)
        block->frame_buffer.data_length = size;
        memcpy(block->frame_buffer.data, data, size);

        int written = sizeof(Ethernet::Header) + size;
        
        // END OF PROTECTED ZONE

        struct sembuf release_full = {FULL_SEM, +1, SEM_UNDO};

        if (semop(_sem_id, &release_full, 1) == -1) {
            perror("semop (signal on FULL_SEM) failed in send");
            return -1;
    }
        
        return written;
    }

    /**
     * @brief Receives a frame from another process via shared memory.
     * This is a blocking call, intended to be run in the NIC's receiver thread.
     * @return Number of bytes read, or -1 on error.
     */
    int receive(void* frame_buffer, int max_size) {

        struct sembuf acquire_full = {FULL_SEM, -1, SEM_UNDO};

        if (semop(_sem_id, &acquire_full, 1) == -1) {
            perror("semop (WAIT ON FULL SEM) failed on receive");
            return -1;
        }

        // PROTECTED ZONE

        SharedBlock* block = static_cast<SharedBlock*>(_shared_block);

        Ethernet::Frame* frame = &block->frame_buffer;

        int data_size = frame->data_length;
        int total_size = data_size + sizeof(frame->header);
        int min_copied = std::min(total_size, max_size);

        memcpy(frame_buffer, frame, min_copied);
        
        // END OF PROTECTED ZONE

        struct sembuf release_empty = {EMPTY_SEM, +1, SEM_UNDO};

        if (semop(_sem_id, &release_empty, 1) == -1) {
            perror("semop (RELEASE ON EMPTY SEM) failed on receive");
            return -1;
        }

        return min_copied;
    }

    union semun {
            int val;                // Value for SETVAL
            struct semid_ds *buf;   // Buffer for IPC_STAT, IPC_SET
            unsigned short *array;  // Array for GETALL, SETALL
        };

    enum { EMPTY_SEM = 0, FULL_SEM = 1 };

private:

    void _cleanupAndThrow(const std::string& errorMessage) {
        std::cerr << "CRITICAL ERROR during ShmEngine construction: " << errorMessage << " - " << strerror(errno) << std::endl;
        std::cerr << "Performing cleanup..." << std::endl;

        // cleanup in reverse order of acquisition

        // Detach shared memory if it was attached
        if (_shared_block != nullptr && _shared_block != (void*)-1) {
            shmdt(_shared_block);
        }

        // Remove shared memory segment if it was created
        if (_shared_id != -1) {
            shmctl(_shared_id, IPC_RMID, nullptr);
        }

        // Remove semaphore set if we were the one who created it (no race conditions?).
        if (_sem_id != -1) {
            semctl(_sem_id, 0, IPC_RMID, nullptr);
        }

        throw std::runtime_error(errorMessage);
    }

    struct SharedBlock {
        // An atomic (todo: better atomicity?) reference counter to track connected processes.
        volatile int client_count;

        Ethernet::Frame frame_buffer;
    };

    // Attached memory segment
    SharedBlock* _shared_block = nullptr;

    // IDs for the shared memory segment and semaphore set.
    int _shared_id = -1;
    int _sem_id = -1;

    // not static to avoid some defining issues
    const char* SHM_KEY_FILE = "./shm_key_file"; // our code handles this file not existing
    const int PROJ_ID = 100;
    const int NUMBER_OF_SEMS = 2;
    
    // dummy MAC address for local IPC.
    inline static Ethernet::MAC _dummy_mac = Ethernet::MAC(DUMMY_MAC_BYTES);
};

#endif // SHM_ENGINE_HPP