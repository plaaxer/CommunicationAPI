
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>

class Logger 
{
public:
    enum Level { DEBUG, INFO, WARNING, ERROR, MESSAGE };

    Logger(const std::string& filename = "app.log")
        : logfile(filename, std::ios::app) {}

    void log(Level level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mu);
        logfile << timestamp() << " [" << levelToString(level) << "] " << message << "\n";
        std::cout << timestamp() << " [" << levelToString(level) << "] " << message << "\n";
        logfile.flush();
    }

private:
    std::ofstream logfile;
    std::mutex mu;

    std::string timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return buf;
    }

    std::string levelToString(Level level) {
        switch (level) {
            case DEBUG: return "DEBUG";
            case INFO: return "INFO";
            case WARNING: return "WARNING";
            case ERROR: return "ERROR";
            case MESSAGE: return "MESSAGE";
        }
        return "UNKNOWN";
    }
};

// EXEMPLO DE USO:
// Passar um arquivo para o log na inicialização:
// Logger logger("mylog.txt");
// Executar o log (salva no arquivo e printa no terminal):
// logger.log(Logger::INFO, "Application started");