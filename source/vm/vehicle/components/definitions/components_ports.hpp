#ifndef COMPONENT_PORTS_H
#define COMPONENT_PORTS_H

#include "api/network/definitions/address.hpp"

/**
 * @brief Defines universal port mappings for inter-component communication.
 *
 * This class provides a centralized, static mapping of component/service types
 * to predefined port numbers.

 * A component "subscribes" to its designated port to receive all messages
 * directed to that service type.
 *
 * Example Usage:
 * ComponentPorts::Port gps_port = static_cast<ComponentPorts::Port>(ComponentPorts::ID::GPS_SENSOR);
 */
class ComponentPorts {
public:

    typedef typename Address::Port Port; 

    /**
     * @enum ID
     * @brief Type-safe enumeration of all known component and service types.
     */
    enum class ID : Port {

        GPS_SENSOR      = 1001

    };

private:

    // shouldn't be instantiated
    ComponentPorts() = delete;
};

#endif // COMPONENT_PORTS_H
