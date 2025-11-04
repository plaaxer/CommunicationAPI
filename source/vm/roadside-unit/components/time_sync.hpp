#ifndef TIME_SYNC_HPP
#define TIME_SYNC_HPP

#include <iostream>
#include <thread>
#include <chrono>

class TimeSync {

    public:

        TimeSync() {}

        ~TimeSync() {}

        void run() {
            std::cout << "Roadside Unit's TimeSync component is running." << std::endl;
            
            // Keep the process alive with minimal CPU usage.
            while (true) {
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
        }

        void receiver_loop() {} // deverá ficar em um loop de receber mensagens, e quando recebe uma mensagem de um carro, chama reply_with_time()

        void reply_with_time() {} // deverá responder a vm que mandou a mensagem para a roadsite unit com um time_payload.hpp (definido em api/networks/definitions)

    private:

};


#endif