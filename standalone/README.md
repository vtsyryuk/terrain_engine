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
build-windows\gauss_with_clusters.exe --client field1_commands.txt --config seminar_config.txt --shutdown
```
