# Windows PDF Export

Основная документация находится в:

```text
DOCUMENTATION.md
```

## Вариант 1: Visual Studio Code

1. Открыть `DOCUMENTATION.md`.
2. Установить расширение Markdown PDF или аналогичное.
3. Выполнить экспорт в PDF.

## Вариант 2: Browser Print to PDF

1. Открыть Markdown preview в редакторе.
2. Скопировать или открыть preview в браузере.
3. Нажать:

```text
Ctrl + P
```

4. Выбрать:

```text
Microsoft Print to PDF
```

## Вариант 3: Pandoc На Windows

Если установлен Pandoc:

```bat
pandoc DOCUMENTATION.md -o DOCUMENTATION.pdf --toc --toc-depth=2
```

Если нужен PDF через LaTeX, дополнительно установить MiKTeX.

## Что Должно Попасть В PDF

- Windows standalone-сборка.
- Code::Blocks/GCC/MinGW.
- `standalone\gauss_with_clusters.cpp`.
- `build_windows.bat`.
- `mingw32-make gcc`.
- Batch-запуск seminar-команд.
- Client/server через Windows Named Pipes.
- Параллельный запуск трёх клиентов через один сервер.
- Описание command/config файлов и выходных файлов.
