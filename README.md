# Terrain Engine

[![CI](https://github.com/vtsyryuk/terrain_engine/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/vtsyryuk/terrain_engine/actions/workflows/ci.yml)
[![SonarCloud](https://github.com/vtsyryuk/terrain_engine/actions/workflows/sonarcloud.yml/badge.svg?branch=main)](https://github.com/vtsyryuk/terrain_engine/actions/workflows/sonarcloud.yml)

C++17 terrain generation and analysis with Gaussian fields, clustering, bitmap output, batch commands, and native client/server operation. The repository includes a modular CMake application and a standalone Windows/MinGW implementation.

## CI Status

| Check | What it verifies |
| --- | --- |
| CI | Builds the modular CMake targets and standalone executable; runs the native image-generation demo |
| SonarCloud | Actions analysis of the actual C++ compilation database and an enforced quality gate |
| Coverage | No instrumented automated coverage suite is configured; successful compilation is not a coverage measurement |
| Cloud E2E | No hosted HTTP/browser application is deployed |

The configured SonarCloud Actions analysis has passed its quality gate. Native demo checks produce a real bitmap artifact; they do not claim cloud verification.

## Project Links

- [CI build and native demo](https://github.com/vtsyryuk/terrain_engine/actions/workflows/ci.yml)
- [SonarCloud project](https://sonarcloud.io/project/overview?id=vtsyryuk_terrain_engine)
- [SonarCloud setup guide](.github/SONARCLOUD.md)
- [Native demo script](scripts/demo.sh)
- [Architecture and commands](DOCUMENTATION.md)
- [Windows standalone guide](docs/windows-standalone.md) and [MinGW instructions](CODEBLOCKS_WINDOWS.md)

## Requirements

- C++17 compiler: GCC, Clang, or MinGW-w64.
- CMake 3.16+ for the modular application.
- Bash for the portable demo script; Windows `.bat` entry points are also included.
- `gnuplot` for PNG plots requested by `PLOT` commands. The small bitmap demo below does not require it.

## Build

Modular application:

```bash
cmake -S . -B build
cmake --build build --parallel 2
```

The same helper used by CI builds the modular and standalone targets and records the compiler invocations:

```bash
python3 .github/scripts/build-sonar-cpp.py
```

See [Windows build instructions](docs/windows-standalone.md) for Code::Blocks, MinGW, batch files and named-pipe client/server examples.

## Demo

Generate a small terrain bitmap locally:

```bash
bash scripts/demo.sh
```

The script compiles the standalone program, generates a 64×64 Gaussian terrain field, runs `SCAN`, writes `build/demo/output/demo.bmp`, and checks that the image is non-empty. Open that bitmap in an image viewer. Generated files stay under `build/demo/`.

For the full modular batch example, run from the repository root:

```bash
./build/terrain_app commands.txt --config config.txt
```

The supplied batch may request PNG plots, so install `gnuplot` for that path. More scenarios and configuration examples are in [standalone/README.md](standalone/README.md). The original Windows README is preserved in [docs/windows-standalone.md](docs/windows-standalone.md).

## Test and Coverage

CI compiles the real C++ translation units, then executes the small native generation demo. This checks the compiler/runtime/output path. Broader algorithm assertions and instrumented coverage are not yet configured; coverage reporting remains a separate next step.

## SonarCloud

GitHub Actions owns analysis; Automatic Analysis is disabled. The scanner reads `build/sonar/compile_commands.json` from the real build and waits for the Sonar way quality gate. Build/test CI stays independent. See [the configuration guide](.github/SONARCLOUD.md).

## Cloud Deployment and E2E — Additional App Needed

The existing batch program and native pipe/socket transport are local interfaces. There is no hosted browser demo, HTTP deployment manifest, or configured cloud URL. A cloud browser demo would need an HTTP wrapper around terrain generation, a host for that wrapper and its generated images, and E2E tests against the actual deployed URL.

Native generation and client/server demonstrations can already run locally. Cloud E2E can be added once that HTTP interface is deployed.
