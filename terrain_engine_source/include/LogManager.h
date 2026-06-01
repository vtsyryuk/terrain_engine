#pragma once
#include <string>

class LogManager {
public:
    LogManager();
    ~LogManager();
    void setup(const std::string& role);
    void log_system(const std::string& msg);
    void log_user(const std::string& msg);
};

// Global instance for compatibility with existing code
extern LogManager log_mgr;
