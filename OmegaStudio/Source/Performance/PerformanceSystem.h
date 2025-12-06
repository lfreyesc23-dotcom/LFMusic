//==============================================================================
// PerformanceSystem.h - Sistema de Optimización y Performance
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <atomic>
#include <chrono>

namespace OmegaStudio {

//==============================================================================
/** Profiling Tools - Herramientas de profiling en tiempo real */
class ProfilingTools {
public:
    //==========================================================================
    struct ProfileData {
        juce::String name;
        double averageTime = 0.0;       // En microsegundos
        double maxTime = 0.0;
        double minTime = std::numeric_limits<double>::max();
        int callCount = 0;
        
        double cpuPercentage = 0.0;     // % de CPU usado
    };
    
    //==========================================================================
    class ScopedTimer {
    public:
        ScopedTimer(ProfilingTools& profiler, const juce::String& name);
        ~ScopedTimer();
        
    private:
        ProfilingTools& profiler_;
        juce::String name_;
        std::chrono::high_resolution_clock::time_point startTime_;
    };
    
    //==========================================================================
    ProfilingTools();
    ~ProfilingTools() = default;
    
    // Enable/disable profiling
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }
    
    // Record timing
    void startTiming(const juce::String& name);
    void endTiming(const juce::String& name);
    
    // Get results
    std::vector<ProfileData> getProfilingData() const;
    ProfileData getProfileData(const juce::String& name) const;
    
    // Reset stats
    void reset();
    void resetProfile(const juce::String& name);
    
    // Real-time monitoring
    double getTotalCPUUsage() const;
    double getAudioCallbackTime() const;
    
    // Report generation
    juce::String generateReport() const;
    void exportToCSV(const juce::File& file) const;
    
private:
    //==========================================================================
    std::atomic<bool> enabled_{true};
    
    struct TimingData {
        std::chrono::high_resolution_clock::time_point startTime;
        std::vector<double> times;
        double totalTime = 0.0;
        int callCount = 0;
    };
    
    std::map<juce::String, TimingData> timingData_;
    juce::CriticalSection lock_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProfilingTools)
};

//==============================================================================
/** CPU Load Balancer - Balanceo de carga entre threads */
class CPULoadBalancer {
public:
    //==========================================================================
    struct Task {
        int id;
        juce::String name;
        std::function<void()> function;
        int priority = 0;               // 0-10 (10 = máxima prioridad)
        double estimatedTime = 0.0;     // En ms
        bool realtime = false;          // ¿Requiere processing en tiempo real?
    };
    
    //==========================================================================
    CPULoadBalancer();
    ~CPULoadBalancer();
    
    // Setup
    void initialize(int numThreads = -1);  // -1 = auto detect
    void shutdown();
    
    // Add tasks
    int addTask(const Task& task);
    void removeTask(int taskId);
    void clearTasks();
    
    // Execution
    void processTasks();
    void processRealtimeTasks();
    
    // Thread pool management
    void setNumThreads(int num);
    int getNumThreads() const { return numThreads_; }
    int getOptimalThreadCount() const;
    
    // Load monitoring
    double getCPULoad() const;
    std::vector<double> getThreadLoads() const;
    
    // Priority
    void setTaskPriority(int taskId, int priority);
    
private:
    //==========================================================================
    int numThreads_ = 0;
    juce::ThreadPool threadPool_;
    
    std::vector<Task> tasks_;
    std::map<int, Task> realtimeTasks_;
    
    int nextTaskId_ = 1;
    
    // Load tracking
    std::vector<std::atomic<double>> threadLoads_;
    
    void balanceTasks();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CPULoadBalancer)
};

//==============================================================================
/** Disk Streaming System - Streaming eficiente de audio desde disco */
class DiskStreamingSystem {
public:
    //==========================================================================
    struct StreamHandle {
        int id;
        juce::File file;
        juce::int64 position = 0;
        bool loop = false;
        bool active = false;
    };
    
    //==========================================================================
    DiskStreamingSystem();
    ~DiskStreamingSystem();
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void shutdown();
    
    // Stream management
    int createStream(const juce::File& file, bool loop = false);
    void destroyStream(int streamId);
    void clearStreams();
    
    // Playback
    bool readFromStream(int streamId, juce::AudioBuffer<float>& buffer, int numSamples);
    void setStreamPosition(int streamId, juce::int64 position);
    juce::int64 getStreamPosition(int streamId) const;
    
    // Buffer configuration
    void setBufferSize(int samples);           // Buffer size por stream
    void setPrebufferAmount(int samples);      // Cantidad a prebufferizar
    
    // Performance
    int getNumActiveStreams() const;
    double getDiskUsage() const;               // MB/s
    bool isBufferUnderrun() const;
    
    // Priority
    void setStreamPriority(int streamId, int priority);
    
private:
    //==========================================================================
    double sampleRate_ = 48000.0;
    int samplesPerBlock_ = 512;
    int bufferSize_ = 65536;
    int prebufferAmount_ = 16384;
    
    struct StreamData {
        StreamHandle handle;
        std::unique_ptr<juce::AudioFormatReader> reader;
        juce::AudioBuffer<float> buffer;
        int readPosition = 0;
        int writePosition = 0;
        int priority = 5;
        
        std::atomic<bool> needsRefill{false};
    };
    
    std::map<int, std::unique_ptr<StreamData>> streams_;
    int nextStreamId_ = 1;
    
    // Background loading thread
    class LoadingThread : public juce::Thread {
    public:
        LoadingThread(DiskStreamingSystem& owner);
        void run() override;
        
    private:
        DiskStreamingSystem& owner_;
    };
    
    std::unique_ptr<LoadingThread> loadingThread_;
    
    juce::CriticalSection lock_;
    
    void refillStream(StreamData& stream);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiskStreamingSystem)
};

//==============================================================================
/** Multi-Threaded Mixer - Mixer con processing paralelo */
class MultiThreadedMixer {
public:
    //==========================================================================
    struct Track {
        int id;
        juce::String name;
        juce::AudioBuffer<float> buffer;
        
        float volume = 1.0f;
        float pan = 0.0f;
        bool solo = false;
        bool mute = false;
        
        std::vector<juce::AudioProcessor*> effects;
        
        int processingThread = 0;       // Assigned thread
    };
    
    //==========================================================================
    MultiThreadedMixer();
    ~MultiThreadedMixer() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock, int numThreads = -1);
    void reset();
    
    // Tracks
    int addTrack(const juce::String& name);
    void removeTrack(int trackId);
    void clearTracks();
    
    Track* getTrack(int trackId);
    const Track* getTrack(int trackId) const;
    
    // Processing
    void process(juce::AudioBuffer<float>& output);
    
    // Thread assignment (automático por defecto)
    void setTrackThread(int trackId, int threadIndex);
    void autoAssignThreads();
    
    // Performance
    std::vector<double> getThreadTimings() const;
    double getTotalProcessingTime() const;
    
private:
    //==========================================================================
    double sampleRate_ = 48000.0;
    int samplesPerBlock_ = 512;
    int numThreads_ = 4;
    
    std::map<int, Track> tracks_;
    int nextTrackId_ = 1;
    
    juce::ThreadPool threadPool_;
    
    // Per-thread timing
    std::vector<std::atomic<double>> threadTimings_;
    
    void processTrack(Track& track);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiThreadedMixer)
};

//==============================================================================
/** GPU Acceleration - Aceleración por GPU para DSP */
class GPUAccelerator {
public:
    //==========================================================================
    enum class Operation {
        FFT,
        Convolution,
        Reverb,
        PitchShift,
        TimeStretch,
        Spectrogram
    };
    
    //==========================================================================
    GPUAccelerator();
    ~GPUAccelerator() = default;
    
    // Initialize
    bool initialize();
    void shutdown();
    
    // Check availability
    bool isAvailable() const { return available_; }
    bool isOperationSupported(Operation op) const;
    
    // GPU info
    juce::String getGPUName() const;
    size_t getGPUMemory() const;
    int getComputeUnits() const;
    
    // Operations
    bool performFFT(const float* input, float* output, int size);
    bool performConvolution(const float* input, const float* impulse, 
                           float* output, int inputSize, int impulseSize);
    
    // Performance
    double getGPUUsage() const;
    double getSpeedup() const;              // vs CPU
    
    // Settings
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled_; }
    
private:
    //==========================================================================
    bool available_ = false;
    bool enabled_ = true;
    
    // GPU context (platform-specific)
    #ifdef __APPLE__
        void* metalDevice_ = nullptr;
        void* metalCommandQueue_ = nullptr;
    #elif defined(_WIN32)
        void* d3d11Device_ = nullptr;
        void* d3d11Context_ = nullptr;
    #else
        void* openclContext_ = nullptr;
    #endif
    
    bool initializeMetal();
    bool initializeD3D11();
    bool initializeOpenCL();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GPUAccelerator)
};

//==============================================================================
/** Memory Pool Manager - Pool de memoria para allocaciones rápidas */
class MemoryPoolManager {
public:
    MemoryPoolManager();
    ~MemoryPoolManager();
    
    // Setup
    void initialize(size_t poolSize);
    void shutdown();
    
    // Allocate/Free
    void* allocate(size_t bytes, size_t alignment = 16);
    void deallocate(void* ptr);
    
    // Stats
    size_t getTotalAllocated() const { return totalAllocated_; }
    size_t getFragmentation() const;
    int getNumAllocations() const { return numAllocations_; }
    
    // Defragmentation
    void defragment();
    
private:
    //==========================================================================
    struct Block {
        void* ptr;
        size_t size;
        bool free;
    };
    
    std::vector<Block> blocks_;
    size_t poolSize_ = 0;
    size_t totalAllocated_ = 0;
    int numAllocations_ = 0;
    
    void* pool_ = nullptr;
    juce::CriticalSection lock_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MemoryPoolManager)
};

//==============================================================================
/** SIMD Optimizer - Optimizaciones SIMD automáticas */
class SIMDOptimizer {
public:
    //==========================================================================
    enum class Architecture {
        None,
        SSE2,
        SSE4,
        AVX,
        AVX2,
        AVX512,
        NEON,
        Auto
    };
    
    //==========================================================================
    SIMDOptimizer();
    ~SIMDOptimizer() = default;
    
    // Detection
    Architecture detectArchitecture() const;
    bool isArchitectureAvailable(Architecture arch) const;
    
    // Set architecture
    void setArchitecture(Architecture arch);
    Architecture getArchitecture() const { return architecture_; }
    
    // Optimized operations
    void add(float* dest, const float* src, int numSamples);
    void multiply(float* dest, const float* src, int numSamples);
    void multiplyConstant(float* dest, float constant, int numSamples);
    
    void copyBuffer(float* dest, const float* src, int numSamples);
    void clearBuffer(float* dest, int numSamples);
    
    float findMax(const float* src, int numSamples);
    float findMin(const float* src, int numSamples);
    float sum(const float* src, int numSamples);
    
    // Info
    juce::String getArchitectureName() const;
    int getVectorSize() const;          // 4 for SSE, 8 for AVX, etc
    
private:
    //==========================================================================
    Architecture architecture_ = Architecture::Auto;
    
    // Architecture-specific implementations
    void add_SSE(float* dest, const float* src, int numSamples);
    void add_AVX(float* dest, const float* src, int numSamples);
    void add_NEON(float* dest, const float* src, int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SIMDOptimizer)
};

} // namespace OmegaStudio
