ETHERNET_FRAME = [ HEADER | PACKET (DATA) | SIZE ] 
PACKET = [ PORT_HEADER | SEGMENT (DATA) ] 
SEGMENT = [ MSG_TYPE | PAYLOAD (DATA) ]
PAYLOAD = TEDS_PAYLOAD OR CONTROL_PAYLOAD OR [...]

TEDS_PAYLOAD = [ TEDS_HEADER | DATA ]
TEDS_HEADER = [ TEDS_TYPE | PURPOSE ]
PURPOSE = REQUEST OR RESPONSE

FRAME
|
+-- ETHERNET_HEADER
|
`-- PACKET
    |
    +-- PORT_HEADER
    |
    `-- SEGMENT
        |
        +-- MSG_TYPE ["(e.g., 'TEDS')"]
        |
        `-- PAYLOAD
            |
            `-- TEDS_PAYLOAD
                |
                +-- TEDS_HEADER
                |   |
                |   `-- TEDS_TYPE_WITH_PURPOSE ["MSB (PURPOSE) | 31 bits (TEDS_TYPE)"] ----+
                |                                                                         |
                `-- DATA ["(Value or Period)"] <------------------------------------------+
                                                    "Bit 31: 0=Request, 1=Response"

// A consumer component subscribes to receive data (Responses)
uint32_t my_type = TEDS_ENGINE_TEMPERATURE;
communicator->subscribe_to_type(TEDS::make_response_type(my_type));

// A producer component "subscribes" to receive interest messages (Requests)
uint32_t my_type = TEDS_ENGINE_TEMPERATURE;
communicator->subscribe_to_interest(TEDS::make_request_type(my_type)); // This could be a new method

// In Protocol::update(), when a TEDS message arrives:
TEDS::Type full_type = ...; // Extracted from the packet
if (TEDS::is_response(full_type)) {
    // It's a data response
    _type_observed.notify(full_type, buf);
} else {
    // It's an interest request
    _type_observed.notify(full_type, buf);
}
