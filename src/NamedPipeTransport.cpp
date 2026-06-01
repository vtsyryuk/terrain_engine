#include "NamedPipeTransport.h"
#include "Logger.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#include <vector>
#endif

NamedPipeServer::NamedPipeServer(std::string pipeName)
    : pipeName_(std::move(pipeName))
{
}

NamedPipeClient::NamedPipeClient(std::string pipeName)
    : pipeName_(std::move(pipeName))
{
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

        Logger::info("Client connected");

        char buffer[kBufferSize] = {};
        DWORD bytesRead = 0;
        std::string response;

        if (ReadFile(pipe, buffer, kBufferSize - 1, &bytesRead, nullptr))
        {
            std::string command(buffer, bytesRead);
            if (command == "SHUTDOWN")
            {
                response = "OK SHUTDOWN";
                shutdown = true;
            }
            else
            {
                try
                {
                    Logger::info("Received: " + command);
                    response = handler(command);
                }
                catch (const std::exception& ex)
                {
                    response = std::string("ERROR ") + ex.what();
                }
            }
        }
        else
        {
            response = "ERROR read failed";
        }

        DWORD bytesWritten = 0;
        WriteFile(pipe, response.c_str(), static_cast<DWORD>(response.size()), &bytesWritten, nullptr);
        FlushFileBuffers(pipe);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
        Logger::info("Client disconnected");
        Logger::info("Pipe closed. Ready for new client.");
    }
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

void NamedPipeServer::run(const std::function<std::string(const std::string&)>&)
{
    throw std::runtime_error("Windows Named Pipes are available only on Windows");
}

std::string NamedPipeClient::send(const std::string&) const
{
    throw std::runtime_error("Windows Named Pipes are available only on Windows");
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
