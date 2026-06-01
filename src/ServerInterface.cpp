#include "ServerInterface.h"

#include "LogManager.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

ServerInterface::ServerInterface(std::string pipeName)
    : pipeClient_(std::move(pipeName))
{
}

void ServerInterface::gauss(int s, double x, double y, double sx, double sy, double r)
{
    std::ostringstream oss;
    oss << "GAUSS " << s << " " << x << " " << y << " " << sx << " " << sy << " " << r;
    send(oss.str());
}

void ServerInterface::generate()
{
    send("GENERATE");
}

void ServerInterface::plot()
{
    send("PLOT");
}

void ServerInterface::plot2D()
{
    send("PLOT2D");
}

void ServerInterface::saveBMP(const std::string& name)
{
    send("BMP_WRITE " + name);
}

void ServerInterface::analiz()
{
    send("ANALIZ");
}

void ServerInterface::slopeCheck()
{
    send("SLOPE_CHECK");
}

void ServerInterface::componentSearch()
{
    send("COMPONENT_SEARCH");
}

void ServerInterface::geometry()
{
    send("GEOMETRY");
}

void ServerInterface::executeFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open commands file: " + filename);
    }

    log_mgr.log_user("Reading commands from: " + filename);

    std::ostringstream batch;
    batch << "BATCH\n";

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t") == std::string::npos)
        {
            continue;
        }

        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        log_mgr.log_user("Executing: " + line);
        batch << line << "\n";
    }

    const std::string response = send(batch.str());
    std::cout << filename << " -> " << response << "\n";
    if (response.rfind("ERROR", 0) == 0)
    {
        throw std::runtime_error(response);
    }
}

std::string ServerInterface::shutdown()
{
    return send("SHUTDOWN");
}

std::string ServerInterface::send(const std::string& command)
{
    return pipeClient_.send(command);
}
