//==============================================================================
// PluginSystemAdvanced.h - Sistema Avanzado de Plugins
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <vector>
#include <map>
#include <memory>

namespace OmegaStudio {

//==============================================================================
/** Plugin Delay Compensation - Compensación automática de latencia */
class PluginDelayCompensation {
public:
    PluginDelayCompensation();
    ~PluginDelayCompensation() = default;
    
    // Setup
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    
    // Register plugins con su latencia
    void registerPlugin(int pluginId, int latencySamples);
    void unregisterPlugin(int pluginId);
    void updatePluginLatency(int pluginId, int latencySamples);
    
    // Calcular compensación global
    int calculateGlobalLatency() const;
    
    // Get compensation delay para un plugin específico
    int getCompensationDelay(int pluginId) const;
    
    // Apply compensation a un buffer
    void applyCompensation(int pluginId, juce::AudioBuffer<float>& buffer);
    
    // Reporte de latencia
    struct LatencyReport {
        int maxLatency = 0;
        std::map<int, int> pluginLatencies;  // pluginId -> samples
        std::map<int, int> compensationDelays;  // pluginId -> compensation
    };
    
    LatencyReport generateReport() const;
    
private:
    //==========================================================================
    std::map<int, int> pluginLatencies_;  // pluginId -> latency samples
    std::map<int, juce::AudioBuffer<float>> delayBuffers_;  // Delay lines
    
    double sampleRate_ = 48000.0;
    int samplesPerBlock_ = 512;
    
    void updateDelayBuffers();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginDelayCompensation)
};

//==============================================================================
/** Plugin State Manager - Save/Load states individuales */
class PluginStateManager {
public:
    //==========================================================================
    struct PluginState {
        int pluginId;
        juce::String pluginName;
        juce::String vendorName;
        juce::MemoryBlock stateData;
        juce::Time timestamp;
        juce::String description;
    };
    
    //==========================================================================
    PluginStateManager();
    ~PluginStateManager() = default;
    
    // Save/Load states
    bool saveState(int pluginId, const juce::MemoryBlock& state, const juce::String& description = "");
    bool loadState(int pluginId, PluginState& outState);
    
    // State history (para undo/redo de plugin)
    void pushState(int pluginId, const juce::MemoryBlock& state);
    bool undoState(int pluginId, juce::MemoryBlock& outState);
    bool redoState(int pluginId, juce::MemoryBlock& outState);
    
    // Preset management
    bool saveAsPreset(int pluginId, const juce::String& presetName);
    bool loadPreset(int pluginId, const juce::String& presetName, juce::MemoryBlock& outState);
    
    juce::StringArray getPresetsForPlugin(int pluginId) const;
    bool deletePreset(int pluginId, const juce::String& presetName);
    
    // Compare states
    bool areStatesEqual(const juce::MemoryBlock& state1, const juce::MemoryBlock& state2) const;
    float calculateStateSimilarity(const juce::MemoryBlock& state1, const juce::MemoryBlock& state2) const;
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::map<int, std::vector<PluginState>> stateHistory_;  // pluginId -> history
    std::map<int, int> stateHistoryPosition_;  // Para undo/redo
    
    static constexpr int maxHistoryPerPlugin = 50;
    
    juce::File getPresetFile(int pluginId, const juce::String& presetName) const;
    juce::File getPresetDirectory(int pluginId) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginStateManager)
};

//==============================================================================
/** Plugin Browser - Búsqueda y organización de plugins */
class PluginBrowser {
public:
    //==========================================================================
    enum class PluginCategory {
        Synth,
        Sampler,
        Drum,
        Effect,
        Dynamics,
        EQ,
        Reverb,
        Delay,
        Modulation,
        Filter,
        Distortion,
        Utility,
        Analyzer,
        Other
    };
    
    struct PluginInfo {
        juce::String name;
        juce::String vendor;
        juce::String version;
        juce::String uniqueId;
        PluginCategory category;
        
        juce::PluginDescription description;
        
        bool isFavorite = false;
        int timesUsed = 0;
        juce::Time lastUsed;
        float rating = 0.0f;  // 0-5 stars
        
        juce::StringArray tags;
    };
    
    //==========================================================================
    PluginBrowser();
    ~PluginBrowser() = default;
    
    // Scan plugins
    void scanForPlugins();
    void scanDirectory(const juce::File& directory);
    void addPlugin(const PluginInfo& info);
    
    // Search
    std::vector<PluginInfo> search(const juce::String& query) const;
    std::vector<PluginInfo> filterByCategory(PluginCategory category) const;
    std::vector<PluginInfo> filterByVendor(const juce::String& vendor) const;
    std::vector<PluginInfo> filterByTags(const juce::StringArray& tags) const;
    
    // Favorites
    void setFavorite(const juce::String& uniqueId, bool favorite);
    std::vector<PluginInfo> getFavorites() const;
    
    // Recently used
    void recordUsage(const juce::String& uniqueId);
    std::vector<PluginInfo> getRecentlyUsed(int maxCount = 10) const;
    std::vector<PluginInfo> getMostUsed(int maxCount = 10) const;
    
    // Rating
    void setRating(const juce::String& uniqueId, float rating);
    
    // Tags
    void addTag(const juce::String& uniqueId, const juce::String& tag);
    void removeTag(const juce::String& uniqueId, const juce::String& tag);
    juce::StringArray getAllTags() const;
    
    // Get all plugins
    std::vector<PluginInfo> getAllPlugins() const;
    const PluginInfo* getPlugin(const juce::String& uniqueId) const;
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::map<juce::String, PluginInfo> plugins_;  // uniqueId -> info
    
    juce::AudioPluginFormatManager formatManager_;
    juce::KnownPluginList knownPluginList_;
    
    // Fuzzy search
    float calculateRelevance(const juce::String& query, const PluginInfo& plugin) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowser)
};

//==============================================================================
/** Sidechain Routing System - Routing visual de sidechain */
class SidechainRouter {
public:
    //==========================================================================
    struct SidechainConnection {
        int sourceTrackIndex = -1;      // Pista fuente
        int destPluginId = -1;          // Plugin destino
        int destTrackIndex = -1;        // Pista del plugin destino
        
        bool enabled = true;
        float amount = 1.0f;            // 0.0 - 1.0
        
        // Filters (opcional)
        bool useHighPassFilter = false;
        float highPassFreq = 80.0f;
        
        bool useLowPassFilter = false;
        float lowPassFreq = 5000.0f;
    };
    
    //==========================================================================
    SidechainRouter();
    ~SidechainRouter() = default;
    
    // Connection management
    int addConnection(const SidechainConnection& connection);
    void removeConnection(int connectionId);
    void clearConnections();
    
    int getNumConnections() const { return static_cast<int>(connections_.size()); }
    const SidechainConnection& getConnection(int index) const { return connections_[index]; }
    
    // Get connections para un plugin específico
    std::vector<SidechainConnection> getConnectionsForPlugin(int pluginId) const;
    std::vector<SidechainConnection> getConnectionsForTrack(int trackIndex) const;
    
    // Processing
    void process(const std::map<int, juce::AudioBuffer<float>*>& trackBuffers);
    
    // Enable/disable
    void setConnectionEnabled(int connectionId, bool enabled);
    void setConnectionAmount(int connectionId, float amount);
    
    // Serialization
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& tree);
    
private:
    //==========================================================================
    std::vector<SidechainConnection> connections_;
    int nextConnectionId_ = 1;
    
    // Filters
    struct FilterState {
        juce::dsp::IIR::Filter<float> highPass;
        juce::dsp::IIR::Filter<float> lowPass;
    };
    
    std::map<int, FilterState> connectionFilters_;  // connectionId -> filters
    
    void updateFilters(int connectionId);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainRouter)
};

//==============================================================================
/** Plugin Preset Browser - Browser integrado de presets */
class PluginPresetBrowser {
public:
    //==========================================================================
    struct PresetInfo {
        juce::String name;
        juce::String category;
        juce::String pluginId;
        juce::File file;
        
        juce::StringArray tags;
        bool isFavorite = false;
        float rating = 0.0f;
        
        juce::Time created;
        juce::Time modified;
    };
    
    //==========================================================================
    PluginPresetBrowser();
    ~PluginPresetBrowser() = default;
    
    // Scan presets
    void scanPresets(const juce::String& pluginId);
    void scanDirectory(const juce::File& directory, const juce::String& pluginId);
    
    // Search
    std::vector<PresetInfo> search(const juce::String& query, const juce::String& pluginId = "") const;
    std::vector<PresetInfo> filterByCategory(const juce::String& category) const;
    std::vector<PresetInfo> filterByTags(const juce::StringArray& tags) const;
    
    // Favorites
    void setFavorite(const juce::String& presetPath, bool favorite);
    std::vector<PresetInfo> getFavorites(const juce::String& pluginId = "") const;
    
    // Rating
    void setRating(const juce::String& presetPath, float rating);
    
    // Get all presets para un plugin
    std::vector<PresetInfo> getPresetsForPlugin(const juce::String& pluginId) const;
    
    // Tags
    void addTag(const juce::String& presetPath, const juce::String& tag);
    void removeTag(const juce::String& presetPath, const juce::String& tag);
    juce::StringArray getAllTags() const;
    
    // Categories
    juce::StringArray getAllCategories() const;
    
private:
    //==========================================================================
    std::map<juce::String, PresetInfo> presets_;  // preset path -> info
    
    float calculateRelevance(const juce::String& query, const PresetInfo& preset) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginPresetBrowser)
};

//==============================================================================
/** Latency Analysis - Análisis detallado de latencia por plugin */
class LatencyAnalyzer {
public:
    //==========================================================================
    struct PluginLatencyInfo {
        int pluginId;
        juce::String pluginName;
        
        int reportedLatency = 0;        // Latencia reportada por el plugin (samples)
        int measuredLatency = 0;        // Latencia medida (samples)
        bool accurate = false;          // ¿Reporte preciso?
        
        double latencyMs = 0.0;         // En milisegundos
        bool needsCompensation = false;
        
        // Por buffer size
        std::map<int, int> latencyByBufferSize;
    };
    
    //==========================================================================
    LatencyAnalyzer();
    ~LatencyAnalyzer() = default;
    
    // Measure latency
    PluginLatencyInfo measurePluginLatency(juce::AudioProcessor* plugin, 
                                           double sampleRate,
                                           int bufferSize);
    
    // Analyze all plugins
    std::vector<PluginLatencyInfo> analyzeAllPlugins(const std::vector<juce::AudioProcessor*>& plugins);
    
    // Get report
    juce::String generateLatencyReport(const std::vector<PluginLatencyInfo>& infos) const;
    
    // Visualización
    std::vector<float> getLatencyDistribution() const;
    
private:
    //==========================================================================
    // Test signal generation
    void generateImpulse(juce::AudioBuffer<float>& buffer);
    int detectImpulse(const juce::AudioBuffer<float>& buffer);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LatencyAnalyzer)
};

} // namespace OmegaStudio
