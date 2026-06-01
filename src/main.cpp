// Modular main: small orchestrator using TerrainEngine and CommandProcessor
#include <iostream>
#include <filesystem>
#include "LogManager.h"
#include "NamedPipeTransport.h"
#include "Server.h"
#include "ServerInterface.h"

#include <stdexcept>
#include <string>

namespace
{
constexpr const char* kDefaultPipeName = R"(\\.\pipe\TerrainPipe)";

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
#ifndef _WIN32
    (void)options;
    throw std::runtime_error("Windows Named Pipes are available only on Windows");
#else
    prepareDirectories();

    Server terrainServer(options.configFile);
    terrainServer.init();
    NamedPipeServer server(options.pipeName);

    std::cout << "[INFO] Server is listening on " << options.pipeName << "\n";
    server.run([&terrainServer](const std::string& command) {
        return terrainServer.processLine(command);
    });
#endif
}

void runClient(const AppOptions& options)
{
#ifndef _WIN32
    (void)options;
    throw std::runtime_error("Windows Named Pipes are available only on Windows");
#else
    ServerInterface server(options.pipeName);
    server.executeFile(options.commandsFile);

    if (options.shutdown)
    {
        std::cout << "SHUTDOWN -> " << server.shutdown() << "\n";
    }
#endif
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
