//==============================================================================
// SIMDProcessor.h
// SIMD-optimized audio processing utilities
//
// Provides platform-specific SIMD implementations:
// - x86/x64: SSE4, AVX2, AVX-512
// - ARM: NEON
// - Apple Silicon: Accelerate framework
//==============================================================================

#pragma once

#include <cstddef>
#include <cstdint>

// Platform detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define OMEGA_X86_SIMD 1
    #include <immintrin.h>  // AVX/AVX2
#elif defined(__ARM_NEON) || defined(__aarch64__)
    #define OMEGA_ARM_NEON 1
    #include <arm_neon.h>
#endif

#ifdef __APPLE__
    #include <Accelerate/Accelerate.h>
    #define OMEGA_USE_ACCELERATE 1
#endif

namespace Omega::Audio::DSP {

//==============================================================================
// SIMD Alignment
//==============================================================================
constexpr size_t SIMD_ALIGNMENT = 32;  // AVX2 requires 32-byte alignment

//==============================================================================
// SIMD Vector Operations
//==============================================================================
class SIMDProcessor {
public:
    //==========================================================================
    // Vector Addition: dst[i] = a[i] + b[i]
    //==========================================================================
    static void add(float* dst, const float* a, const float* b, size_t numSamples);
    
    //==========================================================================
    // Vector Multiplication: dst[i] = a[i] * b[i]
    //==========================================================================
    static void multiply(float* dst, const float* a, const float* b, size_t numSamples);
    
    //==========================================================================
    // Scalar Multiplication: dst[i] = src[i] * scalar
    //==========================================================================
    static void multiplyScalar(float* dst, const float* src, float scalar, size_t numSamples);
    
    //==========================================================================
    // Accumulate: dst[i] += src[i] * gain
    //==========================================================================
    static void addWithGain(float* dst, const float* src, float gain, size_t numSamples);
    
    //==========================================================================
    // Find peak value in buffer
    //==========================================================================
    static float findPeak(const float* src, size_t numSamples);
    
    //==========================================================================
    // RMS (Root Mean Square) calculation
    //==========================================================================
    static float calculateRMS(const float* src, size_t numSamples);
    
    //==========================================================================
    // Clear buffer (set to zero)
    //==========================================================================
    static void clear(float* dst, size_t numSamples);
    
    //==========================================================================
    // Copy buffer
    //==========================================================================
    static void copy(float* dst, const float* src, size_t numSamples);
};

} // namespace Omega::Audio::DSP
