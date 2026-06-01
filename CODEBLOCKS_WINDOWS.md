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
build-windows\gauss_with_clusters.exe seminar1_commands.txt --config seminar_config.txt
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
build-windows\gauss_with_clusters.exe --client seminar1_commands.txt --config seminar_config.txt --shutdown
```

## 6. Параллельный запуск трёх клиентов

В папке `standalone`:

```bat
run_parallel_clients.bat
```

Или через make:

```bat
mingw32-make clients-parallel
```

Этот запуск стартует один общий сервер и три клиентские сессии через один Named Pipe:

```text
\\.\pipe\TerrainPipe
```

Каждый клиент отправляет свой command-файл одним batch-запросом, поэтому сервер обрабатывает `seminar1`, `seminar2` и `seminar3` как отдельные сессии.
