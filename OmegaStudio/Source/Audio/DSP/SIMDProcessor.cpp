//==============================================================================
// SIMDProcessor.cpp
// Platform-specific SIMD implementations
//==============================================================================

#include "SIMDProcessor.h"
#include <cmath>
#include <algorithm>

namespace Omega::Audio::DSP {

//==============================================================================
// Helper: Process remaining samples (scalar fallback)
//==============================================================================
namespace {
    inline size_t alignDown(size_t value, size_t alignment) {
        return value & ~(alignment - 1);
    }
}

//==============================================================================
void SIMDProcessor::add(float* dst, const float* a, const float* b, size_t numSamples) {
#ifdef OMEGA_USE_ACCELERATE
    // Use Apple's optimized vector library
    vDSP_vadd(a, 1, b, 1, dst, 1, numSamples);
#elif defined(OMEGA_X86_SIMD)
    // AVX2: Process 8 floats at once
    const size_t numVectors = numSamples / 8;
    const size_t vectorizedSamples = numVectors * 8;
    
    for (size_t i = 0; i < vectorizedSamples; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 result = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(&dst[i], result);
    }
    
    // Handle remaining samples
    for (size_t i = vectorizedSamples; i < numSamples; ++i) {
        dst[i] = a[i] + b[i];
    }
#else
    // Scalar fallback
    for (size_t i = 0; i < numSamples; ++i) {
        dst[i] = a[i] + b[i];
    }
#endif
}

//==============================================================================
void SIMDProcessor::multiply(float* dst, const float* a, const float* b, size_t numSamples) {
#ifdef OMEGA_USE_ACCELERATE
    vDSP_vmul(a, 1, b, 1, dst, 1, numSamples);
#elif defined(OMEGA_X86_SIMD)
    const size_t vectorizedSamples = alignDown(numSamples, 8);
    
    for (size_t i = 0; i < vectorizedSamples; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 result = _mm256_mul_ps(va, vb);
        _mm256_storeu_ps(&dst[i], result);
    }
    
    for (size_t i = vectorizedSamples; i < numSamples; ++i) {
        dst[i] = a[i] * b[i];
    }
#else
    for (size_t i = 0; i < numSamples; ++i) {
        dst[i] = a[i] * b[i];
    }
#endif
}

//==============================================================================
void SIMDProcessor::multiplyScalar(float* dst, const float* src, float scalar, size_t numSamples) {
#ifdef OMEGA_USE_ACCELERATE
    vDSP_vsmul(src, 1, &scalar, dst, 1, numSamples);
#elif defined(OMEGA_X86_SIMD)
    const __m256 vScalar = _mm256_set1_ps(scalar);
    const size_t vectorizedSamples = alignDown(numSamples, 8);
    
    for (size_t i = 0; i < vectorizedSamples; i += 8) {
        __m256 v = _mm256_loadu_ps(&src[i]);
        __m256 result = _mm256_mul_ps(v, vScalar);
        _mm256_storeu_ps(&dst[i], result);
    }
    
    for (size_t i = vectorizedSamples; i < numSamples; ++i) {
        dst[i] = src[i] * scalar;
    }
#else
    for (size_t i = 0; i < numSamples; ++i) {
        dst[i] = src[i] * scalar;
    }
#endif
}

//==============================================================================
void SIMDProcessor::addWithGain(float* dst, const float* src, float gain, size_t numSamples) {
#ifdef OMEGA_USE_ACCELERATE
    vDSP_vsma(src, 1, &gain, dst, 1, dst, 1, numSamples);
#elif defined(OMEGA_X86_SIMD)
    const __m256 vGain = _mm256_set1_ps(gain);
    const size_t vectorizedSamples = alignDown(numSamples, 8);
    
    for (size_t i = 0; i < vectorizedSamples; i += 8) {
        __m256 vSrc = _mm256_loadu_ps(&src[i]);
        __m256 vDst = _mm256_loadu_ps(&dst[i]);
        __m256 result = _mm256_fmadd_ps(vSrc, vGain, vDst);  // FMA: src*gain + dst
        _mm256_storeu_ps(&dst[i], result);
    }
    
    for (size_t i = vectorizedSamples; i < numSamples; ++i) {
        dst[i] += src[i] * gain;
    }
#else
    for (size_t i = 0; i < numSamples; ++i) {
        dst[i] += src[i] * gain;
    }
#endif
}

//==============================================================================
float SIMDProcessor::findPeak(const float* src, size_t numSamples) {
#ifdef OMEGA_USE_ACCELERATE
    float peak = 0.0f;
    vDSP_maxmgv(src, 1, &peak, numSamples);
    return peak;
#elif defined(OMEGA_X86_SIMD)
    __m256 vMax = _mm256_setzero_ps();
    const size_t vectorizedSamples = alignDown(numSamples, 8);
    
    for (size_t i = 0; i < vectorizedSamples; i += 8) {
        __m256 v = _mm256_loadu_ps(&src[i]);
        v = _mm256_andnot_ps(_mm256_set1_ps(-0.0f), v);  // Absolute value
        vMax = _mm256_max_ps(vMax, v);
    }
    
    // Horizontal maximum
    alignas(32) float maxValues[8];
    _mm256_store_ps(maxValues, vMax);
    float peak = *std::max_element(maxValues, maxValues + 8);
    
    // Handle remaining samples
    for (size_t i = vectorizedSamples; i < numSamples; ++i) {
        peak = std::max(peak, std::abs(src[i]));
    }
    
    return peak;
#else
    float peak = 0.0f;
    for (size_t i = 0; i < numSamples; ++i) {
        peak = std::max(peak, std::abs(src[i]));
    }
    return peak;
#endif
}

//==============================================================================
float SIMDProcessor::calculateRMS(const float* src, size_t numSamples) {
    if (numSamples == 0) return 0.0f;
    
#ifdef OMEGA_USE_ACCELERATE
    float rms = 0.0f;
    vDSP_rmsqv(src, 1, &rms, numSamples);
    return rms;
#elif defined(OMEGA_X86_SIMD)
    __m256 vSum = _mm256_setzero_ps();
    const size_t vectorizedSamples = alignDown(numSamples, 8);
    
    for (size_t i = 0; i < vectorizedSamples; i += 8) {
        __m256 v = _mm256_loadu_ps(&src[i]);
        vSum = _mm256_fmadd_ps(v, v, vSum);  // sum += v * v
    }
    
    // Horizontal sum
    alignas(32) float sumValues[8];
    _mm256_store_ps(sumValues, vSum);
    float sum = 0.0f;
    for (int i = 0; i < 8; ++i) {
        sum += sumValues[i];
    }
    
    // Handle remaining samples
    for (size_t i = vectorizedSamples; i < numSamples; ++i) {
        sum += src[i] * src[i];
    }
    
    return std::sqrt(sum / static_cast<float>(numSamples));
#else
    float sum = 0.0f;
    for (size_t i = 0; i < numSamples; ++i) {
        sum += src[i] * src[i];
    }
    return std::sqrt(sum / static_cast<float>(numSamples));
#endif
}

//==============================================================================
void SIMDProcessor::clear(float* dst, size_t numSamples) {
#ifdef OMEGA_USE_ACCELERATE
    vDSP_vclr(dst, 1, numSamples);
#elif defined(OMEGA_X86_SIMD)
    const __m256 zero = _mm256_setzero_ps();
    const size_t vectorizedSamples = alignDown(numSamples, 8);
    
    for (size_t i = 0; i < vectorizedSamples; i += 8) {
        _mm256_storeu_ps(&dst[i], zero);
    }
    
    for (size_t i = vectorizedSamples; i < numSamples; ++i) {
        dst[i] = 0.0f;
    }
#else
    std::fill_n(dst, numSamples, 0.0f);
#endif
}

//==============================================================================
void SIMDProcessor::copy(float* dst, const float* src, size_t numSamples) {
    std::copy_n(src, numSamples, dst);
}

} // namespace Omega::Audio::DSP
