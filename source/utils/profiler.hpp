#ifndef PROFILER_HPP
#define PROFILER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>

class Profiler
{
public:
    Profiler(const std::string& filename = "app.log");
    void profile(int message);

private:
    std::ofstream outfile;
    std::mutex mu;
    std::time_t startTime;
    std::string timestamp();
    std::string messageType(int type);
};

#endif // PROFILER_HPP