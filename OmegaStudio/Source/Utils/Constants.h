//==============================================================================
// Constants.h
// Global constants for OmegaStudio
//==============================================================================

#pragma once

#include <cstdint>
#include <cstddef>

namespace Omega {

//==============================================================================
// Audio Constants
//==============================================================================
namespace Audio {
    // Standard sample rates
    constexpr double DEFAULT_SAMPLE_RATE = 48000.0;
    constexpr double HIGH_SAMPLE_RATE = 96000.0;
    constexpr double ULTRA_SAMPLE_RATE = 192000.0;
    
    // Buffer sizes (power of 2 for optimal performance)
    constexpr int MIN_BUFFER_SIZE = 64;
    constexpr int DEFAULT_BUFFER_SIZE = 256;
    constexpr int MAX_BUFFER_SIZE = 2048;
    
    // Channel configuration
    constexpr int MAX_AUDIO_CHANNELS = 64;
    constexpr int DEFAULT_INPUT_CHANNELS = 2;
    constexpr int DEFAULT_OUTPUT_CHANNELS = 2;
    
    // Thread priorities
    constexpr int AUDIO_THREAD_PRIORITY = 99;  // Maximum real-time priority
    
    // SIMD alignment (AVX2 requires 32-byte alignment)
    constexpr size_t SIMD_ALIGNMENT = 32;
}

//==============================================================================
// Memory Constants
//==============================================================================
namespace Memory {
    // Memory pool sizes for audio thread
    constexpr size_t AUDIO_POOL_SIZE = 1024 * 1024 * 16;  // 16 MB
    constexpr size_t POOL_BLOCK_SIZE = 64;                 // 64 bytes per block
    
    // Lock-free FIFO sizes (must be power of 2)
    constexpr size_t MESSAGE_FIFO_SIZE = 4096;
    constexpr size_t AUDIO_FIFO_SIZE = 8192;
}

//==============================================================================
// GUI Constants
//==============================================================================
namespace GUI {
    constexpr int DEFAULT_WINDOW_WIDTH = 1920;
    constexpr int DEFAULT_WINDOW_HEIGHT = 1080;
    constexpr int MIN_WINDOW_WIDTH = 1280;
    constexpr int MIN_WINDOW_HEIGHT = 720;
    constexpr int TARGET_FPS = 60;
}

//==============================================================================
// Plugin Constants
//==============================================================================
namespace Plugin {
    constexpr int MAX_VST3_INSTANCES = 128;
    constexpr int MAX_PLUGIN_PARAMETERS = 1024;
    constexpr double PARAMETER_UPDATE_RATE = 60.0;  // Hz
}

} // namespace Omega
