#include <cstdlib>
#include <ctime>
#include <chrono>
#include <string>
#include <fstream>
#include <iostream>
#include "car-components/sensor.cpp"
#include "car-components/message-controller.cpp"

class Vehicle {
    private:
        Sensor velocimetro;
        float speed;
        std::string address;
    public:
        Vehicle();
        void update_sensor();
        std::string get_address();
        void send_message();
};

Vehicle::Vehicle() : velocimetro(get_address()) {};

std::string Vehicle::get_address(){
    std::ifstream file("/sys/class/net/eth0/address");
    if (!file.is_open()) {
        return;
    }

    std::string mac;
    std::getline(file, mac); // read first line
    file.close();
    address = mac;
    return address;
};

