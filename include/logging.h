// include/logging.h
#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <mutex>

extern std::mutex log_mutex;

// Helper function to get formatted timestamp
inline std::string get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
    localtime_r(&in_time_t, &tm_buf);
    
    char buffer[128];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %X", &tm_buf);
    return std::string(buffer);
}

// Simple logging class to handle stream operators
class Logger {
private:
    std::string color_code;
    
public:
    Logger(const std::string& level, const std::string& color) : color_code(color) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "\033[" << color << "m[" << get_timestamp() << " " << level << "] ";
    }
    
    ~Logger() {
        std::cout << "\033[0m" << std::endl;
    }
    
    template<typename T>
    Logger& operator<<(const T& data) {
        std::cout << data;
        return *this;
    }
    
    // Special handling for endl
    Logger& operator<<(std::ostream& (*f)(std::ostream&)) {
        f(std::cout);
        return *this;
    }
};

#define LOG_INFO Logger("INFO ", "32")
#define LOG_WARN Logger("WARN ", "33")
#define LOG_ERROR Logger("ERROR", "31")
#define LOG_CUDA Logger("CUDA ", "36")
#define LOG_STRATUM Logger("STRATUM", "35")

// For backward compatibility
#define ENDL ""
