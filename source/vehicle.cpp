#include <cstdlib>
#include <ctime>
#include <chrono>
#include <string>
#include <fstream>
#include <iostream>
#include "car-components/sensor.cpp"
#include "car-components/message-controller.cpp"


//TODO: 
// - Get MAC Address from machine (Done)
// - Include controller for communicating with message API 

class Vehicle {
    private:
        Sensor velocimetro;
        float speed;
        std::string address;
    public:
        Vehicle();
        void update_sensor();
        void get_address();
        void send_message();
};

Vehicle::Vehicle(){
    get_address();
    Sensor *velocimetro = new Sensor();
};

void Vehicle::get_address(){
    std::ifstream file("/sys/class/net/eth0/address");
    if (!file.is_open()) {
        return;
    }

    std::string mac;
    std::getline(file, mac); // read first line
    file.close();
    address = mac;
};

