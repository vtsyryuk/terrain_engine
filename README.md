
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

Программа читает `commands.txt` и последовательно выполняет сценарии генерации, анализа и визуализации.

## Что создаётся

Основные файлы заметны в папке `output/`:

- `output/terrain.bmp`
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

Дополнительные вспомогательные файлы также могут появляться в `output/`:

- `output/plot3d.gnuplot`
- `output/plot2d.gnuplot`
- `output/plot_clusters.gnuplot`
- `output/output/plot_geometry.gnuplot`
- `output/terrain_data.txt`
- `output/terrain_data_2d.txt`

Логи записываются в папку `logs/`.
