#pragma once
#include <fstream>
#include <string>

class LogManager {
public:
    LogManager();
    ~LogManager();
    void setup(const std::string& role);
    void log_system(const std::string& msg);
    void log_user(const std::string& msg);

private:
    std::string role_ = "app";
    std::ofstream system_log_;
    std::ofstream user_log_;
    bool enabled_ = true;
};

// Global instance for compatibility with existing code
extern LogManager log_mgr;
