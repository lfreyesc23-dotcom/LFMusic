#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <memory>
#include <string>

namespace omega {

/**
 * @brief Sample metadata for organization and search
 */
struct SampleMetadata {
    juce::String name;
    juce::String category;      // "808", "Kick", "Snare", "HiHat", "Clap", "Perc", etc.
    juce::String genre;         // "Trap", "Drill", "House", "Techno", "Hip-Hop", etc.
    juce::String mood;          // "Dark", "Bright", "Aggressive", "Smooth", etc.
    juce::String key;           // "C", "C#", "D", etc. (for tonal samples)
    int bpm;                    // Original BPM (0 if not applicable)
    float duration;             // Duration in seconds
    bool isTonal;               // true for 808s, false for drums
    int rootNote;               // MIDI note number for tonal samples (60 = C3)
    juce::StringArray tags;     // Additional searchable tags
    juce::File filePath;
    
    SampleMetadata() 
        : bpm(0), duration(0.0f), isTonal(false), rootNote(60) {}
};

/**
 * @brief Loaded sample data with audio buffer
 */
struct LoadedSample {
    std::unique_ptr<juce::AudioBuffer<float>> buffer;
    SampleMetadata metadata;
    double sampleRate;
    bool isLoaded;
    
    LoadedSample() : sampleRate(44100.0), isLoaded(false) {}
};

/**
 * @brief Content library manager for samples, loops, and presets
 */
class ContentLibrary {
public:
    ContentLibrary();
    ~ContentLibrary();
    
    // Library management
    void initialize(const juce::File& libraryRoot);
    void scanLibrary();
    void refreshLibrary();
    
    // Sample loading
    std::shared_ptr<LoadedSample> loadSample(const juce::String& sampleId);
    std::shared_ptr<LoadedSample> loadSampleByPath(const juce::File& path);
    void preloadCategory(const juce::String& category);
    void unloadAll();
    
    // Search and filtering
    std::vector<SampleMetadata> searchSamples(
        const juce::String& query,
        const juce::String& category = "",
        const juce::String& genre = "",
        int minBpm = 0,
        int maxBpm = 999
    );
    
    std::vector<SampleMetadata> getSamplesByCategory(const juce::String& category);
    std::vector<SampleMetadata> getSamplesByGenre(const juce::String& genre);
    std::vector<SampleMetadata> getSamplesByKey(const juce::String& key);
    std::vector<SampleMetadata> getSamplesByTag(const juce::String& tag);
    
    // Metadata
    juce::StringArray getCategories() const;
    juce::StringArray getGenres() const;
    juce::StringArray getTags() const;
    int getTotalSampleCount() const;
    
    // Favorites and collections
    void addToFavorites(const juce::String& sampleId);
    void removeFromFavorites(const juce::String& sampleId);
    bool isFavorite(const juce::String& sampleId) const;
    std::vector<SampleMetadata> getFavorites() const;
    
    // Collections (user-created groups)
    void createCollection(const juce::String& name);
    void deleteCollection(const juce::String& name);
    void addToCollection(const juce::String& collectionName, const juce::String& sampleId);
    void removeFromCollection(const juce::String& collectionName, const juce::String& sampleId);
    std::vector<juce::String> getCollectionNames() const;
    std::vector<SampleMetadata> getCollectionSamples(const juce::String& name) const;
    
    // Preview playback
    void setPreviewPlayer(juce::AudioDeviceManager* deviceManager);
    void previewSample(const juce::String& sampleId);
    void stopPreview();
    bool isPreviewPlaying() const;
    
private:
    juce::File libraryRoot_;
    std::map<juce::String, SampleMetadata> sampleDatabase_;
    std::map<juce::String, std::shared_ptr<LoadedSample>> loadedSamples_;
    
    juce::StringArray categories_;
    juce::StringArray genres_;
    juce::StringArray allTags_;
    
    std::set<juce::String> favorites_;
    std::map<juce::String, std::vector<juce::String>> collections_;
    
    // Preview player
    std::unique_ptr<juce::AudioSourcePlayer> previewPlayer_;
    std::unique_ptr<juce::AudioTransportSource> previewTransport_;
    juce::AudioDeviceManager* deviceManager_;
    
    // Helper methods
    void scanDirectory(const juce::File& directory, const juce::String& category);
    SampleMetadata extractMetadata(const juce::File& file);
    juce::String generateSampleId(const juce::File& file);
    void updateCategoriesAndTags();
    bool matchesSearchQuery(const SampleMetadata& metadata, const juce::String& query);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContentLibrary)
};

/**
 * @brief Built-in sample library with embedded 808s and drums
 */
class BuiltInSampleLibrary {
public:
    static void initializeBuiltInSamples(ContentLibrary& library);
    
    // Generate synthetic 808s
    static std::unique_ptr<juce::AudioBuffer<float>> generate808(
        float frequency,        // Hz (e.g., 55.0 for A1, 82.4 for E2)
        float decay,            // seconds (0.5-3.0)
        float punch,            // 0.0-1.0
        float distortion,       // 0.0-1.0
        double sampleRate
    );
    
    // Generate synthetic kick
    static std::unique_ptr<juce::AudioBuffer<float>> generateKick(
        float pitch,            // 40-80 Hz
        float punchAmount,      // 0.0-1.0
        float decayTime,        // 0.05-0.5 seconds
        double sampleRate
    );
    
    // Generate synthetic snare
    static std::unique_ptr<juce::AudioBuffer<float>> generateSnare(
        float tone,             // 150-300 Hz
        float snap,             // 0.0-1.0 (noise amount)
        float decay,            // 0.05-0.3 seconds
        double sampleRate
    );
    
    // Generate synthetic hi-hat
    static std::unique_ptr<juce::AudioBuffer<float>> generateHiHat(
        bool isOpen,
        float brightness,       // 0.0-1.0
        float decay,            // 0.05-1.0 seconds
        double sampleRate
    );
    
    // Generate clap
    static std::unique_ptr<juce::AudioBuffer<float>> generateClap(
        float spread,           // 0-30ms between layers
        int layers,             // 2-5 layers
        double sampleRate
    );
    
private:
    // Synthesis helpers
    static float envelope(float time, float attack, float decay, float sustain, float release);
    static float generateNoise();
    static void applyDistortion(juce::AudioBuffer<float>& buffer, float amount);
};

} // namespace omega
