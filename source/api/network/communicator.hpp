#ifndef COMMUNICATOR_HPP
#define COMMUNICATOR_HPP

#include "api/observer/observers.hpp"
#include "api/network/definitions/message.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/teds.hpp"
#include "api/network/definitions/segment.hpp"
#include <set>
#include <vector>
#include <bitset>
#include <chrono>
#include <thread>

/**
 * @brief End-Point for the components. It creates a communication channel with the Protocol Handler,
 * an instance of Protocol typenamed Channel here.
 */

template <typename Channel>

class Communicator : public PortObserver, public TypeObserver 
{

public:

    typedef typename Channel::Buffer Buffer;

    Communicator(Channel * channel, Address address)
        : _channel(channel), _address(address)
    {

        _channel->attach_port_listener(this, address.port());
    }

    ~Communicator()
    {
        _channel->detach_port_listener(this, _address.port());

        for (const auto& type : _subscribed_types) {
            _channel->detach_type_listener(this, type);
        }
    }

    bool send(const Message* message) {
        // 1. Create the Segment from the Message's high-level data
        Segment segment(message->get_type(), message->get_payload());

        // 2. Serialize the full Segment into a byte stream
        std::vector<char> serialized_segment = segment.get_bytes();

        // // --- DEBUGGING ---
        // // Print the bits of the final serialized data right before it goes to the network.
        // std::cout << "\n--- SENDING MESSAGE DEBUG ---" << std::endl;
        // std::cout << "Total size: " << serialized_segment.size() << " bytes" << std::endl;
        // print_bits(serialized_segment, "Raw Segment Bytes:");
        // std::cout << "Message type: " << static_cast<int>(message->get_type()) << std::endl;
        // std::cout << "---------------------------" << std::endl;
        // // --- END DEBUGGING ---

        std::cout << "[Communicator] Sending message of type " << to_string(message->get_type()) << std::endl;
        return _channel->send(
            _address,
            message->destination(),
            serialized_segment.data(),
            serialized_segment.size()
        ) > 0;
    }

    /**
     * @brief Blocking receive method. Waits for a message and fills the Message object.
     */
    bool receive(Message * message)
        {
        
        // waiting for the arrival of a message.
        _semaphore.p();
        Buffer* buf = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_data.empty()) return false;
            buf = _data.front();
            _data.pop_front();
        }

        Address from;

        // the only heap copy that happens is here.
        int total_segment_size = _channel->receive(buf, &from, message->data(), message->capacity());
        
        _channel->free(buf);
        
        if (total_segment_size <= 0) {
            return false;
        }

        // // ========================================================================
        // // --- RECEIVE DEBUGGING BLOCK ---
        // // (You can comment out this entire block to disable logging)
        // {
        //     // Use a mutex to prevent interleaved output from other threads.
        //     // Assumes a 'g_cout_mutex' is defined in a shared utility header.
        //     // std::lock_guard<std::mutex> lock(g_cout_mutex);

        //     std::cout << "\n--- RECEIVING MESSAGE DEBUG ---" << std::endl;
        //     std::cout << "From Address: " << from << std::endl;
        //     std::cout << "Total size received: " << total_segment_size << " bytes" << std::endl;

        //     // Create a safe, temporary view of the raw data to print its bits.
        //     const char* debug_raw_bytes = static_cast<const char*>(message->data());
        //     std::vector<char> debug_segment_view(debug_raw_bytes, debug_raw_bytes + total_segment_size);
            
        //     print_bits(debug_segment_view, "Raw Segment Bytes:");
        //     std::cout << "-----------------------------" << std::endl;
        // }
        // // --- END DEBUGGING BLOCK ---
        // // ========================================================================

        // getting the pointer to the segment
        const char* raw_bytes = static_cast<const char*>(message->data());

        if (static_cast<size_t>(total_segment_size) < sizeof(Segment::Header)) {
            std::cout << "Invalid or corrupt data received!" << std::endl;
            return false;
        }

        // getting the segment's payload
        const Segment::Header* seg_header = reinterpret_cast<const Segment::Header*>(raw_bytes);
        const char* payload_start = raw_bytes + sizeof(Segment::Header);
        size_t payload_size = total_segment_size - sizeof(Segment::Header);

        // // ========================================================================
        // // --- PARSING DEBUGGING BLOCK ---
        // // (You can comment out this entire block to disable logging)
        // {
        //     // Use a mutex to prevent interleaved output from other threads.
        //     // Assumes a 'g_cout_mutex' is defined in a shared utility header.
        //     // std::lock_guard<std::mutex> lock(g_cout_mutex);

        //     std::cout << "\n--- PARSING MESSAGE DEBUG ---" << std::endl;
        //     std::cout << "Parsed Segment Type: " << static_cast<int>(seg_header->type) << std::endl;
        //     std::cout << "Calculated Payload Size: " << payload_size << " bytes" << std::endl;

        //     // Create a safe, temporary view of just the payload to print its bits.
        //     if (payload_size > 0) {
        //         std::vector<char> payload_view(payload_start, payload_start + payload_size);
        //         print_bits(payload_view, "Payload Raw Bytes:");
        //     } else {
        //         std::cout << "Payload Raw Bytes: [EMPTY]" << std::endl;
        //     }
        //     std::cout << "---------------------------" << std::endl;
        // }
        // // --- END DEBUGGING BLOCK ---
        // // ========================================================================

        Segment::MsgType final_type = seg_header->type;
        Address final_source = from;

        std::memmove(message->data(), payload_start, payload_size);

        message->resize(payload_size);
        message->set_type(final_type);
        message->set_source(final_source);
        return true;
    }

    void subscribe_to_requests(TEDS::Type type_id)
    {
        TEDS::Type request = TEDS::make_request_type(type_id);
        subscribe_to_type(request);
    }

    void subscribe_to_responses(TEDS::Type type_id, TEDS::Period interval_ms = 1000)
    {
        TEDS::Type response = TEDS::make_response_type(type_id);
        subscribe_to_type(response);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        send_interest_message(type_id, interval_ms);
    }

    const Address& address() const {
        return _address;
    }

    void update(Conditionally_Data_Observed<Buffer, Address::Port>* obs, Address::Port c, Buffer* d) override {
        queue_incoming_buffer(d);
    }

    void update(Conditionally_Data_Observed<Buffer, TEDS::Type>* obs, TEDS::Type c, Buffer* d) override {
        queue_incoming_buffer(d);
    }

private:

    /**
     * @brief Subscribes to a specific data type by attaching to the type-based routing port.
     * @param obs Pointer to the observer that will handle incoming data of the specified type.
     * @param type_id The unique identifier for the data type to subscribe to.
     */
    void subscribe_to_type(TEDS::Type type_id)
    {
        _channel->attach_type_listener(this, type_id);
        _subscribed_types.insert(type_id);
    }

    void queue_incoming_buffer(Buffer* buf) {
        std::lock_guard<std::mutex> lock(_mutex); // RAII
        _data.push_back(buf);
        _semaphore.v(); // signal that a new message is available
    }

    /**
     * @brief Sends an interest message to the network to subscribe to a specific data type.
     * @param type_id The unique identifier for the data type.
     * @param interval_ms The interval in milliseconds for how often to receive updates of this type
     */
    void send_interest_message(TEDS::Type base_type_id, TEDS::Period interval_ms) {

        std::vector<char> teds_payload = TEDS::create_request_payload(base_type_id, interval_ms);

        Segment segment(Segment::MsgType::TEDS, teds_payload);

        std::vector<char> serialized_segment = segment.get_bytes();

        // // --- DEBUGGING ---
        // // Print the bits of the final serialized data right before it goes to the network.
        // std::cout << "\n--- SENDING INTEREST MESSAGE DEBUG ---" << std::endl;
        // std::cout << "Total size: " << serialized_segment.size() << " bytes" << std::endl;
        // print_bits(serialized_segment, "Raw Segment Bytes:");
        // std::cout << "Message type: " << 1 << std::endl;
        // print_bits(base_type_id, "TedsType:");
        // std::cout << "TedsType Name: " << TEDS::get_type_name(base_type_id) << std::endl; 
        // std::cout << "---------------------------" << std::endl;
        // // --- END DEBUGGING ---

        std::cout << "[Communicator] Sending INTEREST message of type " << TEDS::get_type_name(base_type_id) << std::endl;

        _channel->send(
            _address,
            Address::broadcast(Channel::TYPE_BASED_ROUTING_PORT),
            serialized_segment.data(),
            serialized_segment.size()
        );
    }

    /**
     * @brief Prints the binary representation of a raw byte vector.
     */
    inline void print_bits(const std::vector<char>& buffer, const std::string& label = "") {
        if (!label.empty()) {
            std::cout << label << " ";
        }

        if (buffer.empty()) {
            std::cout << "[EMPTY]" << std::endl;
            return;
        }

        // Print the bits for each byte in the vector
        for (const char& byte : buffer) {
            // Cast to unsigned char to prevent sign extension when printing
            std::cout << std::bitset<8>(static_cast<unsigned char>(byte)) << " ";
        }
        std::cout << std::endl;
    }

    template<typename T>
    void print_bits(const T& value, const std::string& label = "") {
        // Get the raw memory of the value
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&value);
        
        if (!label.empty()) {
            std::cout << label << " ";
        }

        // Print the bits for each byte in order
        for (size_t i = 0; i < sizeof(T); ++i) {
            std::cout << std::bitset<8>(bytes[i]) << " ";
        }
        std::cout << std::endl;
    }

    Channel * _channel;
    Address _address;
    Semaphore _semaphore;
    std::list<Buffer*> _data;
    std::mutex _mutex;
    std::set<TEDS::Type> _subscribed_types;
};


#endif  // COMMUNICATOR_HPP