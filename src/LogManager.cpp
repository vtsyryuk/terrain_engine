#include "LogManager.h"
#include <fstream>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>

static const char* LOG_DIR = "logs";

LogManager::LogManager() {}
LogManager::~LogManager()
{
    if (system_log_.is_open()) system_log_.close();
    if (user_log_.is_open()) user_log_.close();
}

void LogManager::setup(const std::string& role)
{
    role_ = role;
    enabled_ = true;
    namespace fs = std::filesystem;
    try {
        if (!fs::exists(LOG_DIR)) fs::create_directory(LOG_DIR);
    } catch (...) {
        enabled_ = false;
        return;
    }

    std::string sysFile = std::string(LOG_DIR) + "/" + role + "_system.log";
    std::string usrFile = std::string(LOG_DIR) + "/" + role + "_user.log";

    if (system_log_.is_open()) system_log_.close();
    if (user_log_.is_open()) user_log_.close();

    system_log_.open(sysFile, std::ios::app);
    user_log_.open(usrFile, std::ios::app);
    enabled_ = system_log_.is_open() && user_log_.is_open();

    if (enabled_) {
        log_system("Log started for " + role);
        log_user("Log started for " + role);
    }
}

void LogManager::log_system(const std::string& msg)
{
    if (!enabled_ || !system_log_.is_open()) return;
    auto t = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(t);
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S") << "] SYSTEM: " << msg;
    system_log_ << ss.str() << std::endl;
    system_log_.flush();
    std::cout << ss.str() << std::endl;
}

void LogManager::log_user(const std::string& msg)
{
    if (!enabled_ || !user_log_.is_open()) return;
    auto t = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(t);
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S") << "] USER: " << msg;
    user_log_ << ss.str() << std::endl;
    user_log_.flush();
    std::cout << ss.str() << std::endl;
}

// Define global instance
LogManager log_mgr;
