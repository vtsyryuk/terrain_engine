# Standalone-сборка в Code::Blocks под Windows

## 1. Открыть папку standalone

Все файлы для сдачи standalone-версии лежат здесь:

```text
standalone
```

Откройте в Code::Blocks один файл:

```text
standalone\gauss_with_clusters.cpp
```

## 2. Проверить компилятор

В Code::Blocks:

```text
Settings -> Compiler -> Toolchain executables
```

Выберите MinGW/GCC. Желательно GCC 8+ с поддержкой C++17.

## 3. Собрать через bat

В `cmd`:

```bat
cd standalone
build_windows.bat
```

Или через make/GCC:

```bat
mingw32-make gcc
```

Исполняемый файл появится здесь:

```text
standalone\build-windows\gauss_with_clusters.exe
```

## 4. Batch-запуск

```bat
build-windows\gauss_with_clusters.exe field1_commands.txt --config seminar_config.txt
build-windows\gauss_with_clusters.exe seminar2_commands.txt --config seminar_config.txt
build-windows\gauss_with_clusters.exe seminar3_commands.txt --config seminar_config.txt
```

## 5. Запуск Named Pipes

Откройте два окна `cmd` в папке `standalone`.

Окно 1:

```bat
build-windows\gauss_with_clusters.exe --server --config seminar_config.txt
```

Окно 2:

```bat
build-windows\gauss_with_clusters.exe --client field1_commands.txt --config seminar_config.txt --shutdown
```

Результаты:

```text
standalone\output\field1.bmp
standalone\output\trajectories.bmp
standalone\output\fied1_delaunay.bmp
standalone\output\landscape_kmeans.bmp
standalone\output\fied_em_3.bmp
```
