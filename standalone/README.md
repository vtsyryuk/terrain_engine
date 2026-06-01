# Standalone Build

Build with GCC through make:

```bat
make gcc
```

If Windows/MinGW provides `mingw32-make` instead of `make`:

```bat
mingw32-make gcc
```

Run default sample:

```bat
build-windows\gauss_with_clusters.exe
```

Run client/server on Windows:

```bat
build-windows\gauss_with_clusters.exe --server --config seminar_config.txt
build-windows\gauss_with_clusters.exe --client seminar1_commands.txt --config seminar_config.txt --shutdown
```

Run all three seminar clients in parallel on Windows:

```bat
run_parallel_clients.bat
```

or through make:

```bat
mingw32-make clients-parallel
```

The parallel runner starts one shared server and three client sessions against the same Named Pipe:

```text
\\.\pipe\TerrainPipe
```

Each client sends its whole command file as one batch request, so the server handles seminar1, seminar2 and seminar3 as separate sessions.

The server prints and logs each client connection, command execution, response, disconnect, and readiness for the next client. Pressing `Ctrl+C` or closing/terminating the console requests server shutdown and closes the active pipe handle so the server can exit cleanly.

## Client Retry Settings

The Windows client retries Named Pipe connection failures according to `seminar_config.txt`:

```text
CLIENT_CONNECT_RETRIES=30
CLIENT_CONNECT_RETRY_DELAY_MS=250
CLIENT_CONNECT_RETRY_MAX_DELAY_MS=5000
CLIENT_CONNECT_RETRY_STRATEGY=fixed
```

Supported strategies are `fixed`, `linear`, and `exponential`. Each failed retry is printed to the console and written to `logs\app_system.log`.
