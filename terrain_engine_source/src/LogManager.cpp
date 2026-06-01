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
LogManager::~LogManager() {}

void LogManager::setup(const std::string& role)
{
    namespace fs = std::filesystem;
    try {
        if (!fs::exists(LOG_DIR)) fs::create_directory(LOG_DIR);
    } catch (...) {}

    std::string sysFile = std::string(LOG_DIR) + "/" + role + "_system.log";
    std::string usrFile = std::string(LOG_DIR) + "/" + role + "_user.log";

    // Open files and write header
    std::ofstream system_log(sysFile, std::ios::app);
    std::ofstream user_log(usrFile, std::ios::app);
    if (system_log.is_open()) {
        auto t = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(t);
        system_log << "[" << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S") << "] SYSTEM: Log started for " << role << "\n";
    }
    if (user_log.is_open()) {
        auto t = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(t);
        user_log << "[" << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S") << "] USER: Log started for " << role << "\n";
    }
}

void LogManager::log_system(const std::string& msg)
{
    namespace fs = std::filesystem;
    std::string sysFile = std::string(LOG_DIR) + "/app_system.log";
    std::ofstream system_log(sysFile, std::ios::app);
    if (!system_log.is_open()) return;
    auto t = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(t);
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S") << "] SYSTEM: " << msg;
    system_log << ss.str() << std::endl;
    std::cout << ss.str() << std::endl;
}

void LogManager::log_user(const std::string& msg)
{
    std::string usrFile = std::string(LOG_DIR) + "/app_user.log";
    std::ofstream user_log(usrFile, std::ios::app);
    if (!user_log.is_open()) return;
    auto t = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(t);
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S") << "] USER: " << msg;
    user_log << ss.str() << std::endl;
    std::cout << ss.str() << std::endl;
}

// Define global instance
LogManager log_mgr;
