#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <string>
#include <mutex>
#include <sstream>

/**
 * @brief A singleton Logger
 */
class Logger {
public:
    static Logger& getInstance() {
        static Logger instance; 
        return instance;
    }

    void open(const std::string& log_path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stream.is_open()) {
            m_stream.close();
        }
        m_stream.open(log_path, std::ios::out | std::ios::trunc);
    }

    void log(const std::stringstream& message_stream) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stream.is_open()) {
            m_stream << message_stream.str() << "\n";
        }
    }

private:
    Logger() {} 
    ~Logger() {
        if (m_stream.is_open()) {
            m_stream.close();
        }
    }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::ofstream m_stream;
    std::mutex    m_mutex;
};


/**
 * @details 
 * A temporary class that stores the log message.
 * When it is destroyed (finds a ";"") he sends the
 * message for the logger singleton
 */
class LogEntry
{
public:
    // prepares the buffer
    LogEntry() {}

    // sends the message for the logger
    ~LogEntry() {
        Logger::getInstance().log(m_buffer);
    }

    template<typename T>
    LogEntry& operator<<(const T& msg) {
        m_buffer << msg;
        return *this;
    }

private:
    std::stringstream m_buffer;
};

// macro to simplify the use
#define LOG_STREAM LogEntry()

#endif // LOGGER_HPP