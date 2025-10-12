#include "api/observer/concurrent_observer.h"
#include "api/network/definitions/buffer.hpp"
#include "api/network/definitions/address.hpp"
#include "api/network/definitions/teds.hpp"

// observer for Port-based notifications
using PortObserver = Conditional_Data_Observer<Buffer<Ethernet::Frame>, Address::Port>;

// observer for TEDS Type-based notifications
using TypeObserver = Conditional_Data_Observer<Buffer<Ethernet::Frame>, TEDS::Type>;