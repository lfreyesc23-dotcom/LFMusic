/**
 * @file SampleManager.h
 * @brief Professional sample library manager with streaming and categorization
 * 
 * Features:
 * - Async loading and streaming from disk
 * - Hierarchical categorization system
 * - Tag-based search and filtering
 * - Automatic BPM and key detection
 * - Sample preview and waveform analysis
 * - Memory-efficient streaming for large files
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <set>
#include <functional>
#include "../../Utils/Constants.h"

namespace omega {

/**
 * @struct SampleMetadata
 * @brief Metadata for a sample
 */
struct SampleMetadata {
    juce::String name;
    juce::File filePath;
    
    // Audio properties
    double sampleRate = 0.0;
    int numChannels = 0;
    int64_t lengthInSamples = 0;
    double lengthInSeconds = 0.0;
    
    // Musical properties
    float bpm = 0.0f;           // Detected BPM
    int key = -1;               // Detected key (0=C, 1=C#, etc.) -1=unknown
    juce::String keyName;       // Human-readable key name
    
    // Categorization
    juce::String category;      // Main category (e.g., "Drums", "Bass")
    juce::String subcategory;   // Subcategory (e.g., "Kick", "808")
    std::set<juce::String> tags; // User-defined tags
    
    // Analysis
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    bool hasAnalysis = false;
    
    // User data
    int rating = 0;             // 0-5 stars
    juce::String description;
    juce::Time dateAdded;
    juce::Time dateModified;
    
    // Internal
    juce::String uuid;          // Unique identifier
};

/**
 * @class Sample
 * @brief Represents a loaded audio sample
 */
class Sample {
public:
    Sample(const SampleMetadata& metadata);
    ~Sample() = default;

    /**
     * Load sample into memory
     * @return True if loaded successfully
     */
    bool load();

    /**
     * Unload sample from memory
     */
    void unload();

    /**
     * Check if sample is loaded in memory
     */
    bool isLoaded() const noexcept { return m_loaded; }

    /**
     * Get audio buffer (only valid if loaded)
     */
    const juce::AudioBuffer<float>* getBuffer() const { return m_buffer.get(); }

    /**
     * Get metadata
     */
    const SampleMetadata& getMetadata() const { return m_metadata; }

    /**
     * Update metadata
     */
    void updateMetadata(const SampleMetadata& metadata) { m_metadata = metadata; }

    /**
     * Create streaming reader for large files
     * @return Audio format reader (caller owns)
     */
    std::unique_ptr<juce::AudioFormatReader> createReader() const;

private:
    SampleMetadata m_metadata;
    std::unique_ptr<juce::AudioBuffer<float>> m_buffer;
    bool m_loaded = false;
};

/**
 * @class SampleLibrary
 * @brief Container for organizing samples
 */
class SampleLibrary {
public:
    SampleLibrary(const juce::String& name);
    ~SampleLibrary() = default;

    /**
     * Add sample to library
     * @param sample Sample to add
     * @return True if added successfully
     */
    bool addSample(std::shared_ptr<Sample> sample);

    /**
     * Remove sample from library
     * @param uuid Sample UUID
     * @return True if removed successfully
     */
    bool removeSample(const juce::String& uuid);

    /**
     * Get sample by UUID
     */
    std::shared_ptr<Sample> getSample(const juce::String& uuid) const;

    /**
     * Get all samples
     */
    std::vector<std::shared_ptr<Sample>> getAllSamples() const;

    /**
     * Get samples by category
     */
    std::vector<std::shared_ptr<Sample>> getSamplesByCategory(const juce::String& category) const;

    /**
     * Search samples by text (searches name, category, tags)
     */
    std::vector<std::shared_ptr<Sample>> searchSamples(const juce::String& query) const;

    /**
     * Filter samples by tags
     */
    std::vector<std::shared_ptr<Sample>> filterByTags(const std::set<juce::String>& tags) const;

    /**
     * Get library name
     */
    const juce::String& getName() const { return m_name; }

    /**
     * Get sample count
     */
    int getSampleCount() const { return static_cast<int>(m_samples.size()); }

    /**
     * Get all categories in library
     */
    std::set<juce::String> getAllCategories() const;

    /**
     * Get all tags in library
     */
    std::set<juce::String> getAllTags() const;

private:
    juce::String m_name;
    std::unordered_map<juce::String, std::shared_ptr<Sample>> m_samples;
    mutable juce::CriticalSection m_lock;
};

/**
 * @class SampleManager
 * @brief Main sample management system
 * 
 * Manages multiple sample libraries with:
 * - Async loading and scanning
 * - Automatic analysis (BPM, key, waveform)
 * - Memory management (LRU cache)
 * - Import/export functionality
 * - Thumbnail generation
 */
class SampleManager {
public:
    /**
     * Scan status callback
     */
    using ScanProgressCallback = std::function<void(int current, int total, const juce::String& fileName)>;

    SampleManager();
    ~SampleManager();

    /**
     * Initialize sample manager
     * @param maxMemoryMB Maximum memory for loaded samples (MB)
     */
    void initialize(int maxMemoryMB = 500);

    /**
     * Scan directory for audio files
     * @param directory Directory to scan
     * @param recursive Scan subdirectories
     * @param progressCallback Progress callback (optional)
     * @return Number of samples found
     */
    int scanDirectory(const juce::File& directory, 
                     bool recursive = true,
                     ScanProgressCallback progressCallback = nullptr);

    /**
     * Import single file
     * @param file Audio file to import
     * @param category Category to assign
     * @param autoAnalyze Automatically analyze BPM/key
     * @return UUID of imported sample (empty on failure)
     */
    juce::String importFile(const juce::File& file,
                           const juce::String& category = "Uncategorized",
                           bool autoAnalyze = true);

    /**
     * Create new library
     * @param name Library name
     * @return Pointer to created library
     */
    SampleLibrary* createLibrary(const juce::String& name);

    /**
     * Get library by name
     */
    SampleLibrary* getLibrary(const juce::String& name) const;

    /**
     * Get all libraries
     */
    std::vector<SampleLibrary*> getAllLibraries() const;

    /**
     * Remove library
     * @param name Library name
     * @return True if removed
     */
    bool removeLibrary(const juce::String& name);

    /**
     * Get sample by UUID (searches all libraries)
     */
    std::shared_ptr<Sample> getSample(const juce::String& uuid) const;

    /**
     * Load sample into memory
     * @param uuid Sample UUID
     * @return True if loaded successfully
     */
    bool loadSample(const juce::String& uuid);

    /**
     * Unload sample from memory
     * @param uuid Sample UUID
     */
    void unloadSample(const juce::String& uuid);

    /**
     * Preload samples (async)
     * @param uuids List of sample UUIDs to preload
     */
    void preloadSamples(const std::vector<juce::String>& uuids);

    /**
     * Global search across all libraries
     * @param query Search query
     * @return Matching samples
     */
    std::vector<std::shared_ptr<Sample>> globalSearch(const juce::String& query) const;

    /**
     * Analyze sample (BPM, key detection)
     * @param uuid Sample UUID
     * @return True if analysis successful
     */
    bool analyzeSample(const juce::String& uuid);

    /**
     * Generate audio thumbnail
     * @param uuid Sample UUID
     * @param width Thumbnail width in pixels
     * @param height Thumbnail height in pixels
     * @return Thumbnail image
     */
    juce::Image generateThumbnail(const juce::String& uuid, int width, int height);

    /**
     * Save library database
     * @param databaseFile Database file path
     * @return True if saved successfully
     */
    bool saveDatabase(const juce::File& databaseFile);

    /**
     * Load library database
     * @param databaseFile Database file path
     * @return True if loaded successfully
     */
    bool loadDatabase(const juce::File& databaseFile);

    /**
     * Get total sample count across all libraries
     */
    int getTotalSampleCount() const;

    /**
     * Get loaded sample count
     */
    int getLoadedSampleCount() const;

    /**
     * Get memory usage in MB
     */
    float getMemoryUsageMB() const;

    /**
     * Clear memory cache (unload all samples)
     */
    void clearCache();

    /**
     * Set auto-analysis enabled
     */
    void setAutoAnalysisEnabled(bool enabled) { m_autoAnalysis = enabled; }

    /**
     * Check if auto-analysis is enabled
     */
    bool isAutoAnalysisEnabled() const { return m_autoAnalysis; }

private:
    // Helper functions
    bool isAudioFile(const juce::File& file) const;
    SampleMetadata extractMetadata(const juce::File& file);
    juce::String generateUUID() const;
    bool detectBPM(Sample* sample);
    bool detectKey(Sample* sample);
    void manageCacheSize();

    // Libraries
    std::unordered_map<juce::String, std::unique_ptr<SampleLibrary>> m_libraries;

    // Audio format manager
    juce::AudioFormatManager m_formatManager;

    // Memory management
    int m_maxMemoryMB = 500;
    std::atomic<int> m_currentMemoryUsage{0};
    std::vector<juce::String> m_lruCache; // LRU cache for loaded samples

    // Settings
    bool m_autoAnalysis = true;

    // Thread safety
    mutable juce::CriticalSection m_lock;

    // Background thread for async operations
    std::unique_ptr<juce::ThreadPool> m_threadPool;

    // Thumbnail cache
    std::unordered_map<juce::String, juce::Image> m_thumbnailCache;
    mutable juce::CriticalSection m_thumbnailLock;
};

/**
 * @class BPMDetector
 * @brief Automatic BPM detection using onset detection
 */
class BPMDetector {
public:
    BPMDetector();
    ~BPMDetector() = default;

    /**
     * Detect BPM from audio buffer
     * @param buffer Audio buffer
     * @param sampleRate Sample rate
     * @return Detected BPM (0.0 if detection failed)
     */
    float detectBPM(const juce::AudioBuffer<float>& buffer, double sampleRate);

private:
    void detectOnsets(const std::vector<float>& energy);
    float calculateBPMFromOnsets();
    
    std::vector<float> m_onsets;
    static constexpr float kMinBPM = 60.0f;
    static constexpr float kMaxBPM = 200.0f;
};

/**
 * @class KeyDetector
 * @brief Automatic musical key detection using chroma features
 */
class KeyDetector {
public:
    KeyDetector();
    ~KeyDetector() = default;

    /**
     * Detect musical key from audio buffer
     * @param buffer Audio buffer
     * @param sampleRate Sample rate
     * @return Key number (0=C, 1=C#, etc.) or -1 if failed
     */
    int detectKey(const juce::AudioBuffer<float>& buffer, double sampleRate);

    /**
     * Get key name from key number
     * @param keyNumber Key number (0-11)
     * @return Key name (e.g., "C", "F#")
     */
    static juce::String getKeyName(int keyNumber);

private:
    void calculateChroma(const juce::AudioBuffer<float>& buffer, double sampleRate);
    int findBestKeyMatch() const;
    
    std::array<float, 12> m_chromaProfile;
    
    // Major key profiles
    static constexpr std::array<float, 12> kMajorProfile = {
        6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f,
        2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f
    };
};

} // namespace omega
