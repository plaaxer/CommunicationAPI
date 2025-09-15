#ifndef PACKET_ENVELOPE_HPP
#define PACKET_ENVELOPE_HPP

#include <cstdint>

// TODO: Calculate the exact length for the application data
// constexpr unsigned int APP_DATA_LENGTH = ;

class PacketEnvelope {

public:
    // TO BE USED LATER
    // typedef unsigned char Data[APP_DATA_LENGTH]; 

public:
    enum MessageType {
        LATENCY = 1,
        SMART_DATA = 2
    };

    struct Header { 
        MessageType msg_type;

        Header(MessageType msg_type)
            : msg_type(msg_type) {}

        // message type getter and setter (not necessary for now, since Header is a Struct - which is public - and is inside the public section of the class. But are being used for organization's sake)
        MessageType get_msg_type() { return msg_type; }
        void set_type(MessageType type) { type = type; }
    };

    struct Packet
    {
        Header header;

        void* get_data() { return data.data(); }
        const void* get_data() const { return data.data(); }

        size_t size() const { return sizeof(Packet); }
        
        // -> Application packet (SmartData or LatencyTest)
        std::vector<uint8_t> data;

        // Data data; // To be used later 

    };


};

#endif