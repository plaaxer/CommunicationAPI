#ifndef SMART_DATA_HPP
#define SMART_DATA_HPP

#include <iostream>
#include <string>

// attributes lenght definitions to support POD easy packing in Packet/Header
constexpr unsigned int MAX_DEV_NAME = 32;
constexpr unsigned int MAX_DATA_TYPE = 16;


/**
 * @ref Data-Centric Cyber-Physical Systems Design With SmartData (Frolich, 2018)
 *
 * @details
 * SmartData will describe units accordingly. TO DO yet
 * TODO: Specify the UNITS accordingly to the real approach
 * TODO: Implement the constructors
 */
class SmartData
{
public:
    // Some temporary information to compose the SmartData API packets and further
    typedef std::string DeviceName;
    typedef std::string DataType;
    typedef int Data;  // To use Unit and better specify later

public:
    /**
     * TODO: constructor and more...
     */

    // Dummy units. ONLY FOR THE MVP!!
    struct Units {
        struct Temperature {
            typedef int ValueType;
            static constexpr int id = 0;
        };

        struct Pressure {
            typedef int ValueType;
            static constexpr int id = 1;
        };

        struct Humidity {
            typedef int ValueType;
            static constexpr int id = 2; 
        };

        struct Voltage {
            typedef int ValueType;
            static constexpr int id = 3; 
        };

    };

    struct Header
    {
        char dev_name[MAX_DEV_NAME];
        char d_type[MAX_DATA_TYPE];

        Header(const std::string& name = "", const std::string& data_type = "") {
            std::strncpy(dev_name, name.c_str(), MAX_DEV_NAME);
            dev_name[MAX_DEV_NAME-1] = '\0';
            std::strncpy(d_type, data_type.c_str(), MAX_DATA_TYPE);
            d_type[MAX_DATA_TYPE-1] = '\0';
        }
    } __attribute__((packed));

    struct Packet
    {
        Header _header;
        Data _data;

        Packet(const Header& header = {}, Data data = 0)
            : _header(header), _data(data) {}

        size_t size() const { return sizeof(Packet); }

        // template<typename T>
        // T* data() { return reinterpret_cast<T*>(&_data); }

        template<typename T>
        T get_data() const {
            T value;
            std::memcpy(&value, &_data, sizeof(T));
            return value;
        }

        Header get_header() const { return _header; }
    }; // __attribute__((packed));  // int have align issues in this case**

};


/**
 * @brief Easy print with stream operator override
 */
inline std::ostream& operator<<(std::ostream& os, SmartData::Packet& p) 
{
    return os << "[Device Name]: " << p._header.dev_name << '\n'
                //<< "[Data Type]: " << p._header.d_type << '\n' // can add later
                << "[Value]: " << p.get_data<int>() << std::endl;
}

#endif // SMART_DATA_HPP