@echo off
setlocal enabledelayedexpansion

echo Building Frutilla Studio for Windows...

REM Step 1: Build OmegaStudio DAW
echo [1/3] Building OmegaStudio DAW (C++/JUCE)...
cd OmegaStudio

if not exist "build" (
    echo Creating build directory...
    cmake -B build -DCMAKE_BUILD_TYPE=Release
)

cmake --build build --config Release --parallel %NUMBER_OF_PROCESSORS%

if not exist "build\OmegaStudio_artefacts\Release\Omega Studio.exe" (
    echo Failed to build OmegaStudio DAW
    exit /b 1
)

echo OmegaStudio DAW built successfully
cd ..

REM Step 2: Install Node.js dependencies
echo [2/3] Installing Node.js dependencies...
call npm install

echo Dependencies installed

REM Step 3: Build Electron app
echo [3/3] Building Electron app...
call npm run build:win

echo.
echo Build completed successfully!
echo.
echo The application has been built to: dist\win-unpacked\Frutilla Studio.exe
echo.
echo To run the app:
echo   npm run dev  (development)
echo   start "dist\win-unpacked\Frutilla Studio.exe"  (production)
echo.

endlocal
