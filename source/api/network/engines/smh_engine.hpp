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
#include <vector>
#include <string>

/**
 * Most of them are easily accessible through the overall link.
 * overall: https://man7.org/linux/man-pages/man7/svipc.7.html
 * [1] ftok: https://man7.org/linux/man-pages/man3/ftok.3.html
 * [2] shmget: https://man7.org/linux/man-pages/man2/shmget.2.html
 * [3] shmat: https://man7.org/linux/man-pages/man2/shmat.2.html
 * [4] semget: https://man7.org/linux/man-pages/man2/semget.2.html
 * [5] semctl: https://man7.org/linux/man-pages/man2/semctl.2.html
 * [6] semop: https://man7.org/linux/man-pages/man2/semop.2.html
 * [7] shmctl: https://www.man7.org/linux/man-pages/man2/shmctl.2.html
 *
 * Other references:
 * https://hildstrom.com/projects/2015/08/ipc_sysv_posix/index.html
 * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_ipc.h.html
 */

static const uint8_t DUMMY_MAC_BYTES[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

const int MAX_CLIENTS = 16;     // Max number of concurrent processes
const int BUFFER_SLOTS = 32;   // Number of messages that can be buffered
const int BASE_PORT = 1000;     // Base for dynamic port assignment
const int CONTROL_SEMS = 4; // REGISTRY, CLAIM, DIRECTORY, and WRITER_WAIT (new)

/**
 * @class ShmEngine
 * @brief An Engine for intra-VM communication using System V Shared Memory.
 * This implementation uses a circular buffer to support multiple writers and multiple readers.
 */
class ShmEngine {

private:

    struct MessageSlot {
        volatile uint64_t sequence_id;
        Ethernet::Frame   frame;
    };    

public:
    /**
     * @brief Constructor for the Shared Memory Engine.
     */
    ShmEngine() {

        // Obtain the id of the shm and the set of semaphores from the environment variables
        // These environment variables were created by the parent process in start_carwhen using ShmEngine's bootstrapIPC static function
        _shared_id = std::atoi(getenv("SHARED_ID"));
        _sem_id = std::atoi(getenv("SEM_ID"));

        // Attach the shared memory segment to this process's address space. See [3]
        void* shared_pointer = shmat(_shared_id, nullptr, 0);
        if (shared_pointer == (void*)-1) {
            _cleanupAndThrow("Failed to attach shared memory segment");
        }

        // Creates pointer to a SharedBlock object from shared_pointer
        _shared_block = static_cast<SharedBlock*>(shared_pointer);

        // "Subscribes" the process
        _register_client();

        // std::cout << "ShmEngine initialized. Client registered in slot " << _client_slot_index << "." << std::endl;
    }

    /**
     * @brief Destructor for the Shared Memory Engine.
     */
    ~ShmEngine() {
        if (!_shared_block) {
            std::cout << "Warning: destructor of ShmEngine called with no attached shared memory";
            return;
        }

        _deregister_client();
        // std::cout << "Client deregistered from slot " << _client_slot_index << "." << std::endl;

        bool is_last_client = true;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (_shared_block->client_registry[i].is_active) {
                is_last_client = false;
                break;
            }
        }

        if (shmdt(_shared_block) == -1) {
            std::cerr << "ShmEngine warning: shmdt failed: " << strerror(errno) << std::endl;
        }

        // removing ipc objects from the system
        if (is_last_client) {
            std::cout << "Last client detached. Removing IPC resources..." << std::endl;

            // shared memory segment
            if (shmctl(_shared_id, IPC_RMID, nullptr) == -1) {
                std::cerr << "ShmEngine warning: shmctl(IPC_RMID) failed: " << strerror(errno) << std::endl;
            } else {
                std::cout << "Shared memory segment removed successfully." << std::endl;
            }

            // semaphore set
            if (semctl(_sem_id, 0, IPC_RMID) == -1) {
                std::cerr << "ShmEngine warning: semctl(IPC_RMID) failed: " << strerror(errno) << std::endl;
            } else {
                std::cout << "Semaphore set removed successfully." << std::endl;
            }
        }

        std::cout << "ShmEngine destroyed." << std::endl;
    }

    /**
     * @brief Static function to initiate the shared memory and semaphores in parent process. 
     * @details 
     * Called by start_car before the first fork() is called.
     * Guarantees that all child processes (gateway + components) have access to the shared memory.
     * When each child process is constructed, they will then be able to access the shared memory and semaphore IDs via environment variables.
     * With these IDs, the child processes can then attach themselves to the shared memory
     */
    static bool bootstrapIPC() {
        int shared_id = -1;
        int sem_id = -1;
        SharedBlock* shared_block = nullptr;
        bool success = false;

        // Use a do-while(false) loop to manage initialization flow.
        // 'break' will act as a 'goto' to the cleanup section.
        do {
            // 1. Create shared memory
            shared_id = shmget(IPC_PRIVATE, sizeof(SharedBlock), 0666 | IPC_CREAT | IPC_EXCL);
            if (shared_id == -1) {
                perror("IPC Bootstrap: failed to create/get shared memory segment");
                break;
            }

            // 2. Attach the shared memory segment
            void* shared_pointer = shmat(shared_id, nullptr, 0);
            if (shared_pointer == (void *)-1) {
                perror("IPC Bootstrap: failed to attach memory segment");
                break;
            }
            shared_block = static_cast<SharedBlock*>(shared_pointer);

            // 3. Initialize the shared_block's structure
            shared_block->directory.next_port_to_assign = BASE_PORT;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                shared_block->directory.entries[i].is_active = false;
            }
            shared_block->claim_sequence_id = 1;
            shared_block->publish_sequence_id = 0;
            shared_block->writer_is_blocked = false;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                shared_block->client_registry[i].is_active = false;
                shared_block->client_registry[i].last_read_sequence_id.store(0);
            }

            // 4. Create shared semaphore set
            const int TOTAL_SEMS = CONTROL_SEMS + MAX_CLIENTS;
            sem_id = semget(IPC_PRIVATE, TOTAL_SEMS, 0666| IPC_CREAT | IPC_EXCL);
            if (sem_id == -1) {
                perror("IPC Bootstrap: IPC_PRIVATE semaphore set creation failed");
                break;
            }

            // 5. Initialize semaphores
            union semun arg;

            // REGISTRY_SEM
            arg.val = 1;
            if (semctl(sem_id, REGISTRY_SEM, SETVAL, arg) == -1) {
                perror("IPC Bootstrap: semctl (REGISTRY_SEM) initialization failed");
                break;
            }

            // CLAIM_SEM
            arg.val = 1;
            if (semctl(sem_id, CLAIM_SEM, SETVAL, arg) == -1) {
                perror("IPC Bootstrap: semctl (CLAIM_SEM) initializatoin failed");
                break;
            }

            // DIRECTORY_SEM
            arg.val = 1;
            if (semctl(sem_id, DIRECTORY_SEM, SETVAL, arg) == -1) {
                perror("IPC Bootstrap: semctl (DIRECTORY_SEM) initializatoin failed");
                break;
            }

            // WRITER_WAIT_SEM
            arg.val = 0;
            if (semctl(sem_id, WRITER_WAIT_SEM, SETVAL, arg) == -1) {
                perror("IPC Bootstrap: semctl (WRITER_WAIT_SEM) initializatoin failed");
                break;
            }
            
            // Clients' consumer semaphores
            arg.val = 0;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (semctl(sem_id, CONTROL_SEMS + i, SETVAL, arg) == -1) {
                    perror("IPC Bootstrap: semctl (CLIENT_SEM) initialization failed");
                    break; // Exit the outer do-while loop on failure
                }
            }
            
            // If we reach this point, all initializations were successful.
            success = true;

        } while (false); // Loop only executes once.

        if (!success) {
            // Detach if attached
            if (shared_block != nullptr) {
                shmdt(shared_block);
            }
            // Remove shm if created
            if (shared_id != -1) {
                shmctl(shared_id, IPC_RMID, nullptr);
            }
            // Remove sem if created
            if (sem_id != -1) {
                semctl(sem_id, 0, IPC_RMID);
            }
            return false;
        }

        // On success, detach the parent process from the shared memory
        if (shmdt(shared_block) == -1) {
            perror("IPC Bootstrap: detaching parent initializer process from shared_block failed");
        }

        // Save the shm_id and sem_id in the environment for child processes
        char shared_id_buf[32], sem_id_buf[32];
        std::snprintf(shared_id_buf, sizeof(shared_id_buf), "%d", shared_id);
        std::snprintf(sem_id_buf, sizeof(sem_id_buf), "%d", sem_id);
        setenv("SHARED_ID", shared_id_buf, 1);
        setenv("SEM_ID", sem_id_buf, 1);

        std::cout << "IPC bootstrap complete. shared_id=" << shared_id << ", sem_id=" << sem_id << std::endl;
        return true;
    }

    /**
     * @brief Returns a dummy MAC address for local communication.
     */
    const Ethernet::MAC& address() {
        return _dummy_mac;
    }

    /**
     * @brief Sends a frame to other processes via shared memory.
     * @details
     * The writer must first claim a sequence ID; that is protected by a semaphor to prevent
     * conflicts with multiple writers. Then it must check whether the slowest reader - that
     * is, the one that has read the least messages - is BUFFER_SLOTS slots behind himself.
     * If he is, then the writer must wait for him to read the remaining messages otherwise
     * he could end up overwriting data.
     * @return Number of bytes written, or -1 on error.
     */
    int send(const unsigned char* src_mac, const unsigned char* dst_mac, unsigned short protocol, const void* data, unsigned int size) {
        // 1. Atomically claim a sequence ID for our message
        struct sembuf acquire_claim = {CLAIM_SEM, -1, SEM_UNDO};

        // std::cout << "[DEBUG PID:" << getpid() << "] send: Attempting to claim a ticket..." << std::endl;
        if (semop(_sem_id, &acquire_claim, 1) == -1) {
            perror("semop (acquire CLAIM_SEM) failed in send");
            return -1;
        }
        // std::cout << "[DEBUG PID:" << getpid() << "] send: Claimed ticket." << std::endl;

        uint64_t my_ticket_id = _shared_block->claim_sequence_id++;
        struct sembuf release_claim = {CLAIM_SEM, +1, SEM_UNDO};
        if (semop(_sem_id, &release_claim, 1) == -1) {
            perror("semop (release CLAIM_SEM) failed in send");
            return -1;
        }
        
        // 2. Wait until the slot is free from the slowest reader (not more spin-wait)
        while (true) {
            // 2.1 Verify who is the slowest reader
            uint64_t slowest_reader_id = my_ticket_id;
            bool readers_active = false;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (_shared_block->client_registry[i].is_active) {
                    readers_active = true;
                    uint64_t last_read = _shared_block->client_registry[i].last_read_sequence_id.load(std::memory_order_acquire);
                    slowest_reader_id = std::min(slowest_reader_id, last_read);
                }
            }
            
            if (!readers_active || (my_ticket_id - slowest_reader_id < BUFFER_SLOTS)) {
                _shared_block->writer_is_blocked = false;
                break;
            }

            // Debug
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (_shared_block->client_registry[i].is_active &&
                    _shared_block->client_registry[i].last_read_sequence_id.load(std::memory_order_acquire) == slowest_reader_id) {
                    std::cout << "[INFO] Slowest reader slot: " << i
                                << ", PID: " << _shared_block->client_registry[i].process_id;

                    // Check if PID is registered in the directory and its device name is "Gateway"
                    bool is_gateway = false;
                    for (int j = 0; j < MAX_CLIENTS; ++j) {
                        if (_shared_block->directory.entries[j].is_active &&
                            _shared_block->directory.entries[j].owner_pid == _shared_block->client_registry[i].process_id &&
                            strcmp(_shared_block->directory.entries[j].component_name, "Gateway") == 0) {
                            is_gateway = true;
                            break;
                        }
                    }
                    std::cout << (is_gateway ? " [Gateway]" : " [Not Gateway]");  // not reliable
                    std::cout << std::endl;
                }
            }
            
            _shared_block->writer_is_blocked = true;
            struct sembuf wait_op = {WRITER_WAIT_SEM, -1, SEM_UNDO};

            std::cout << "[DEBUG PID:" << getpid() << "] send: Writer is blocked. My ticket: " 
                         << my_ticket_id << ", Slowest reader at: " << slowest_reader_id << ". Going to sleep..." << std::endl;

            if (semop(_sem_id, &wait_op, 1) == -1) {
                if (errno == EINTR) continue;
                perror("semop (WRITER_WAIT_SEM) failed in send");
                _shared_block->writer_is_blocked = false;
                return -1;
            }

            // std::cout << "[DEBUG PID:" << getpid() << "] send: Writer woke up. Re-checking condition..." << std::endl;
        }
        
        // 3. Write data to the buffer
        uint64_t slot_index = my_ticket_id % BUFFER_SLOTS;
        MessageSlot* slot = &_shared_block->buffer[slot_index];
        slot->frame.header.shost = src_mac ? Ethernet::MAC(src_mac) : _dummy_mac;
        slot->frame.header.dhost = Ethernet::MAC(dst_mac);
        slot->frame.header.type = htons(protocol);
        slot->frame.data_length = size;
        memcpy(slot->frame.data, data, size);

        // 4. Publish the message by setting its sequence ID. This must be the LAST step of writing.
        // we use std::atomic_thread_fence to ensure all writes above are visible before this one.
        std::atomic_thread_fence(std::memory_order_release);
        slot->sequence_id = my_ticket_id;

        // 5. Update the global publish counter, but only if it's our turn.
        // This ensures readers see a contiguous stream of messages.
        uint64_t current_publish_id = my_ticket_id - 1;
        while(!_shared_block->publish_sequence_id.compare_exchange_weak(current_publish_id, my_ticket_id)) {
            current_publish_id = my_ticket_id - 1;
        }

        // 6. Notify all active clients to consume
        std::vector<sembuf> signal_ops;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (_shared_block->client_registry[i].is_active) {
                sembuf op;
                op.sem_num = CONTROL_SEMS + i;
                op.sem_op  = +1;
                op.sem_flg = SEM_UNDO;
                signal_ops.push_back(op);
            }
        }
        if (!signal_ops.empty()) {
            //std::cout << "[DEBUG PID:" << getpid() << "] send: Notifying " << signal_ops.size() << " clients of message " << my_ticket_id << "." << std::endl;

            if (semop(_sem_id, signal_ops.data(), signal_ops.size()) == -1) {
                perror("batched semop failed in send");
            }
        }

        return sizeof(Ethernet::Header) + size;
    }


    /**
     * @brief Receives a frame from another process via shared memory.
     * @details 
     * Firstly, a new message needs to be published by a writer; we wait until
     * that happens (i.e, there's a publish id for our target id). Then, we calculate
     * the referenced block for that id and extract its content, finally updating
     * our own read messages counter.
     * @return Number of bytes read, or -1 on error.
     */
    int receive(void* frame_buffer, int max_size) {
        // 1. Identify the next message we need to read
        uint64_t last_read = _shared_block->client_registry[_client_slot_index].last_read_sequence_id.load();
        uint64_t target_seq_id = last_read + 1;

        // 2. Verify if this reader is the slowest, to later writer release
        bool i_am_the_slowest = false;
        if (_shared_block->writer_is_blocked) {
            uint64_t slowest_reader_id = last_read;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (_shared_block->client_registry[i].is_active) {
                    uint64_t current_client_last_read = _shared_block->client_registry[i].last_read_sequence_id.load();
                    slowest_reader_id = std::min(slowest_reader_id, current_client_last_read);
                }
            }
            if (last_read == slowest_reader_id) {
                i_am_the_slowest = true;
                std::cout << "[DEBUG PID:" << getpid() << "] receive: I am the slowest reader at ticket " << last_read << "." << std::endl;
            }
        }

        unsigned short my_sem_index = CONTROL_SEMS + _client_slot_index;
        struct sembuf wait_op = {my_sem_index, -1, SEM_UNDO};


        // std::cout << "[DEBUG PID:" << getpid() << "] receive: Waiting for message " << target_seq_id << "..." << std::endl;
        if (semop(_sem_id, &wait_op, 1) == -1) {
            if (errno == EINTR) return 0;
            perror("semop (wait client) failed in receive");
            return -1;
        }
        // std::cout << "[DEBUG PID:" << getpid() << "] receive: Woke up for message " << target_seq_id << "." << std::endl;

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
        _shared_block->client_registry[_client_slot_index].last_read_sequence_id.store(target_seq_id, std::memory_order_release);


        // 5. Release the writer waiting the slowest
        if (i_am_the_slowest) {
            struct sembuf signal_op = {WRITER_WAIT_SEM, +1, SEM_UNDO};
            std::cout << "[DEBUG PID:" << getpid() << "] receive: I was the slowest, waking up the writer." << std::endl;
            if (semop(_sem_id, &signal_op, 1) == -1) {
                perror("semop (signal WRITER_WAIT_SEM) failed in receive");
            }
        }

        return bytes_to_copy;
    }

    /**
     * @brief Zero-copy receive. Waits for a new message and returns a direct, non-owning
     * pointer to the MessageSlot in shared memory.
     * @details We use a private target_seq_id to track which message we need to read next.
     * This is incremented only after a successful read, ensuring we don't skip messages.
     * @return A pointer to the MessageSlot, or nullptr on error/interruption.
     */
    MessageSlot* receive_zerocopy(uint64_t target_seq_id) {

        // Wait for the writer to signal that a new message is available.
        unsigned short my_sem_index = CONTROL_SEMS + _client_slot_index;
        struct sembuf wait_op = {my_sem_index, -1, SEM_UNDO};

        if (semop(_sem_id, &wait_op, 1) == -1) {
            if (errno == EINTR) return nullptr; // Interrupted, not an error.
            perror("semop (wait client) failed in receive_zerocopy");
            return nullptr;
        }

        // The message is ready. Calculate the slot and return a pointer to it.
        // The data is not copied. The caller is responsible for calling release_frame later.
        uint64_t slot_index = target_seq_id % BUFFER_SLOTS;
        MessageSlot* slot = &_shared_block->buffer[slot_index];

        // Use a memory fence to ensure we see all writes from the sender.
        std::atomic_thread_fence(std::memory_order_acquire);

        if (slot->sequence_id != target_seq_id) {
            std::cerr << "CRITICAL: Mismatch between expected and actual sequence ID in zero-copy receive!" << std::endl;
            // invalid state.
            return nullptr;
        }
        
        return slot;
    }

    /**
     * @brief Releases a frame that was previously acquired via receive_zerocopy.
     * This signals that the reader is done with the data.
     * @param sequence_id The sequence ID of the frame being released.
     */
    void release_frame(uint64_t sequence_id) {

        // determine if we are the slowest reader, which might be blocking a writer.
        bool i_am_the_slowest = false;
        if (_shared_block->writer_is_blocked) {
            uint64_t my_last_read = _shared_block->client_registry[_client_slot_index].last_read_sequence_id.load();
            uint64_t slowest_reader_id = my_last_read + 1; // Assume we are not the slowest initially

            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (i != _client_slot_index && _shared_block->client_registry[i].is_active) {
                    uint64_t current_client_last_read = _shared_block->client_registry[i].last_read_sequence_id.load();
                    slowest_reader_id = std::min(slowest_reader_id, current_client_last_read);
                }
            }
            // If our last read ID is less than or equal to all others, we are the one holding things up.
            if (my_last_read <= slowest_reader_id) {
                i_am_the_slowest = true;
            }
        }

        // officialy acknowledging the read by updating our state. this allows other writers to proceed.
        _shared_block->client_registry[_client_slot_index].last_read_sequence_id.store(sequence_id);

        // if we were the slowest reader and a writer is waiting, wake it up.
        if (i_am_the_slowest) {
            struct sembuf signal_op = {WRITER_WAIT_SEM, +1, SEM_UNDO};
            if (semop(_sem_id, &signal_op, 1) == -1) {
                perror("semop (signal WRITER_WAIT_SEM) failed in release_frame");
            }
        }
    }

    /**
     * @brief Registers a component's service and gets a port assigned.
     * @return The assigned port number, or 0 on failure.
     */
    uint16_t registerService(const std::string& name, uint32_t type_id) 
    {
        // Lock the directory for exclusive access
        struct sembuf op = {DIRECTORY_SEM, -1, SEM_UNDO};
        // std::cout << "[DEBUG PID:" << getpid() << "] registerService: Locking directory..." << std::endl;
        if (semop(_sem_id, &op, 1) == -1) {
            perror("Failed to lock directory for registration");
            return 0; // Failure
        }

        // std::cout << "[DEBUG PID:" << getpid() << "] registerService: Directory locked." << std::endl;

        // std::cout << "Registering service of name " << name << " with type " << type_id << std::endl;

        // Find an empty slot in the directory
        for (int i = 0; i < MAX_CLIENTS; ++i) {

            if (!_shared_block->directory.entries[i].is_active) {

                auto& entry = _shared_block->directory.entries[i];
                entry.is_active = true;
                strncpy(entry.component_name, name.c_str(), sizeof(entry.component_name) - 1);
                entry.component_name[sizeof(entry.component_name) - 1] = '\0';  // null 
                entry.component_type_id = type_id;
                entry.owner_pid = getpid();

                // Assign a new, unique port
                entry.assigned_port = _shared_block->directory.next_port_to_assign++;

                // Unlock the directory
                op.sem_op = 1;
                semop(_sem_id, &op, 1);

                // std::cout << "Process " << getpid() << " has been assigned to port  " << entry.assigned_port << std::endl;

                return entry.assigned_port;
            }
        }

        // No slot found, unlock and return failure
        op.sem_op = 1;
        semop(_sem_id, &op, 1);
        return 0;
    }

    /**
     * @brief Looks up the port for a given service name.
     * @return The port number, or 0 if not found.
     */
    uint16_t lookupService(const std::string& name) {

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (_shared_block->directory.entries[i].is_active && 
                strcmp(_shared_block->directory.entries[i].component_name, name.c_str()) == 0) {
                return _shared_block->directory.entries[i].assigned_port;
            }
        }
        return 0;
    }

    /**
     * @brief Looks up the port for the first service matching a given type ID.
     * @param type_id The numeric type ID to search for.
     * @return The port number
     */
    uint16_t lookupServiceByType(uint32_t type_id) {
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (_shared_block->directory.entries[i].is_active && 
                _shared_block->directory.entries[i].component_type_id == type_id) {
                return _shared_block->directory.entries[i].assigned_port;
            }
        }
        return 0;
    }

    

private:

    struct DirectoryEntry {
        volatile bool is_active;
        char component_name[64];
        uint32_t component_type_id;
        uint16_t assigned_port;
        pid_t owner_pid;
    };

    struct ComponentDirectory {
        volatile uint16_t next_port_to_assign;
        DirectoryEntry entries[MAX_CLIENTS];
    };

    struct ClientState {
        volatile bool     is_active;
        volatile pid_t    process_id;
        std::atomic<uint64_t> last_read_sequence_id;
    };

    struct SharedBlock {
        ComponentDirectory directory;  // The service directory

        // Writer coordination
        volatile uint64_t claim_sequence_id;
        std::atomic<uint64_t> publish_sequence_id; // Atomic for safe CAS operations

        // Reader & Data
        ClientState client_registry[MAX_CLIENTS];
        MessageSlot buffer[BUFFER_SLOTS];

        // Writer control
        volatile bool writer_is_blocked;
    };

    /**
     * @brief Register a client (aka process) on the shared memory registry.
     * This action is protected by the REGISTRY_SEM.
     */
    void _register_client() {
        struct sembuf op = {REGISTRY_SEM, -1, SEM_UNDO};

        // std::cout << "[DEBUG PID:" << getpid() << "] _register_client: Locking registry..." << std::endl;

        // acquiring the registry
        if (semop(_sem_id, &op, 1) == -1) _cleanupAndThrow("Failed to lock registry for registration");

        // std::cout << "[DEBUG PID:" << getpid() << "] _register_client: Registry locked." << std::endl;
        
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (!_shared_block->client_registry[i].is_active) {

                // sets this process' client slot
                _client_slot_index = i;

                auto& client = _shared_block->client_registry[i];
                client.is_active = true;
                client.process_id = getpid();
                client.last_read_sequence_id.store(_shared_block->publish_sequence_id.load());

                op.sem_op = 1;
                // releasing the registry
                if (semop(_sem_id, &op, 1) == -1) _cleanupAndThrow("Failed to unlock registry after registration");
                return;
            }
        }

        op.sem_op = 1; // release before throwing to prevent deadlocks
        semop(_sem_id, &op, 1);
        _cleanupAndThrow("Failed to register client: No available slots");
    }

    /**
     * @brief Deregister (removes) a client (watching process).
     */
    void _deregister_client() {
        if (_client_slot_index != -1) {
            struct sembuf op = {REGISTRY_SEM, -1, SEM_UNDO};
            // std::cout << "[DEBUG PID:" << getpid() << "] _deregister_client: Locking registry..." << std::endl;
            if (semop(_sem_id, &op, 1) == -1) {
                perror("Failed to lock registry for deregistration");
                return;
            }

            // std::cout << "[DEBUG PID:" << getpid() << "] _deregister_client: Registry locked." << std::endl;
            _shared_block->client_registry[_client_slot_index].is_active = false;

            op.sem_op = 1;
            semop(_sem_id, &op, 1);
        }
    }
    
    void _cleanupAndThrow(const std::string& errorMessage) {
        std::cerr << "CRITICAL ERROR during ShmEngine construction: " << errorMessage << " - " << strerror(errno) << std::endl;
        std::cerr << "Performing cleanup..." << std::endl;

        if (_shared_block != nullptr && _shared_block != (void*)-1) shmdt(_shared_block);
        if (_shared_id != -1) shmctl(_shared_id, IPC_RMID, nullptr);
        if (_sem_id != -1) semctl(_sem_id, 0, IPC_RMID, nullptr);

        throw std::runtime_error(errorMessage);
    }

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    };

    // semaphores
    enum { REGISTRY_SEM = 0, CLAIM_SEM = 1, DIRECTORY_SEM = 2, WRITER_WAIT_SEM = 3 };

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