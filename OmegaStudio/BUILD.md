# OmegaStudio Build Instructions

## Quick Start (macOS)

```bash
# 1. Clone JUCE framework
git clone https://github.com/juce-framework/JUCE.git --branch 8.0.0 --depth 1

# 2. Configure the project
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build --config Release -j8

# 4. Run
open build/OmegaStudio_artefacts/Release/Omega\ Studio.app
```

## Quick Start (Windows)

```powershell
# 1. Clone JUCE framework
git clone https://github.com/juce-framework/JUCE.git --branch 8.0.0 --depth 1

# 2. Configure the project
cmake -B build -G "Visual Studio 17 2022" -A x64

# 3. Build
cmake --build build --config Release

# 4. Run
.\build\OmegaStudio_artefacts\Release\OmegaStudio.exe
```

## Troubleshooting

### macOS: "Cannot find JUCE"
Make sure the JUCE directory is in the project root:
```
OmegaStudio/
├── CMakeLists.txt
├── JUCE/           ← Must be here
└── Source/
```

### Windows: "MSVC not found"
Install Visual Studio 2022 with "Desktop development with C++" workload.

### Audio Device Not Found
- **macOS**: Go to System Settings → Sound and verify your audio device
- **Windows**: Install ASIO4ALL or use a professional audio interface

### Build Errors with C++23
Ensure you have:
- **macOS**: Xcode 15+ (Apple Clang 15+)
- **Windows**: Visual Studio 2022 17.6+
- **Linux**: GCC 13+ or Clang 16+

## Advanced Build Options

### Build with Verbose Output
```bash
cmake --build build --verbose
```

### Clean Build
```bash
rm -rf build
cmake -B build
cmake --build build
```

### Install (macOS)
```bash
cmake --install build
```

This will copy Omega Studio.app to /Applications

### Custom JUCE Location
```bash
cmake -B build -DJUCE_DIR=/path/to/JUCE
```

## Development Tips

### Debug Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Debug builds include:
- Assertions enabled
- Full debug symbols
- No optimizations
- Detailed logging

### Release Build (Production)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Release builds include:
- Full optimizations (-O3)
- Link-Time Optimization (LTO)
- SIMD enabled (AVX2/NEON)
- Stripped symbols

### Profile Build (Performance Analysis)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

Includes optimizations + debug symbols for profiling.

## Platform-Specific Notes

### macOS Apple Silicon
The project automatically detects ARM64 and uses NEON instructions.
No additional configuration needed.

### macOS Intel
The project uses AVX2 instructions. Requires a CPU from 2013 or newer
(Haswell or later).

### Windows ASIO
For professional audio interfaces, install manufacturer-provided ASIO drivers.
For testing, install ASIO4ALL: https://www.asio4all.org/

## CI/CD

GitHub Actions workflow example:
```yaml
- name: Build OmegaStudio
  run: |
    cmake -B build
    cmake --build build --config Release
```

See `.github/workflows/build.yml` for the complete CI setup.
