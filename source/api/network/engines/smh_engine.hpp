#ifndef SHM_ENGINE_HPP
#define SHM_ENGINE_HPP

#include "../definitions/ethernet.hpp"

// Required headers for System V IPC
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <cerrno>
#include <iostream>
#include <cstring>

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
        // --- TODO: Implement IPC Setup ---

        // 1. Generate a unique key for IPC resources.
        //    - Use ftok() with a predefined file path (e.g., "/tmp/shm_key_file").
        //    - Ensure this file exists.

        // 2. Create/Get the shared memory segment.
        //    - Use shmget() with the generated key.
        //    - Use flags like IPC_CREAT to create it if it doesn't exist.
        //    - Handle potential errors.

        // 3. Attach the shared memory segment to this process's address space.
        //    - Use shmat() with the ID from shmget().
        //    - Store the returned pointer in a private member (e.g., _shared_buffer).
        //    - Handle potential errors.

        // 4. Create/Get the semaphore set.
        //    - Use semget() with the same key. You'll need at least two semaphores
        //      for a basic producer-consumer model (e.g., one for 'full', one for 'empty').
        //    - Use IPC_CREAT.
        //    - Handle potential errors.

        // 5. Initialize the semaphores (only needs to be done once by one process).
        //    - Use semctl() with the SETVAL command.
        //    - e.g., Initialize 'empty' semaphore to 1 (buffer is initially empty).
        //    - e.g., Initialize 'full' semaphore to 0 (buffer is initially not full).

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

private:
    // --- TODO: Add private members for IPC management ---

    // A pointer to the attached shared memory segment.
    // Should be cast to a struct representing the shared data layout.
    void* _shared_buffer = nullptr;

    // IDs for the shared memory segment and semaphore set.
    int _shm_id = -1;
    int _sem_id = -1;
    
    // A static dummy MAC address for local IPC.
    static Ethernet::MAC _dummy_mac;
};

// Initialize the static dummy MAC address.
// Using a value from the "Locally Administered Address Ranges".
//Ethernet::MAC ShmEngine::_dummy_mac = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

#endif // SHM_ENGINE_HPP