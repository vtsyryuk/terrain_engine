// Modular main: small orchestrator using TerrainEngine and CommandProcessor
#include <iostream>
#include <filesystem>
#include "LogManager.h"
#include "NamedPipeTransport.h"
#include "Server.h"
#include "ServerInterface.h"

#include <stdexcept>
#include <string>
#include <sstream>

namespace
{
#ifdef _WIN32
constexpr const char* kDefaultPipeName = R"(\\.\pipe\TerrainPipe)";
#else
constexpr const char* kDefaultPipeName = "/tmp/terrain_engine.sock";
#endif

struct AppOptions
{
#ifdef _WIN32
    std::string mode = "client";
#else
    std::string mode = "batch";
#endif
    std::string commandsFile = "commands.txt";
    std::string configFile = "config.txt";
    std::string pipeName = kDefaultPipeName;
    bool shutdown = false;
};

AppOptions parseOptions(int argc, char* argv[])
{
    AppOptions options;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--server" || arg == "server")
        {
            options.mode = "server";
        }
        else if (arg == "--client" || arg == "client")
        {
            options.mode = "client";
        }
        else if (arg == "--shutdown")
        {
            options.shutdown = true;
        }
        else if (arg == "--pipe" && i + 1 < argc)
        {
            options.pipeName = argv[++i];
        }
        else if (arg == "--config" && i + 1 < argc)
        {
            options.configFile = argv[++i];
        }
        else
        {
            options.commandsFile = arg;
        }
    }

    return options;
}

void prepareDirectories()
{
    std::filesystem::create_directories("output");
    std::filesystem::create_directories("logs");
}

void runBatch(const AppOptions& options)
{
    prepareDirectories();

    Server server(options.configFile);
    server.init();
    executeCommandsFromFile(server, options.commandsFile);

    std::cout << "[INFO] Completed successfully\n";
}

void runServer(const AppOptions& options)
{
    prepareDirectories();

    Server terrainServer(options.configFile);
    terrainServer.init();
    NamedPipeServer server(options.pipeName);

    std::cout << "[INFO] Server is listening on " << options.pipeName << "\n";
    server.run([&terrainServer, &options](const std::string& command) {
        if (command.rfind("BATCH\n", 0) == 0 || command.rfind("BATCH\r\n", 0) == 0)
        {
            Server sessionServer(options.configFile);
            sessionServer.init();

            const std::size_t start = command.find('\n');
            std::istringstream input(command.substr(start == std::string::npos ? command.size() : start + 1));
            std::string line;
            std::string lastResponse = "OK";

            while (std::getline(input, line))
            {
                if (line.empty() || line[0] == '#' || line.find_first_not_of(" \t\r\n") == std::string::npos)
                {
                    continue;
                }

                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);

                std::cout << "[SERVER] Batch command: " << line << "\n";
                if (line == "EXIT")
                {
                    break;
                }

                lastResponse = sessionServer.processLine(line);
            }

            return lastResponse;
        }

        return terrainServer.processLine(command);
    });
}

void runClient(const AppOptions& options)
{
    ServerInterface server(options.pipeName);
    server.executeFile(options.commandsFile);

    if (options.shutdown)
    {
        std::cout << "SHUTDOWN -> " << server.shutdown() << "\n";
    }
}
}

int main(int argc, char* argv[])
{
    try
    {
        const AppOptions options = parseOptions(argc, argv);
        log_mgr.setup(options.mode == "batch" ? "app" : options.mode);
        if (options.mode == "server")
        {
            runServer(options);
        }
        else if (options.mode == "client")
        {
            runClient(options);
        }
        else
        {
            runBatch(options);
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[ERROR] " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
