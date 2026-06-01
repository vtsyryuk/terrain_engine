# Сборка в Code::Blocks под Windows

## 1. Открыть проект

Откройте в Code::Blocks файл:

```text
TerrainGenerator_CodeBlocks.cbp
```

## 2. Проверить компилятор

В Code::Blocks:

```text
Settings -> Compiler -> Toolchain executables
```

Выберите MinGW/GCC. Желательно GCC 8+ с поддержкой C++17.

## 3. Собрать

Выберите цель:

```text
Release
```

Затем:

```text
Build -> Build
```

Исполняемый файл появится здесь:

```text
bin/Release/terrain_app.exe
```

## 4. Запуск Named Pipes

Откройте два окна `cmd` в папке проекта.

Окно 1:

```bat
bin\Release\terrain_app.exe --server
```

Окно 2:

```bat
bin\Release\terrain_app.exe --client commands.txt --shutdown
```

## 5. Семинарские примеры

Для проверки примеров из PDF:

```bat
bin\Release\terrain_app.exe --config files\seminar_config.txt files\seminar1_commands.txt
bin\Release\terrain_app.exe --config files\seminar_config.txt files\seminar2_commands.txt
bin\Release\terrain_app.exe --config files\seminar_config.txt files\seminar3_commands.txt
```

Результаты:

```text
output\terrain_seminar1.png
output\terrain_seminar2.png
output\terrain_seminar3.png
output\landscape_seminar1.bmp
output\landscape_seminar2.bmp
output\landscape_seminar3.bmp
```

## Если ошибка с std::filesystem

Проект уже добавляет линковку:

```text
-lstdc++fs
```

Если у вас новый MinGW и Code::Blocks пишет, что `stdc++fs` не найден, удалите эту опцию:

```text
Project -> Build options -> Linker settings -> Other linker options
```

Удалить:

```text
-lstdc++fs
```
