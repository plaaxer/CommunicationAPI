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
        int address;
    public:
        Vehicle();
        int get_address();
        void send_message();
};

Vehicle::Vehicle() : velocimetro(get_address()){};
int Vehicle::get_address(){
    std::ifstream file("/sys/class/net/eth0/address");
    if (!file.is_open()) {
        return;
    }

    std::string mac;
    std::getline(file, mac); // read first line
    file.close();
    int mac_int = atoi(mac.c_str());
    address = mac_int;
    return address;
};

void Vehicle::send_message(){

}
