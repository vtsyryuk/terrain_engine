#pragma once

#include <functional>
#include <string>

class NamedPipeServer
{
public:
    explicit NamedPipeServer(std::string pipeName);

    void run(const std::function<std::string(const std::string&)>& handler);

private:
    std::string pipeName_;
};

class NamedPipeClient
{
public:
    explicit NamedPipeClient(
        std::string pipeName,
        int maxConnectAttempts = 30,
        int retryDelayMs = 250,
        int maxRetryDelayMs = 5000,
        std::string retryStrategy = "fixed");

    std::string send(const std::string& command) const;
    void sendFile(const std::string& filename) const;

private:
    std::string pipeName_;
    int maxConnectAttempts_;
    int retryDelayMs_;
    int maxRetryDelayMs_;
    std::string retryStrategy_;
};

using PipeClient = NamedPipeClient;
