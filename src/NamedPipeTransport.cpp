#include "NamedPipeTransport.h"
#include "Logger.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

NamedPipeServer::NamedPipeServer(std::string pipeName)
    : pipeName_(std::move(pipeName))
{
}

NamedPipeClient::NamedPipeClient(
    std::string pipeName,
    int maxConnectAttempts,
    int retryDelayMs,
    int maxRetryDelayMs,
    std::string retryStrategy)
    : pipeName_(std::move(pipeName)),
      maxConnectAttempts_(std::max(1, maxConnectAttempts)),
      retryDelayMs_(std::max(1, retryDelayMs)),
      maxRetryDelayMs_(std::max(retryDelayMs_, maxRetryDelayMs)),
      retryStrategy_(std::move(retryStrategy))
{
    std::transform(retryStrategy_.begin(), retryStrategy_.end(), retryStrategy_.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (retryStrategy_ != "fixed" && retryStrategy_ != "linear" && retryStrategy_ != "exponential")
    {
        Logger::warn("Unknown client retry strategy '" + retryStrategy_ + "', using fixed");
        retryStrategy_ = "fixed";
    }
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

int retryDelayForAttempt(int baseDelayMs, int maxDelayMs, const std::string& strategy, int attempt)
{
    long long delay = baseDelayMs;
    if (strategy == "linear")
    {
        delay = static_cast<long long>(baseDelayMs) * (attempt + 1);
    }
    else if (strategy == "exponential")
    {
        delay = baseDelayMs;
        for (int i = 0; i < attempt; ++i)
        {
            delay *= 2;
            if (delay >= maxDelayMs)
            {
                return maxDelayMs;
            }
        }
    }

    return static_cast<int>(std::min<long long>(delay, maxDelayMs));
}

void logClientRetry(
    const std::string& pipeName,
    const std::string& reason,
    int attempt,
    int maxAttempts,
    int delayMs,
    const std::string& strategy)
{
    Logger::warn(
        "Connect attempt " + std::to_string(attempt + 1) + "/"
        + std::to_string(maxAttempts) + " to " + pipeName
        + " failed: " + reason
        + "; retrying in " + std::to_string(delayMs)
        + " ms (" + strategy + ")");
}

void logClientRetriesExhausted(const std::string& pipeName, int maxAttempts)
{
    Logger::error(
        "No server at " + pipeName + " after "
        + std::to_string(maxAttempts) + " connect attempts");
}
}

#ifdef _WIN32

namespace
{
constexpr DWORD kBufferSize = 4096;

std::runtime_error lastWindowsError(const std::string& prefix)
{
    return std::runtime_error(prefix + " (WinAPI error " + std::to_string(GetLastError()) + ")");
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
    for (int attempt = 0; attempt < maxConnectAttempts_; ++attempt)
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
            const bool hasMoreAttempts = attempt + 1 < maxConnectAttempts_;
            const int delayMs = retryDelayForAttempt(
                retryDelayMs_,
                maxRetryDelayMs_,
                retryStrategy_,
                attempt);
            if (hasMoreAttempts)
            {
                logClientRetry(
                    pipeName_,
                    "WinAPI error " + std::to_string(err),
                    attempt,
                    maxConnectAttempts_,
                    delayMs,
                    retryStrategy_);
            }
            else
            {
                break;
            }

            if (err == ERROR_PIPE_BUSY)
            {
                WaitNamedPipeA(pipeName_.c_str(), static_cast<DWORD>(delayMs));
                continue;
            }

            Sleep(static_cast<DWORD>(delayMs));
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

    logClientRetriesExhausted(pipeName_, maxConnectAttempts_);
    return "ERROR: No server pipe after "
        + std::to_string(maxConnectAttempts_) + " connect attempts";
}

#else

namespace
{
constexpr std::size_t kUnixBufferSize = 65536;
volatile sig_atomic_t gUnixStopRequested = 0;
volatile sig_atomic_t gUnixServerFd = -1;
char gUnixSocketPath[sizeof(sockaddr_un::sun_path)] = {};

void handleUnixShutdownSignal(int)
{
    gUnixStopRequested = 1;
    if (gUnixServerFd >= 0)
    {
        close(static_cast<int>(gUnixServerFd));
        gUnixServerFd = -1;
    }
    if (gUnixSocketPath[0] != '\0')
    {
        unlink(gUnixSocketPath);
    }
}

void installUnixSignalHandlers(const std::string& socketPath, int serverFd)
{
    std::strncpy(gUnixSocketPath, socketPath.c_str(), sizeof(gUnixSocketPath) - 1);
    gUnixSocketPath[sizeof(gUnixSocketPath) - 1] = '\0';
    gUnixServerFd = serverFd;
    gUnixStopRequested = 0;

    struct sigaction shutdownAction {};
    shutdownAction.sa_handler = handleUnixShutdownSignal;
    sigemptyset(&shutdownAction.sa_mask);
    shutdownAction.sa_flags = 0;
    sigaction(SIGINT, &shutdownAction, nullptr);
    sigaction(SIGTERM, &shutdownAction, nullptr);

    struct sigaction pipeAction {};
    pipeAction.sa_handler = SIG_IGN;
    sigemptyset(&pipeAction.sa_mask);
    pipeAction.sa_flags = 0;
    sigaction(SIGPIPE, &pipeAction, nullptr);
}

void clearUnixSignalState()
{
    gUnixServerFd = -1;
    gUnixSocketPath[0] = '\0';
    gUnixStopRequested = 0;
}

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

    installUnixSignalHandlers(pipeName_, serverFd);
    Logger::info("Unix domain socket created: " + pipeName_);

    bool shutdown = false;
    int clientId = 0;
    while (!shutdown)
    {
        const int clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd < 0)
        {
            if (gUnixStopRequested)
            {
                break;
            }

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

    const bool stoppedBySignal = gUnixStopRequested != 0;
    closeIfValid(serverFd);
    unlink(pipeName_.c_str());
    clearUnixSignalState();
    if (stoppedBySignal)
    {
        logServerEvent("Server stopped after signal");
    }
    else
    {
        logServerEvent("Server stopped after shutdown request");
    }
}

std::string NamedPipeClient::send(const std::string& command) const
{
    sockaddr_un address = makeUnixAddress(pipeName_);

    for (int attempt = 0; attempt < maxConnectAttempts_; ++attempt)
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

        const int connectErrno = errno;
        closeIfValid(fd);

        const bool hasMoreAttempts = attempt + 1 < maxConnectAttempts_;
        const int delayMs = retryDelayForAttempt(
            retryDelayMs_,
            maxRetryDelayMs_,
            retryStrategy_,
            attempt);
        if (hasMoreAttempts)
        {
            logClientRetry(
                pipeName_,
                std::strerror(connectErrno),
                attempt,
                maxConnectAttempts_,
                delayMs,
                retryStrategy_);
            usleep(static_cast<useconds_t>(delayMs) * 1000);
        }
    }

    logClientRetriesExhausted(pipeName_, maxConnectAttempts_);
    return "ERROR: No server at Unix socket " + pipeName_ + " after "
        + std::to_string(maxConnectAttempts_) + " connect attempts";
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
