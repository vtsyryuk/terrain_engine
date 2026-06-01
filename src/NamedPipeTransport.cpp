#include "NamedPipeTransport.h"
#include "Logger.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <cerrno>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <vector>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

NamedPipeServer::NamedPipeServer(std::string pipeName)
    : pipeName_(std::move(pipeName))
{
}

NamedPipeClient::NamedPipeClient(std::string pipeName)
    : pipeName_(std::move(pipeName))
{
}

namespace
{
void logServerEvent(const std::string& message)
{
    Logger::info(message);
    std::cout << "[SERVER] " << message << "\n";
}

std::string commandSummary(const std::string& command)
{
    if (command.rfind("BATCH\n", 0) == 0 || command.rfind("BATCH\r\n", 0) == 0)
    {
        return "BATCH request (" + std::to_string(command.size()) + " bytes)";
    }

    return command;
}
}

#ifdef _WIN32

namespace
{
constexpr DWORD kBufferSize = 4096;
constexpr int kMaxConnectAttempts = 30;

std::runtime_error lastWindowsError(const std::string& prefix)
{
    return std::runtime_error(prefix + " (WinAPI error " + std::to_string(GetLastError()) + ")");
}

void startServerProcess()
{
    wchar_t exePath[MAX_PATH] = {};
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) == 0)
    {
        throw lastWindowsError("Cannot determine executable path");
    }

    std::wstring commandLine = L"\"";
    commandLine += exePath;
    commandLine += L"\" server";

    std::vector<wchar_t> command(commandLine.begin(), commandLine.end());
    command.push_back(L'\0');

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    if (!CreateProcessW(
            nullptr,
            command.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NEW_CONSOLE,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo))
    {
        throw lastWindowsError("CreateProcessW for server failed");
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    Sleep(1000);
}
}

void NamedPipeServer::run(const std::function<std::string(const std::string&)>& handler)
{
    bool shutdown = false;
    int clientId = 0;

    while (!shutdown)
    {
        HANDLE pipe = CreateNamedPipeA(
            pipeName_.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            kBufferSize,
            kBufferSize,
            0,
            nullptr
        );

        if (pipe == INVALID_HANDLE_VALUE)
        {
            throw lastWindowsError("CreateNamedPipe failed");
        }

        Logger::info("Named Pipe created. Waiting for client...");

        const BOOL connected = ConnectNamedPipe(pipe, nullptr)
            ? TRUE
            : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (!connected)
        {
            CloseHandle(pipe);
            throw lastWindowsError("ConnectNamedPipe failed");
        }

        ++clientId;
        logServerEvent("Client #" + std::to_string(clientId) + " connected via " + pipeName_);

        char buffer[kBufferSize] = {};
        DWORD bytesRead = 0;
        std::string response;

        if (ReadFile(pipe, buffer, kBufferSize - 1, &bytesRead, nullptr))
        {
            std::string command(buffer, bytesRead);
            if (command == "SHUTDOWN")
            {
                logServerEvent("Client #" + std::to_string(clientId) + " requested server shutdown");
                response = "OK SHUTDOWN";
                shutdown = true;
            }
            else
            {
                try
                {
                    logServerEvent("Client #" + std::to_string(clientId) + " executing: " + commandSummary(command));
                    response = handler(command);
                    logServerEvent("Client #" + std::to_string(clientId) + " response: " + response);
                }
                catch (const std::exception& ex)
                {
                    response = std::string("ERROR ") + ex.what();
                    logServerEvent("Client #" + std::to_string(clientId) + " failed: " + response);
                }
            }
        }
        else
        {
            response = "ERROR read failed";
            logServerEvent("Client #" + std::to_string(clientId) + " read failed");
        }

        DWORD bytesWritten = 0;
        WriteFile(pipe, response.c_str(), static_cast<DWORD>(response.size()), &bytesWritten, nullptr);
        FlushFileBuffers(pipe);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
        logServerEvent("Client #" + std::to_string(clientId) + " disconnected");
        if (!shutdown)
        {
            logServerEvent("Ready for next client");
        }
    }

    logServerEvent("Server stopped after shutdown request");
}

std::string NamedPipeClient::send(const std::string& command) const
{
    bool serverStarted = false;

    for (int attempt = 0; attempt < kMaxConnectAttempts; ++attempt)
    {
        HANDLE pipe = CreateFileA(
            pipeName_.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );

        if (pipe == INVALID_HANDLE_VALUE)
        {
            const DWORD err = GetLastError();
            if (err == ERROR_PIPE_BUSY)
            {
                WaitNamedPipeA(pipeName_.c_str(), 1000);
                continue;
            }

            if (!serverStarted)
            {
                serverStarted = true;
                startServerProcess();
                continue;
            }

            Sleep(250);
            continue;
        }

        DWORD mode = PIPE_READMODE_BYTE;
        SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr);

        DWORD bytesWritten = 0;
        if (!WriteFile(pipe, command.c_str(), static_cast<DWORD>(command.size()), &bytesWritten, nullptr))
        {
            CloseHandle(pipe);
            throw lastWindowsError("WriteFile to named pipe failed");
        }

        char buffer[kBufferSize] = {};
        DWORD bytesRead = 0;
        if (!ReadFile(pipe, buffer, kBufferSize - 1, &bytesRead, nullptr))
        {
            CloseHandle(pipe);
            throw lastWindowsError("ReadFile from named pipe failed");
        }

        CloseHandle(pipe);
        return std::string(buffer, bytesRead);
    }

    return "ERROR: No server";
}

#else

namespace
{
constexpr std::size_t kUnixBufferSize = 65536;
constexpr int kMaxUnixConnectAttempts = 30;

std::runtime_error lastUnixError(const std::string& prefix)
{
    return std::runtime_error(prefix + " (" + std::strerror(errno) + ")");
}

void closeIfValid(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

sockaddr_un makeUnixAddress(const std::string& socketPath)
{
    sockaddr_un address {};
    address.sun_family = AF_UNIX;

    if (socketPath.size() >= sizeof(address.sun_path))
    {
        throw std::runtime_error("Unix socket path is too long: " + socketPath);
    }

    std::strncpy(address.sun_path, socketPath.c_str(), sizeof(address.sun_path) - 1);
    return address;
}
}

void NamedPipeServer::run(const std::function<std::string(const std::string&)>& handler)
{
    const int serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        throw lastUnixError("socket failed");
    }

    unlink(pipeName_.c_str());

    sockaddr_un address = makeUnixAddress(pipeName_);
    if (bind(serverFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
    {
        closeIfValid(serverFd);
        throw lastUnixError("bind failed");
    }

    if (listen(serverFd, SOMAXCONN) < 0)
    {
        closeIfValid(serverFd);
        unlink(pipeName_.c_str());
        throw lastUnixError("listen failed");
    }

    Logger::info("Unix domain socket created: " + pipeName_);

    bool shutdown = false;
    int clientId = 0;
    while (!shutdown)
    {
        const int clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            closeIfValid(serverFd);
            unlink(pipeName_.c_str());
            throw lastUnixError("accept failed");
        }

        ++clientId;
        logServerEvent("Client #" + std::to_string(clientId) + " connected via " + pipeName_);

        char buffer[kUnixBufferSize] = {};
        const ssize_t bytesRead = read(clientFd, buffer, sizeof(buffer) - 1);
        std::string response;

        if (bytesRead > 0)
        {
            std::string command(buffer, static_cast<std::size_t>(bytesRead));
            if (command == "SHUTDOWN")
            {
                logServerEvent("Client #" + std::to_string(clientId) + " requested server shutdown");
                response = "OK SHUTDOWN";
                shutdown = true;
            }
            else
            {
                try
                {
                    logServerEvent("Client #" + std::to_string(clientId) + " executing: " + commandSummary(command));
                    response = handler(command);
                    logServerEvent("Client #" + std::to_string(clientId) + " response: " + response);
                }
                catch (const std::exception& ex)
                {
                    response = std::string("ERROR ") + ex.what();
                    logServerEvent("Client #" + std::to_string(clientId) + " failed: " + response);
                }
            }
        }
        else
        {
            response = "ERROR read failed";
            logServerEvent("Client #" + std::to_string(clientId) + " read failed");
        }

        const ssize_t ignored = write(clientFd, response.c_str(), response.size());
        (void)ignored;
        closeIfValid(clientFd);
        logServerEvent("Client #" + std::to_string(clientId) + " disconnected");
        if (!shutdown)
        {
            logServerEvent("Ready for next client");
        }
    }

    closeIfValid(serverFd);
    unlink(pipeName_.c_str());
    logServerEvent("Server stopped after shutdown request");
}

std::string NamedPipeClient::send(const std::string& command) const
{
    sockaddr_un address = makeUnixAddress(pipeName_);

    for (int attempt = 0; attempt < kMaxUnixConnectAttempts; ++attempt)
    {
        const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0)
        {
            throw lastUnixError("socket failed");
        }

        if (connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0)
        {
            const ssize_t bytesWritten = write(fd, command.c_str(), command.size());
            if (bytesWritten < 0)
            {
                closeIfValid(fd);
                throw lastUnixError("write failed");
            }

            char buffer[kUnixBufferSize] = {};
            const ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
            closeIfValid(fd);

            if (bytesRead < 0)
            {
                throw lastUnixError("read failed");
            }

            return std::string(buffer, static_cast<std::size_t>(bytesRead));
        }

        closeIfValid(fd);
        usleep(250000);
    }

    return "ERROR: No server at Unix socket " + pipeName_;
}

#endif

void NamedPipeClient::sendFile(const std::string& filename) const
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open commands file: " + filename);
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const std::string response = send(line);
        std::cout << line << " -> " << response << "\n";
    }
}
