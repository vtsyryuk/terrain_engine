// Modular main: small orchestrator using TerrainEngine and CommandProcessor
#include <iostream>
#include <filesystem>
#include "Config.h"
#include "TerrainEngine.h"
#include "CommandProcessor.h"
#include "LogManager.h"

int main()
{
    try
    {
        std::filesystem::create_directories("output");
        std::filesystem::create_directories("logs");

        log_mgr.setup("app");

        Config config;
        config.load("config.txt");

        TerrainEngine engine(config.width, config.height);

        CommandProcessor processor(engine);
        processor.executeFile("commands.txt");

        std::cout << "[INFO] Completed successfully\n";
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[ERROR] " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
