#pragma once
#include <string>

struct Logger
{
    static void info(const std::string& s);
    static void user(const std::string& s);
    static void warn(const std::string& s);
    static void error(const std::string& s);
};
