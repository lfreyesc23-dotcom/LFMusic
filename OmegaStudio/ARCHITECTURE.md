# Architecture Documentation

## Table of Contents
1. [System Overview](#system-overview)
2. [Threading Model](#threading-model)
3. [Memory Management](#memory-management)
4. [Audio Processing Pipeline](#audio-processing-pipeline)
5. [Real-Time Safety Guidelines](#real-time-safety-guidelines)

---

## System Overview

OmegaStudio follows a strict separation-of-concerns architecture:

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │     GUI      │  │  Sequencer   │  │  File I/O    │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                   Lock-Free FIFO Layer                   │
│          (Messages, Parameters, Meter Values)            │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                   Audio Engine Core                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ Audio Graph  │  │  DSP Engine  │  │ Plugin Host  │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                   Hardware Layer (JUCE)                  │
│              ASIO / CoreAudio / WASAPI                   │
└─────────────────────────────────────────────────────────┘
```

---

## Threading Model

### Thread Responsibilities

#### Audio Thread (Real-Time Priority)
- **Priority**: Maximum (99 on most systems)
- **Allowed Operations**:
  - Audio buffer processing
  - SIMD operations
  - Lock-free atomic reads/writes
  - Memory pool allocations
  - Lock-free FIFO pushes
  
- **Forbidden Operations**:
  - `malloc()` / `free()` / `new` / `delete`
  - Mutex locks
  - System calls
  - File I/O
  - Memory allocations (except from pool)
  - Blocking operations

**Code Example**:
```cpp
// ✅ CORRECT: Real-time safe
void audioCallback(float** outputs, int numSamples) {
    float* temp = audioPool_.allocate();  // RT-safe
    processAudio(outputs, numSamples);
    meterValue_.store(peak, std::memory_order_relaxed);
    audioPool_.deallocate(temp);
}

// ❌ WRONG: Will cause audio dropouts
void audioCallback(float** outputs, int numSamples) {
    std::vector<float> temp(numSamples);  // malloc!
    mutex_.lock();                         // blocking!
    processAudio(outputs, numSamples);
    mutex_.unlock();
}
```

#### GUI Thread (Normal Priority)
- **Priority**: Normal
- **Responsibilities**:
  - UI rendering (60 FPS)
  - User input handling
  - Parameter updates
  - File loading/saving
  - Plugin GUI hosting

---

## Memory Management

### Memory Pool Architecture

```cpp
// Pre-allocated at startup
MemoryPool audioPool(16 * 1024 * 1024, 64);  // 16 MB, 64-byte blocks

// Free list structure (stored in free blocks)
struct FreeNode {
    FreeNode* next;
};

// Allocation: O(1) time, lock-free
void* allocate() {
    SpinLock guard(lock_);  // ~10 nanoseconds
    FreeNode* node = freeList_;
    freeList_ = node->next;
    return node;
}
```

**Performance Characteristics**:
- **Allocation Time**: ~20 nanoseconds
- **Deallocation Time**: ~10 nanoseconds
- **Fragmentation**: Zero (fixed-size blocks)
- **Thread Safety**: Lock-free (spin lock for <100ns)

---

## Audio Processing Pipeline

### Signal Flow

```
Input Devices
     │
     ▼
┌─────────────────┐
│  Audio Graph    │
│  (Topological)  │
└─────────────────┘
     │
     ├──► [Instrument Node 1] ──► [Effect Chain] ──┐
     │                                               │
     ├──► [Instrument Node 2] ──► [Effect Chain] ──┤
     │                                               ├──► [Mixer] ──► [Master] ──► Output
     └──► [Audio Input Node] ───────────────────────┘
```

### Node Processing Order

1. **Topological Sort**: Ensure nodes process in dependency order
2. **Latency Compensation**: Auto-delay earlier nodes to align with longest path
3. **SIMD Processing**: Process 8 samples at once (AVX2)

**Example Cycle**:
```
Buffer Size: 256 samples @ 48kHz = 5.33ms
CPU Budget: ~3.7ms (70% threshold)
Processing Time: ~0.8ms (15% CPU)
Headroom: 4.6x safety margin
```

---

## Real-Time Safety Guidelines

### Rules of the Audio Thread

#### Rule 1: No Unbounded Operations
```cpp
// ❌ BAD: Can take arbitrary time
std::vector<float> v;
v.push_back(sample);  // May resize (malloc)

// ✅ GOOD: Pre-allocated
std::array<float, 1024> v;
v[index++] = sample;
```

#### Rule 2: No Waiting
```cpp
// ❌ BAD: Blocking
std::mutex mutex;
mutex.lock();  // Another thread might hold this for milliseconds

// ✅ GOOD: Lock-free
std::atomic<float> value;
value.store(newValue, std::memory_order_relaxed);
```

#### Rule 3: No System Calls
```cpp
// ❌ BAD: Kernel transition
FILE* f = fopen("data.wav", "r");

// ✅ GOOD: GUI thread loads, audio thread reads
void* preloadedData = guiThread_->getPreloadedSamples();
```

#### Rule 4: No Exceptions
```cpp
// ❌ BAD: Stack unwinding
try {
    processAudio();
} catch (...) { }

// ✅ GOOD: Return error codes
ErrorCode processAudio() noexcept {
    if (!buffer) return ErrorCode::NullBuffer;
    // ...
    return ErrorCode::Success;
}
```

### Verification Tools

#### Static Analysis
```bash
# Clang Thread Safety Analysis
clang++ -Wthread-safety -Werror=thread-safety

# GCC Real-Time Analysis
gcc -fanalyzer -Wanalyzer-too-complex
```

#### Runtime Analysis
```bash
# macOS: Thread Sanitizer
clang++ -fsanitize=thread

# Linux: Valgrind with real-time extensions
valgrind --tool=drd --check-stack-var=yes
```

---

## Plugin Delay Compensation (PDC)

### Problem
Plugins introduce latency (e.g., lookahead compressor: 512 samples)

### Solution
Delay all parallel paths to match longest latency

```
Track 1: [Input] ──► [Comp 512ms] ──► [Delay 0ms] ──► [Mix]
                                                           │
Track 2: [Input] ──► [EQ 0ms] ─────► [Delay 512ms] ──► [Mix]
```

**Algorithm**:
1. Calculate latency of all paths
2. Find maximum latency
3. Insert delay nodes to compensate

---

## Performance Optimization

### SIMD Optimization Example

**Scalar Version**:
```cpp
for (int i = 0; i < 1000; ++i) {
    output[i] = input[i] * gain;
}
// Cycles: ~1000
```

**AVX2 Version**:
```cpp
__m256 vGain = _mm256_set1_ps(gain);
for (int i = 0; i < 1000; i += 8) {
    __m256 v = _mm256_loadu_ps(&input[i]);
    __m256 result = _mm256_mul_ps(v, vGain);
    _mm256_storeu_ps(&output[i], result);
}
// Cycles: ~125 (8x faster)
```

### Cache Optimization

```cpp
// ❌ BAD: Cache miss every iteration
for (int i = 0; i < nodeCount; ++i) {
    nodes[i]->process();  // Scattered memory access
}

// ✅ GOOD: Sequential memory access
for (int i = 0; i < nodeCount; ++i) {
    nodeDataArray[i].process();  // Data-oriented design
}
```

---

## Future Enhancements

### Phase 2: GPU Acceleration
- **Waveform Rendering**: OpenGL/Vulkan
- **Spectral Analysis**: CUDA/Metal compute shaders
- **Plugin GUIs**: Hardware-accelerated compositing

### Phase 3: Distributed Processing
- **Network Audio**: Ultra-low latency (< 5ms)
- **Cloud Rendering**: Offload CPU-intensive tasks
- **Collaborative Sessions**: Real-time multi-user DAW

---

## References

- [JUCE Audio Callback Documentation](https://docs.juce.com/master/classAudioIODeviceCallback.html)
- [Ross Bencina - Lock-Free Audio](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing)
- [Intel Optimization Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
