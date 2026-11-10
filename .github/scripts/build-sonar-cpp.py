#!/usr/bin/env python3
"""Build the actual C++ targets and record their compiler invocations for SonarCloud."""
import json
from pathlib import Path
import shutil
import subprocess

root = Path.cwd()
build = root / 'build' / 'sonar'
build.mkdir(parents=True, exist_ok=True)
compiler = shutil.which('g++')
if not compiler:
    raise SystemExit('g++ is required')
entries = []
if (root / 'CMakeLists.txt').exists():
    subprocess.run(['cmake', '-S', str(root), '-B', str(build),
                    '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DCMAKE_BUILD_TYPE=Debug'], check=True)
    subprocess.run(['cmake', '--build', str(build), '--parallel', '2'], check=True)
    entries = json.loads((build / 'compile_commands.json').read_text())
    sources = sorted((root / 'standalone').glob('*.cpp'))
else:
    sources = sorted(root.glob('*.cpp'))
for source in sources:
    command = [compiler, '-std=c++17', '-Wall', '-Wextra', str(source), '-o', str(build / source.stem)]
    subprocess.run(command, check=True)
    entries.append({'directory': str(root), 'file': str(source), 'arguments': command})
if not entries:
    raise SystemExit('No compiler invocations were recorded')
(build / 'compile_commands.json').write_text(json.dumps(entries, indent=2) + '\n')
print(f'Built and recorded {len(entries)} C++ translation units; no test coverage is fabricated.')
