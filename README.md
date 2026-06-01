# Terrain Generator: Windows Standalone

Проект подготовлен для запуска на Windows как standalone-приложение из папки `standalone/`.

Основной файл:

```text
standalone\gauss_with_clusters.cpp
```

В этой папке также лежат все command/config файлы, bat-скрипты и Makefile для сборки через GCC/MinGW.

## Требования

- Windows 10/11.
- GCC/MinGW с поддержкой C++17.
- Code::Blocks с MinGW или отдельный MinGW-w64.
- `gnuplot` в `PATH` для PNG-графиков.

## Сборка

```bat
cd standalone
build_windows.bat
```

Или через make:

```bat
cd standalone
mingw32-make gcc
```

Или вручную:

```bat
cd standalone
gcc -x c++ -std=c++17 -Wall -Wextra -pedantic gauss_with_clusters.cpp -lstdc++ -o build-windows\gauss_with_clusters.exe
```

## Batch-Запуск

```bat
build-windows\gauss_with_clusters.exe seminar1_commands.txt --config seminar_config.txt
build-windows\gauss_with_clusters.exe seminar2_commands.txt --config seminar_config.txt
build-windows\gauss_with_clusters.exe seminar3_commands.txt --config seminar_config.txt
```

Запуск по умолчанию:

```bat
build-windows\gauss_with_clusters.exe
```

## Client/Server Named Pipes

Окно 1:

```bat
build-windows\gauss_with_clusters.exe --server --config seminar_config.txt
```

Окно 2:

```bat
build-windows\gauss_with_clusters.exe --client seminar1_commands.txt --config seminar_config.txt --shutdown
```

Используется один Windows Named Pipe:

```text
\\.\pipe\TerrainPipe
```

Один сервер может обслуживать несколько клиентских сессий. Клиент отправляет command-файл одним batch-запросом.

## Параллельный Запуск Клиентов

```bat
run_parallel_clients.bat
```

Или:

```bat
mingw32-make clients-parallel
```

Скрипт запускает один сервер и три клиента:

```text
seminar1_commands.txt
seminar2_commands.txt
seminar3_commands.txt
```

## Output

Результаты создаются в:

```text
standalone\output\
```

Основные файлы:

```text
field1.bmp
trajectories.bmp
fied1_delaunay.bmp
landscape_kmeans.bmp
fied_em_3.bmp
terrain_seminar1.png
terrain_seminar2.png
terrain_seminar3.png
landscape_seminar1.bmp
landscape_seminar2.bmp
landscape_seminar3.bmp
```

Подробная Windows-документация:

```text
DOCUMENTATION.md
CODEBLOCKS_WINDOWS.md
standalone\README.md
```
