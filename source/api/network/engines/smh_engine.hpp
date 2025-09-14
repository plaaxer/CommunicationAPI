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
#include <stdexcept>
#include <atomic>

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

// --- Configuration for the Circular Buffer ---
const int MAX_CLIENTS = 16;     // Max number of concurrent processes
const int BUFFER_SLOTS = 128;   // Number of messages that can be buffered

/**
 * @class ShmEngine
 * @brief An Engine for intra-VM communication using System V Shared Memory.
 * This implementation uses a circular buffer to support multiple writers and multiple readers.
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
            _cleanupAndThrow("Failed to create IPC key file");
        }

        // generating the key for shared memory access. See [1]
        key_t shared_key = ftok(SHM_KEY_FILE, PROJ_ID);
        if (shared_key == -1) {
            _cleanupAndThrow("Failed to generate IPC key");
        }

        // allocating or getting the shared memory segment. See [2]
        _shared_id = shmget(shared_key, sizeof(SharedBlock), 0666 | IPC_CREAT);
        if (_shared_id == -1) {
            _cleanupAndThrow("Failed to create/get shared memory segment");
        }

        // attaching the shared memory segment to this process's address space. See [3]
        void* shared_pointer = shmat(_shared_id, nullptr, 0);
        if (shared_pointer == (void*)-1) {
            _cleanupAndThrow("Failed to attach shared memory segment");
        }
        _shared_block = static_cast<SharedBlock*>(shared_pointer);

        // creating or getting the semaphore set. See [4]
        _sem_id = semget(shared_key, 2, IPC_CREAT | IPC_EXCL | 0666); // 2 semaphores for mutexes

        if (_sem_id != -1) {
            // This block runs only for the VERY FIRST process that creates the IPC set.
            std::cout << "Semaphore set created. Initializing shared memory..." << std::endl;
            union semun arg;

            // Initialize the shared block state
            _shared_block->claim_sequence_id = 1;
            _shared_block->publish_sequence_id = 0;

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                _shared_block->client_registry[i].is_active = false;
                _shared_block->client_registry[i].last_read_sequence_id = 0;
            }

            // Initialize the REGISTRY_SEM to 1 (acting as a mutex). See [5]
            arg.val = 1;
            if (semctl(_sem_id, REGISTRY_SEM, SETVAL, arg) == -1) {
                _cleanupAndThrow("semctl (REGISTRY_SEM) initialization failed");
            }

            // Initialize the CLAIM_SEM to 1 (acting as a mutex).
            arg.val = 1;
            if (semctl(_sem_id, CLAIM_SEM, SETVAL, arg) == -1) {
                _cleanupAndThrow("semctl (CLAIM_SEM) initialization failed");
            }

        } else if (errno == EEXIST) {
            // See [4]
            _sem_id = semget(shared_key, 2, 0666);
            if (_sem_id == -1) {
                _cleanupAndThrow("Failed to get existing semaphore set");
            }
        } else {
            _cleanupAndThrow("Failed to create/get semaphore set");
        }

        // --- Register this process in the client registry ---
        _register_client();

        std::cout << "ShmEngine initialized. Client registered in slot " << _client_slot_index << "." << std::endl;
    }

    /**
     * @brief Destructor for the Shared Memory Engine.
     */
    ~ShmEngine() {
        if (!_shared_block) {
            std::cout << "Warning: destructor of ShmEngine called with no attached shared memory";
            return;
        }

        // --- Deregister this client ---
        _deregister_client();
        std::cout << "Client deregistered from slot " << _client_slot_index << "." << std::endl;

        // --- Check if we are the last client ---
        bool is_last_client = true;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (_shared_block->client_registry[i].is_active) {
                is_last_client = false;
                break;
            }
        }
        
        // --- Detach the shared memory segment ---
        if (shmdt(_shared_block) == -1) {
            std::cerr << "ShmEngine warning: shmdt failed: " << strerror(errno) << std::endl;
        }

        // --- If we are the last process, remove IPC objects from the system ---
        if (is_last_client) {
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
     */
    const Ethernet::MAC& address() {
        return _dummy_mac;
    }

    /**
     * @brief Sends a frame to other processes via shared memory.
     * @return Number of bytes written, or -1 on error.
     */
    int send(const unsigned char* dst_mac, unsigned short protocol, const void* data, unsigned int size) {
        // 1. Atomically claim a sequence ID for our message
        struct sembuf acquire_claim = {CLAIM_SEM, -1, SEM_UNDO};
        if (semop(_sem_id, &acquire_claim, 1) == -1) {
            perror("semop (acquire CLAIM_SEM) failed in send");
            return -1;
        }
        uint64_t my_ticket_id = _shared_block->claim_sequence_id++;
        struct sembuf release_claim = {CLAIM_SEM, +1, SEM_UNDO};
        if (semop(_sem_id, &release_claim, 1) == -1) {
            perror("semop (release CLAIM_SEM) failed in send");
            return -1;
        }

        // 2. Calculate the slot and wait until it's free from the slowest reader
        uint64_t slot_index = my_ticket_id % BUFFER_SLOTS;
        
        // This is a spin-wait. In a real-time system, a more advanced semaphore or
        // condition variable might be used, but this is simple and correct.
        while (true) {
            uint64_t slowest_reader_id = my_ticket_id; // Assume no readers are behind
            bool readers_active = false;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (_shared_block->client_registry[i].is_active) {
                    readers_active = true;
                    uint64_t last_read = _shared_block->client_registry[i].last_read_sequence_id;
                    slowest_reader_id = std::min(slowest_reader_id, last_read);
                }
            }

            if (!readers_active || (my_ticket_id - slowest_reader_id < BUFFER_SLOTS)) {
                break; // Slot is free to be written
            }
            std::cout << "[WARNING] Writer stuck on slow readers" << std::endl;
            usleep(100); // Wait for slow readers to catch up
        }

        // 3. Write data to the buffer
        MessageSlot* slot = &_shared_block->buffer[slot_index];
        slot->frame.header.shost = this->address();
        slot->frame.header.dhost = Ethernet::MAC(dst_mac);
        slot->frame.header.type = htons(protocol);
        slot->frame.data_length = size;
        memcpy(slot->frame.data, data, size);

        std::cout << "[SEND] Wrote data. Setting slot->sequence_id to " << my_ticket_id << std::endl; // <-- ADD

        // 4. Publish the message by setting its sequence ID. This must be the LAST step of writing.
        // We use std::atomic_thread_fence to ensure all writes above are visible before this one.
        std::atomic_thread_fence(std::memory_order_release);
        slot->sequence_id = my_ticket_id;

        // 5. Update the global publish counter, but only if it's our turn.
        // This ensures readers see a contiguous stream of messages.
        uint64_t current_publish_id = my_ticket_id - 1;
        // This is another spin-wait, ensuring strict ordering of publication.
        while(!_shared_block->publish_sequence_id.compare_exchange_weak(current_publish_id, my_ticket_id)) {
            current_publish_id = my_ticket_id - 1;
            usleep(100);
        }

        return sizeof(Ethernet::Header) + size;
    }

    /**
     * @brief Receives a frame from another process via shared memory.
     * @return Number of bytes read, or -1 on error.
     */
    int receive(void* frame_buffer, int max_size) {
        // 1. Identify the next message we need to read
        uint64_t last_read = _shared_block->client_registry[_client_slot_index].last_read_sequence_id; // <-- ADD
        uint64_t target_seq_id = last_read + 1;

        std::cout << "[RECV] My last read ID was " << last_read << ". Waiting for target ID: " << target_seq_id << std::endl;

        // 2. Wait until that message has been published by a writer
        // This is a spin-wait. It's simple and avoids complex semaphore signaling.
        while (_shared_block->publish_sequence_id < target_seq_id) {
            usleep(200);
        }

        std::cout << "We are inside the while at RECEIVE" << std::endl;

        // 3. Calculate the slot and read the data
        uint64_t slot_index = target_seq_id % BUFFER_SLOTS;
        MessageSlot* slot = &_shared_block->buffer[slot_index];

        // Ensure the writer has finished placing the sequence ID (memory barrier)
        std::atomic_thread_fence(std::memory_order_acquire);

        // We can double-check the sequence ID to be absolutely sure, though the publish counter is the primary guard.
        if (slot->sequence_id != target_seq_id) {
            // This would indicate a serious logic error, e.g., memory corruption.
            std::cerr << "CRITICAL: Mismatch between publish counter and slot sequence ID!" << std::endl;
            return -1;
        }

        int total_size = sizeof(slot->frame.header) + slot->frame.data_length;
        int bytes_to_copy = std::min(total_size, max_size);
        memcpy(frame_buffer, &slot->frame, bytes_to_copy);

        // 4. Acknowledge the read by updating our own state in the registry
        _shared_block->client_registry[_client_slot_index].last_read_sequence_id = target_seq_id;

        return bytes_to_copy;
    }

    std::string name() {
        return "SharedMemoryEngine";
    }

private:
    // --- Data Structures for Shared Memory ---

    struct MessageSlot {
        volatile uint64_t sequence_id;
        Ethernet::Frame   frame;
    };

    struct ClientState {
        volatile bool     is_active;
        volatile pid_t    process_id;
        volatile uint64_t last_read_sequence_id;
    };

    struct SharedBlock {
        // Writer coordination
        volatile uint64_t claim_sequence_id;
        std::atomic<uint64_t> publish_sequence_id; // Atomic for safe CAS operations

        // Reader & Data
        ClientState client_registry[MAX_CLIENTS];
        MessageSlot buffer[BUFFER_SLOTS];
    };

    // --- Private Helper Methods ---

    void _register_client() {
        struct sembuf op = {REGISTRY_SEM, -1, SEM_UNDO};
        if (semop(_sem_id, &op, 1) == -1) _cleanupAndThrow("Failed to lock registry for registration");

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (!_shared_block->client_registry[i].is_active) {
                _client_slot_index = i;
                auto& client = _shared_block->client_registry[i];
                client.is_active = true;
                client.process_id = getpid();
                client.last_read_sequence_id = _shared_block->publish_sequence_id.load();
                
                op.sem_op = 1; // Release lock
                if (semop(_sem_id, &op, 1) == -1) _cleanupAndThrow("Failed to unlock registry after registration");
                return;
            }
        }
        
        op.sem_op = 1; // Release lock before throwing
        semop(_sem_id, &op, 1);
        _cleanupAndThrow("Failed to register client: No available slots");
    }

    void _deregister_client() {
        if (_client_slot_index != -1) {
            struct sembuf op = {REGISTRY_SEM, -1, SEM_UNDO};
            if (semop(_sem_id, &op, 1) == -1) {
                perror("Failed to lock registry for deregistration");
                return; // Can't throw in destructor
            }

            _shared_block->client_registry[_client_slot_index].is_active = false;
            
            op.sem_op = 1; // Release lock
            semop(_sem_id, &op, 1);
        }
    }

    void _cleanupAndThrow(const std::string& errorMessage) {
        std::cerr << "CRITICAL ERROR during ShmEngine construction: " << errorMessage << " - " << strerror(errno) << std::endl;
        std::cerr << "Performing cleanup..." << std::endl;

        if (_shared_block != nullptr && _shared_block != (void*)-1) {
            shmdt(_shared_block);
        }
        if (_shared_id != -1) {
            shmctl(_shared_id, IPC_RMID, nullptr);
        }
        if (_sem_id != -1) {
            semctl(_sem_id, 0, IPC_RMID, nullptr);
        }
        throw std::runtime_error(errorMessage);
    }
    
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    };

    enum { REGISTRY_SEM = 0, CLAIM_SEM = 1 };

    // Attached memory segment
    SharedBlock* _shared_block = nullptr;

    // This process's slot in the client registry
    int _client_slot_index = -1;

    // IDs for the shared memory segment and semaphore set.
    int _shared_id = -1;
    int _sem_id = -1;

    // not static to avoid some defining issues
    const char* SHM_KEY_FILE = "./shm_key_file";
    const int PROJ_ID = 100;

    // dummy MAC address for local IPC.
    inline static Ethernet::MAC _dummy_mac = Ethernet::MAC(DUMMY_MAC_BYTES);
};

#endif // SHM_ENGINE_HPP