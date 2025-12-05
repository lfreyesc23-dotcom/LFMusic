#!/bin/bash
# OmegaStudio Build Script (macOS/Linux)
# Usage: ./build.sh [clean|debug|release|run]

set -e  # Exit on error

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
BUILD_TYPE="${2:-Release}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Functions
print_header() {
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}  $1${NC}"
    echo -e "${BLUE}================================================${NC}"
}

print_success() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

# Check if JUCE exists
check_juce() {
    if [ ! -d "$PROJECT_DIR/JUCE" ]; then
        print_error "JUCE not found!"
        echo ""
        echo "Please clone JUCE framework:"
        echo "  cd $PROJECT_DIR"
        echo "  git clone https://github.com/juce-framework/JUCE.git --branch 8.0.0 --depth 1"
        exit 1
    fi
    print_success "JUCE found"
}

# Clean build
clean_build() {
    print_header "Cleaning Build"
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    else
        print_warning "Build directory not found (nothing to clean)"
    fi
}

# Configure project
configure() {
    print_header "Configuring Project ($BUILD_TYPE)"
    check_juce
    
    # Detect platform
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        cmake -B "$BUILD_DIR" \
              -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
              -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
    else
        # Linux
        cmake -B "$BUILD_DIR" \
              -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    fi
    
    print_success "Configuration complete"
}

# Build project
build() {
    print_header "Building Project ($BUILD_TYPE)"
    
    # Get CPU count for parallel build
    if [[ "$OSTYPE" == "darwin"* ]]; then
        JOBS=$(sysctl -n hw.ncpu)
    else
        JOBS=$(nproc)
    fi
    
    cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$JOBS"
    print_success "Build complete"
}

# Run application
run_app() {
    print_header "Running OmegaStudio"
    
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        APP_PATH="$BUILD_DIR/OmegaStudio_artefacts/$BUILD_TYPE/Omega Studio.app"
        if [ -d "$APP_PATH" ]; then
            open "$APP_PATH"
            print_success "Application launched"
        else
            print_error "Application not found at: $APP_PATH"
            exit 1
        fi
    else
        # Linux
        BIN_PATH="$BUILD_DIR/OmegaStudio_artefacts/$BUILD_TYPE/OmegaStudio"
        if [ -f "$BIN_PATH" ]; then
            "$BIN_PATH"
            print_success "Application launched"
        else
            print_error "Executable not found at: $BIN_PATH"
            exit 1
        fi
    fi
}

# Main script
main() {
    print_header "OmegaStudio Build Script"
    
    case "${1:-build}" in
        clean)
            clean_build
            ;;
        config)
            configure
            ;;
        build)
            if [ ! -d "$BUILD_DIR/CMakeFiles" ]; then
                configure
            fi
            build
            ;;
        rebuild)
            clean_build
            configure
            build
            ;;
        debug)
            BUILD_TYPE="Debug"
            configure
            build
            ;;
        release)
            BUILD_TYPE="Release"
            configure
            build
            ;;
        run)
            run_app
            ;;
        all)
            clean_build
            configure
            build
            run_app
            ;;
        *)
            echo "Usage: $0 {clean|config|build|rebuild|debug|release|run|all}"
            echo ""
            echo "Commands:"
            echo "  clean    - Remove build directory"
            echo "  config   - Configure CMake"
            echo "  build    - Build project (default)"
            echo "  rebuild  - Clean + configure + build"
            echo "  debug    - Configure and build in Debug mode"
            echo "  release  - Configure and build in Release mode"
            echo "  run      - Run the application"
            echo "  all      - Clean + configure + build + run"
            exit 1
            ;;
    esac
    
    print_success "Done!"
}

main "$@"
