
# Terrain Generator Refactored

## Требования

- C++17
- CMake
- `gnuplot` (для генерации графиков PNG)

## Standalone Windows-сборка

Самый простой вариант для сдачи и запуска в Code::Blocks находится в папке `standalone/`.
Там лежит один исходный файл, bat-файл сборки и все command/config файлы.

```bat
cd standalone
build_windows.bat
```

Через make/GCC:

```bat
cd standalone
mingw32-make gcc
```

Или вручную через GCC/MinGW:

```bat
cd standalone
gcc -x c++ -std=c++17 -Wall -Wextra -pedantic gauss_with_clusters.cpp -lstdc++ -o build-windows\gauss_with_clusters.exe
```

Batch-запуск:

```bat
build-windows\gauss_with_clusters.exe field1_commands.txt --config seminar_config.txt
```

Клиент-серверный запуск на Windows:

```bat
build-windows\gauss_with_clusters.exe --server --config seminar_config.txt
build-windows\gauss_with_clusters.exe --client field1_commands.txt --config seminar_config.txt --shutdown
```

## macOS/Linux-сборка для проверки batch-режима

```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```

## Запуск

```bash
./build/terrain_app
```

На Windows запуск без параметров работает как клиент. На macOS/Linux запуск без параметров работает как batch-режим: программа читает `commands.txt` и последовательно выполняет сценарии генерации, анализа и визуализации без Named Pipes.

## Клиент-серверный режим Windows Named Pipes

На Windows приложение можно запустить как сервер вычислений и отдельный клиент команд:

```bash
terrain_app --server
terrain_app --client commands.txt
```

Клиент отправляет команды через канал `\\.\pipe\TerrainPipe`, а сервер выполняет генерацию, анализ и сохранение файлов. Чтобы после выполнения команд остановить сервер, используйте:

```bash
terrain_app --client commands.txt --shutdown
```

## Что создаётся

Основные файлы заметны в папке `output/`:

- `output/my_landscape.bmp`
- `output/terrain_3d.png`
- `output/terrain_2d.png`
- `output/gradient_vectors.txt`
- `output/steepness_map.bmp`
- `output/em_clusters.txt`
- `output/em_responsibilities.txt`
- `output/em_overlay.png`
- `output/my_delaunay_voronoi.png`
- `output/delaunay.txt`
- `output/voronoi.txt`
- `output/points_centers.txt`

Дополнительные вспомогательные файлы также могут появляться в `output/`:

- `output/my_plot_script.gnuplot`
- `output/my_plot_2d.gnuplot`
- `output/plot_clusters.gnuplot`
- `output/plot_geometry.gnuplot`
- `output/terrain_data.txt`
- `output/terrain_data_2d.txt`

Логи записываются в папку `logs/`.
