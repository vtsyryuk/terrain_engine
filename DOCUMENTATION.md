# Terrain Generator: Windows Standalone Documentation

## Назначение

Программа предназначена для генерации искусственного рельефа по набору гауссовых функций, сохранения результатов в BMP/DAT/PNG и выполнения анализа:

- построение поля высот;
- сохранение карты рельефа в BMP;
- построение карты траекторий/крутизны;
- триангуляция Делоне;
- кластеризация K-means;
- кластеризация EM;
- запуск в режиме Windows client/server через Named Pipes.

Актуальная Windows-версия находится только в папке:

```text
standalone
```

## Состав Папки Standalone

```text
standalone/
  gauss_with_clusters.cpp      основной single-file исходник
  build_windows.bat            сборка через GCC/MinGW
  Makefile                     сборка через make/mingw32-make
  run_parallel_clients.bat     один сервер и три клиента параллельно
  seminar_config.txt           конфигурация сетки
  seminar1_commands.txt        команды семинара 1
  seminar2_commands.txt        команды семинара 2
  seminar3_commands.txt        команды семинара 3
  commands.txt                 дополнительный пример
  config.txt                   дополнительная конфигурация
```

Сгенерированные файлы создаются в:

```text
standalone/output/
standalone/logs/
standalone/build-windows/
```

Эти папки не предназначены для коммита.

## Требования Windows

- Windows 10/11.
- GCC/MinGW с поддержкой C++17.
- Code::Blocks с MinGW или отдельный MinGW-w64.
- Желательно добавить MinGW `bin` в `PATH`, например:

```text
C:\Program Files\CodeBlocks\MinGW\bin
```

Для PNG-графиков нужен `gnuplot` в `PATH`. BMP/DAT файлы создаются без gnuplot.

## Сборка

Перейти в папку standalone:

```bat
cd standalone
```

Сборка через bat:

```bat
build_windows.bat
```

Сборка через make:

```bat
mingw32-make gcc
```

Или вручную через GCC:

```bat
gcc -x c++ -std=c++17 -Wall -Wextra -pedantic gauss_with_clusters.cpp -lstdc++ -o build-windows\gauss_with_clusters.exe
```

После сборки исполняемый файл:

```text
standalone\build-windows\gauss_with_clusters.exe
```

## Batch-Запуск

Запуск по умолчанию выполняет `seminar1_commands.txt` с `seminar_config.txt`:

```bat
build-windows\gauss_with_clusters.exe
```

Явный запуск семинаров:

```bat
build-windows\gauss_with_clusters.exe seminar1_commands.txt --config seminar_config.txt
build-windows\gauss_with_clusters.exe seminar2_commands.txt --config seminar_config.txt
build-windows\gauss_with_clusters.exe seminar3_commands.txt --config seminar_config.txt
```

Через make:

```bat
mingw32-make run-seminar1
mingw32-make run-seminar2
mingw32-make run-seminar3
```

## Client/Server Режим Windows

Программа поддерживает Windows Named Pipes.

Архитектура:

```text
Client process
  -> \\.\pipe\TerrainPipe
Server process
  -> выполняет вычисления
  -> пишет output-файлы
```

Один сервер может обслуживать несколько клиентских сессий. Клиент отправляет весь command-файл одним batch-запросом, поэтому каждая сессия обрабатывается независимо.

Окно 1, сервер:

```bat
build-windows\gauss_with_clusters.exe --server --config seminar_config.txt
```

Окно 2, клиент:

```bat
build-windows\gauss_with_clusters.exe --client seminar1_commands.txt --config seminar_config.txt --shutdown
```

Флаг `--shutdown` отправляет серверу команду завершения после выполнения файла.

## Параллельный Запуск Клиентов

Для запуска трёх клиентов seminar1/2/3 через один сервер:

```bat
run_parallel_clients.bat
```

Или:

```bat
mingw32-make clients-parallel
```

Скрипт делает следующее:

1. Запускает один сервер.
2. Запускает три клиента параллельно.
3. Все клиенты подключаются к одному каналу:

```text
\\.\pipe\TerrainPipe
```

4. После выполнения отправляет серверу `--shutdown-only`.

## Конфигурация

Файл:

```text
seminar_config.txt
```

Пример:

```ini
WIDTH=100
HEIGHT=100
NOISE_LEVEL=0
```

Параметры:

- `WIDTH` — ширина карты.
- `HEIGHT` — высота карты.
- `NOISE_LEVEL` — уровень шума в процентах.

## Команды

### GAUSS

```text
GAUSS sign cx cy sx sy rho
```

Добавляет гауссову функцию.

- `sign = 1` — холм.
- `sign = -1` — впадина.
- `sign = 0` — совместимость с PDF-вариантом, трактуется как `-1`.
- `cx`, `cy` — центр.
- `sx`, `sy` — размеры.
- `rho` — корреляция. Если значение вне диапазона `(-1, 1)`, программа ставит `rho = 0`.

### GENERATE

```text
GENERATE
```

Создаёт нормализованную карту рельефа.

### SCAN

```text
SCAN
```

Сохраняет сырое поле в `output\field.dat`, затем выполняет генерацию.

### GNUPLOT_FILE

```text
GNUPLOT_FILE 1 to field.dat
```

Сохраняет DAT-файл с текущей картой:

```text
output\field.dat
```

### BMP_WRITE

```text
BMP_WRITE 1 to field1.bmp
```

Сохраняет карту рельефа в BMP.

### TRAJECTORIES

```text
TRAJECTORIES 1 to trajectories.bmp
```

Создаёт BMP-карту траекторий/крутизны.

### DELONE

```text
DELONE trajectories.bmp to fied1_delaunay.bmp
```

Строит триангуляцию Делоне по центрам найденных компонент и сохраняет BMP.

Также поддерживается имя:

```text
DELAUNAY
```

### KMEANS

```text
KMEANS 2 trajectories.bmp to landscape_kmeans.bmp
```

Выполняет K-means кластеризацию и сохраняет BMP с цветными кластерами.

### EM

```text
EM 3 trajectories.bmp to fied_em_3.bmp
```

Выполняет EM-кластеризацию и сохраняет BMP с цветными кластерами.

### PLOT

```text
PLOT 1 field.dat terrain_seminar1.png
```

Создаёт PNG-график через gnuplot.

### EXIT

```text
EXIT
```

Завершает обработку command-файла.

## Пример Command-Файла

```text
GAUSS 1 50 50 10 30 40
GAUSS 1 20 10 5 5 40
GAUSS 1 80 10 5 5 40
GAUSS 0 20 80 5 5 40

GENERATE

GNUPLOT_FILE 1 to field.dat

BMP_WRITE 1 to field1.bmp

TRAJECTORIES 1 to trajectories.bmp
DELONE trajectories.bmp to fied1_delaunay.bmp

KMEANS 2 trajectories.bmp to landscape_kmeans.bmp

EM 3 trajectories.bmp to fied_em_3.bmp
EXIT
```

## Основные Output-Файлы

После запуска `seminar1_commands.txt`:

```text
output\field.dat
output\field1.bmp
output\trajectories.bmp
output\fied1_delaunay.bmp
output\landscape_kmeans.bmp
output\fied_em_3.bmp
```

После запуска seminar-файлов:

```text
output\terrain_seminar1.png
output\landscape_seminar1.bmp
output\terrain_seminar2.png
output\landscape_seminar2.bmp
output\terrain_seminar3.png
output\landscape_seminar3.bmp
```

## Логи

Логи пишутся в:

```text
logs\app_system.log
logs\app_user.log
```

В них сохраняются выполненные команды, предупреждения и сообщения о созданных файлах.

## Проверка Перед Сдачей

В Windows:

```bat
cd standalone
build_windows.bat
build-windows\gauss_with_clusters.exe seminar1_commands.txt --config seminar_config.txt
build-windows\gauss_with_clusters.exe seminar2_commands.txt --config seminar_config.txt
build-windows\gauss_with_clusters.exe seminar3_commands.txt --config seminar_config.txt
run_parallel_clients.bat
```

Проверить, что появились файлы в:

```text
output\
```

## Важное Замечание

Client/server режим использует именно Windows Named Pipes, поэтому он предназначен для запуска на Windows. На других системах программа может быть собрана для проверки batch-логики, но режимы `--server` и `--client` сообщат, что Named Pipes доступны только на Windows.
