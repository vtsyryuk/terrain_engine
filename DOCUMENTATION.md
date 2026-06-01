# Генератор Ландшафта - Документация Проекта

## Введение

Проект **Terrain Generator** — это модульное приложение на C++17 для синтеза и анализа искусственных ландшафтов. Приложение реализует полный конвейер обработки: от генерации поверхности через гауссовы колокола до кластеризации и геометрического анализа.

## Архитектура Проекта

### Общая структура

Программный комплекс построен на основе клиент-серверной архитектуры с использованием именованных каналов Windows (Named Pipes) в качестве транспортного механизма. Основные удобства:

1. Четкое разделение ответственности: клиент отвечает только за взаимодействие с пользователем и отправку команд, сервер — за всю вычислительную работу.
2. Возможность повторного использования: один экземпляр сервера может обслуживать множество клиентских сессий.
3. Устойчивость к сбоям: клиент и сервер работают в разных процессах, что предотвращает полное падение системы при ошибке в одном из компонентов.
4. Потенциал для распределенных вычислений: архитектура допускает размещение клиента и сервера на разных машинах при переходе на сетевые каналы.
5. Удобство отладки и тестирования: компоненты могут тестироваться независимо друг от друга.

Проект собирает исполняемый файл `terrain_app`, который использует статическую библиотеку `libterrain_core.a`. На Windows приложение может запускаться как сервер или клиент Named Pipes; на macOS/Linux сохраняется последовательный batch-режим для разработки и проверки основной вычислительной логики.

```
terrain_app --client commands.txt
    ↓ Named Pipe: \\.\pipe\TerrainPipe
terrain_app --server
    ↓
Server : ILandscapeOperations (вычислительная логика)
    ├→ CommandProcessor (совместимый парсер команд)
    ├→ TerrainEngine (генерация и визуализация)
    ├→ Scenarios (анализ и фильтрация)
    ├→ Analysis (кластеризация)
    └→ ClusterVisualizer (визуализация результатов)
```

## Модули

### 1. TerrainEngine
**Файлы**: `include/TerrainEngine.h`, `src/TerrainEngine.cpp`

Основной движок для управления ландшафтом. Содержит объект `LandscapeMap` и координирует все операции с высотной картой.

**Ключевые функции**:
- `add(const GaussianBell& bell)` — добавить гауссиан на карту
- `normalize()` — нормализовать значения в диапазон [0, 255]
- `saveBMP(filename)` — сохранить как BMP файл
- `gradient(x, y)` — вычислить градиент в точке

### 2. LandscapeMap
**Файлы**: `include/LandscapeMap.h`, `src/LandscapeMap.cpp`

Хранилище высотной карты (2D массив double). Представляет поверхность ландшафта и предоставляет операции над ней.

**Основные операции**:
- Чтение/запись высот через `at(x, y)`
- Нормализация: масштабирование значений в [0, 255]
- Вычисление градиента (производной по x и y)
- Сохранение в формате BMP (8-бит палитра, оттенки серого)

### 3. GaussianBell
**Файл**: `include/GaussianBell.h`

Структура данных, описывающая одно гауссово возвышение:
- `sign` — направление (1 = холм, -1 = впадина)
- `cx, cy` — координаты центра
- `sx, sy` — стандартные отклонения (ширина холма)
- `rho` — амплитуда (высота/глубина)

### 4. Scenarios
**Файл**: `src/Scenarios.cpp`

Пространство имен с функциями анализа и фильтрации:

#### `fieldGeneration(map, bells, noise_percent)`
Создание ландшафта из набора гауссивов.
1. Добавить каждый гауссиан на карту
2. Нормализовать значения
3. Применить шум
4. Сгладить фильтром Гаусса (2 итерации)

#### `plot3D(map)` и `plot2D(map)`
Генерация визуализаций через gnuplot.
- **3D**: сплошная поверхность (pm3d) с шагом 5 пикселей
- **2D**: тепловая карта с цветовой палитрой

#### `slopeAnalysis(map)`
Вычисление вектора градиента с шагом 20 пикселей.
Результат: файл `gradient_vectors.txt` (x y angle magnitude).

#### `slopeCheck(map, threshold)`
Создание карты крутизны в формате BMP.
- Вычисляется величина градиента: $\sqrt{g_x^2 + g_y^2}$
- Интенсивность пикселя: `(magnitude / threshold) * 255`
- Результат: `steepness_map.bmp` (черное = плоское, белое = крутое)

#### `geometryScenario(map, min_size)`
Построение триангуляции Делоне и диаграммы Вороного:
1. Найти связные компоненты (пиксели > 200)
2. Вычислить центроиды компонент
3. Построить триангуляцию Делоне
4. Визуализировать через gnuplot

### 5. Analysis
**Файл**: `src/Analysis.cpp`

Алгоритмы кластеризации.

#### K-means
```cpp
Analysis::kmeans_cluster(map, k, min_size)
```
- Инициализация случайных центроидов
- Итеративное приписание пикселей ближайшему центру
- Обновление центроидов
- Критерий сходимости: максимум 100 итераций

Выходные файлы:
- `kmeans.txt` (x y label)

#### Expectation-Maximization (EM)
```cpp
Analysis::em_cluster(map, k, em_iterations, damping)
```
- Инициализация гауссовых смесей
- E-шаг: вычисление ответственности каждого кластера
- M-шаг: обновление параметров
- Демпирование для стабильности

Выходные файлы:
- `em_clusters.txt` (x y cluster_id)
- `em_responsibilities.txt` (ответственности для каждого пикселя)

### 6. ClusterVisualizer
**Файлы**: `include/ClusterVisualizer.h`, `src/ClusterVisualizer.cpp`

Создание PNG визуализации с наложением кластеров на исходную карту.

**Процесс**:
1. Прочитать `em_clusters.txt`
2. Создать gnuplot скрипт с палеттой цветов
3. Наложить точки кластеров поверх тепловой карты
4. Сгенерировать PNG через gnuplot

### 7. GnuplotRenderer
**Файлы**: `include/GnuplotRenderer.h`, `src/GnuplotRenderer.cpp`

Обертка для выполнения gnuplot скриптов.

```cpp
GnuplotRenderer::executeScript(script_path)
```
- Запускает gnuplot с заданным скриптом
- Захватывает вывод для логирования ошибок

### 8. Server и ILandscapeOperations
**Файлы**: `include/ILandscapeOperations.h`, `include/Server.h`, `src/Server.cpp`

Серверная часть реализует интерфейс из учебника:

```cpp
class ILandscapeOperations {
public:
    virtual void addGauss(int sign, double cx, double cy, double sx, double sy, double rho) = 0;
    virtual void generate() = 0;
    virtual void plot() = 0;
    virtual void plot2D() = 0;
    virtual void saveBMP(const std::string& filename = "") = 0;
    virtual void analiz() = 0;
    virtual void slopeCheck() = 0;
    virtual void componentSearch() = 0;
    virtual void geometry() = 0;
};
```

`Server` хранит состояние ландшафта между командами клиента: добавленные гауссовы колокола, карту высот и параметры из `config.txt`.

### 9. CommandProcessor
**Файлы**: `include/CommandProcessor.h`, `src/CommandProcessor.cpp`

Парсер и исполнитель команд из файла `commands.txt`.

**Команды**:
```
GAUSS sign cx cy sx sy rho      # Добавить гауссиан
GENERATE                         # Создать поле
PLOT                            # 3D визуализация
PLOT2D                          # 2D визуализация
BMP_WRITE filename              # Сохранить BMP
ANALIZ                          # Анализ градиентов
SLOPE_CHECK threshold           # Карта крутизны
COMPONENT_SEARCH k min_size     # K-means кластеризация
EM_CLUSTER k                    # EM кластеризация
GEOMETRY min_size               # Триангуляция Делоне
```

### 10. NamedPipeTransport и ServerInterface
**Файлы**: `include/NamedPipeTransport.h`, `src/NamedPipeTransport.cpp`

Транспортный слой для взаимодействия клиента и сервера через Windows Named Pipes.

**Режимы запуска**:
```bash
terrain_app --server
terrain_app --client commands.txt
terrain_app --client commands.txt --shutdown
```

- `NamedPipeServer` создает канал `\\.\pipe\TerrainPipe`, принимает команды и передает их в `Server`.
- `PipeClient` подключается к каналу, при необходимости запускает сервер через `CreateProcessW`.
- `ServerInterface` предоставляет клиентский фасад для команд `gauss`, `generate`, `plot`, `plot2D`, `saveBMP`, `analiz`, `slopeCheck`, `componentSearch`, `geometry`.
- Флаг `--shutdown` отправляет серверу служебную команду завершения после выполнения файла.

### 11. Logger и LogManager
**Файлы**: 
- `include/Logger.h`, `src/Logger.cpp`
- `include/LogManager.h`, `src/LogManager.cpp`

Система логирования в два файла:
- `logs/server_system.log` / `logs/server_user.log` — серверные логи
- `logs/client_system.log` / `logs/client_user.log` — клиентские логи
- `logs/app_system.log` / `logs/app_user.log` — batch-режим на macOS/Linux

```cpp
Logger::info(message);   // SYSTEM лог
Logger::user(message);   // USER лог
Logger::warn(message);   // SYSTEM лог (внимание)
Logger::error(message);  // SYSTEM лог (ошибка)
```

### 12. Config
**Файлы**: `include/Config.h`, `src/Config.cpp`

Загрузчик конфигурации из файла `config.txt`:
```ini
WIDTH=900
HEIGHT=900
NOISE_LEVEL=5.0
KMEANS_K=2
SLOPE_THRESHOLD=2.0
MIN_CLUSTER_SIZE=2
MAX_K=5
```

## Поток выполнения

1. **Инициализация**
   - Создать директории `output/`, `logs/`
   - Загрузить конфигурацию
   - Инициализировать TerrainEngine

2. **Обработка команд**
   - Прочитать `commands.txt`
   - Парсировать и выполнить каждую команду
   - Логировать результаты

3. **Генерация ландшафта**
   - Добавить гауссовы возвышения на карту
   - Нормализовать
   - Применить шум и сглаживание
   - Сохранить как BMP

4. **Анализ**
   - Вычислить градиенты
   - Создать карту крутизны
   - Запустить кластеризацию (K-means, EM)
   - Выполнить геометрический анализ (Делоне/Вороной)

5. **Визуализация**
   - Создать 3D и 2D графики через gnuplot
   - Наложить кластеры на карту
   - Сохранить PNG файлы

## Выходные файлы

### BMP Файлы (8-бит, оттенки серого)
- `my_landscape.bmp` — исходная высотная карта (900×900)
- `steepness_map.bmp` — карта крутизны (черное = плоское, белое = крутое)

### PNG Файлы (gnuplot)
- `terrain_3d.png` — 3D поверхность
- `terrain_2d.png` — 2D тепловая карта
- `em_overlay.png` — кластеры EM наложены на карту
- `my_delaunay_voronoi.png` — триангуляция Делоне и диаграмма Вороного

### Данные (текстовые)
- `gradient_vectors.txt` — x y angle magnitude (шаг 20)
- `kmeans.txt` — x y cluster_id (K-means результаты)
- `em_clusters.txt` — x y cluster_id (EM результаты)
- `em_responsibilities.txt` — вероятности принадлежности к кластерам
- `delaunay.txt` — координаты ребер триангуляции
- `voronoi.txt` — координаты ребер диаграммы Вороного
- `points_centers.txt` — центры компонент

### Скрипты Gnuplot
- `my_plot_script.gnuplot` — скрипт для 3D визуализации
- `my_plot_2d.gnuplot` — скрипт для 2D визуализации
- `plot_clusters.gnuplot` — скрипт для наложения кластеров
- `plot_geometry.gnuplot` — скрипт для геометрии

### Логи
- `logs/app_system.log` — системные сообщения
- `logs/app_user.log` — пользовательские сообщения

## Сборка и Запуск

### Требования
- C++17 компилятор (AppleClang, GCC, Clang)
- CMake >= 3.16
- gnuplot (для визуализации)
- macOS: `brew install cmake gnuplot`

### Сборка
```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```

### Запуск
```bash
./build/terrain_app
```

### Очистка
```bash
rm -rf build output logs
```

## Примеры Использования

### Пример 1: Простой ландшафт
```
# commands.txt
GAUSS 1 450 450 150 150 0.8
GENERATE
PLOT
PLOT2D
BMP_WRITE my_landscape.bmp
```

### Пример 2: Анализ крутизны
```
GAUSS 1 150 150 80 80 0.6
GAUSS -1 450 450 100 100 -0.4
GENERATE
ANALIZ
SLOPE_CHECK 1.5
BMP_WRITE my_landscape.bmp
```

### Пример 3: Кластеризация
```
GAUSS 1 200 200 100 100 0.7
GAUSS 1 700 700 100 100 0.7
GENERATE
COMPONENT_SEARCH 2 100
EM_CLUSTER 2
GEOMETRY 50
```

## Математика

### Гауссово возвышение
$$f(x,y) = \text{sign} \cdot \rho \cdot \exp\left(-\frac{(x-c_x)^2}{2\sigma_x^2} - \frac{(y-c_y)^2}{2\sigma_y^2}\right)$$

### Градиент
$$\nabla f = \left(\frac{\partial f}{\partial x}, \frac{\partial f}{\partial y}\right)$$

Вычисляется через центральные разности:
$$\frac{\partial f}{\partial x} \approx \frac{f(x+1,y) - f(x-1,y)}{2}$$

### K-means
Итеративная минимизация суммы квадратов расстояний:
$$J = \sum_{i=1}^{N} \min_k \|x_i - \mu_k\|^2$$

### EM (Expectation-Maximization)
Максимизация правдоподобия смеси гауссовых распределений:
$$p(x) = \sum_{k=1}^{K} \pi_k \mathcal{N}(x | \mu_k, \Sigma_k)$$

### Триангуляция Делоне
Условие: окружность любого треугольника не содержит других точек.
Вычисляется методом перебора всех троек точек.

## Производительность

Типичные времена выполнения (на macOS M1):
- Генерация поля 900×900: ~100 мс
- K-means (k=2, 100 итераций): ~500 мс
- EM (k=3, 100 итераций): ~1000 мс
- Триангуляция Делоне (1000+ точек): ~100 мс
- Gnuplot визуализация: ~1-5 сек (зависит от запроса)

## Расширения и Модификации

### Добавление новой команды
1. Обновить `CommandProcessor.cpp` для парсирования
2. Реализовать функцию в соответствующем модуле
3. Добавить логирование через `Logger::info()`

### Изменение алгоритма кластеризации
- Отредактировать `Analysis.cpp`
- Предусмотреть сохранение результатов в соответствующие файлы

### Кастомная визуализация
- Создать функцию в `Scenarios.cpp` или новый модуль
- Использовать `GnuplotRenderer` или другую графическую библиотеку

## Заключение

Проект демонстрирует:
- Модульную архитектуру на C++17
- Полный конвейер обработки геоданных
- Применение классических алгоритмов (K-means, EM, Делоне)
- Интеграцию с gnuplot для визуализации
- логирование и обработку ошибок
