#!/bin/bash

set -e

echo "🍓 Building Frutilla Studio for macOS..."

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Step 1: Build OmegaStudio DAW
echo -e "${BLUE}[1/3]${NC} Building OmegaStudio DAW (C++/JUCE)..."
cd OmegaStudio

if [ ! -d "build" ]; then
    echo "Creating build directory..."
    cmake -B build -DCMAKE_BUILD_TYPE=Release
fi

cmake --build build --config Release --parallel $(sysctl -n hw.ncpu)

if [ ! -f "build/OmegaStudio_artefacts/Release/Omega Studio.app/Contents/MacOS/Omega Studio" ]; then
    echo "❌ Failed to build OmegaStudio DAW"
    exit 1
fi

echo -e "${GREEN}✅ OmegaStudio DAW built successfully${NC}"
cd ..

# Step 2: Install Node.js dependencies
echo -e "${BLUE}[2/3]${NC} Installing Node.js dependencies..."
npm install

echo -e "${GREEN}✅ Dependencies installed${NC}"

# Step 3: Build Electron app
echo -e "${BLUE}[3/3]${NC} Building Electron app..."
npm run build:mac

echo ""
echo -e "${GREEN}🎉 Build completed successfully!${NC}"
echo ""
echo "The application has been built to: ${YELLOW}dist/mac/Frutilla Studio.app${NC}"
echo ""
echo "To run the app:"
echo "  ${YELLOW}npm run dev${NC}  (development)"
echo "  ${YELLOW}open 'dist/mac/Frutilla Studio.app'${NC}  (production)"
echo ""
