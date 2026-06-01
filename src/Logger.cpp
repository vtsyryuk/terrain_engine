#include "Logger.h"
#include "LogManager.h"

void Logger::info(const std::string& s) { log_mgr.log_system(s); }
void Logger::user(const std::string& s) { log_mgr.log_user(s); }
void Logger::warn(const std::string& s) { log_mgr.log_system(std::string("WARNING: ") + s); }
void Logger::error(const std::string& s) { log_mgr.log_system(std::string("ERROR: ") + s); }
