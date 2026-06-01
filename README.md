
# Terrain Generator Refactored

## Требования

- C++17
- CMake
- `gnuplot` (для генерации графиков PNG)

## Установка зависимостей на macOS

```bash
brew install cmake gnuplot
```

## Сборка

```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```

## Запуск

```bash
./build/terrain_app
```

По умолчанию программа читает `commands.txt` и последовательно выполняет сценарии генерации, анализа и визуализации.

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
