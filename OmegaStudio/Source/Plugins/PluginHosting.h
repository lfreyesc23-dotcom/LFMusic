#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>

namespace OmegaStudio {

/**
 * @brief Plugin Format Support (FL Studio 2025)
 * VST2, VST3, Audio Unit, CLAP
 */
class PluginHosting
{
public:
    struct PluginInfo {
        juce::String name;
        juce::String manufacturer;
        juce::String version;
        juce::String category;
        juce::String uniqueId;
        juce::String pluginFormatName;  // VST, VST3, AU, CLAP
        bool isInstrument = false;
        bool hasEditor = false;
        int numInputs = 0;
        int numOutputs = 0;
        juce::File file;
    };
    
    PluginHosting();
    ~PluginHosting();
    
    // Plugin scanning
    void scanForPlugins();
    void scanDirectory(const juce::File& directory, bool recursive = true);
    void addPluginSearchPath(const juce::File& path);
    void removePluginSearchPath(const juce::File& path);
    std::vector<juce::File> getPluginSearchPaths() const;
    
    // Plugin list
    std::vector<PluginInfo> getAllPlugins() const;
    std::vector<PluginInfo> getInstruments() const;
    std::vector<PluginInfo> getEffects() const;
    std::vector<PluginInfo> getPluginsByManufacturer(const juce::String& manufacturer) const;
    std::vector<PluginInfo> getPluginsByCategory(const juce::String& category) const;
    
    // Search
    std::vector<PluginInfo> searchPlugins(const juce::String& query) const;
    
    // Plugin loading
    std::unique_ptr<juce::AudioPluginInstance> loadPlugin(const PluginInfo& info);
    std::unique_ptr<juce::AudioPluginInstance> loadPluginByName(const juce::String& name);
    
    // Blacklist (problematic plugins)
    void addToBlacklist(const juce::String& pluginId);
    void removeFromBlacklist(const juce::String& pluginId);
    bool isBlacklisted(const juce::String& pluginId) const;
    void clearBlacklist();
    
    // Favorites
    void addToFavorites(const juce::String& pluginId);
    void removeFromFavorites(const juce::String& pluginId);
    std::vector<PluginInfo> getFavorites() const;
    bool isFavorite(const juce::String& pluginId) const;
    
    // Recent plugins
    void addToRecent(const juce::String& pluginId);
    std::vector<PluginInfo> getRecentPlugins(int count = 20) const;
    
    // Plugin state
    juce::MemoryBlock savePluginState(juce::AudioPluginInstance* plugin);
    bool loadPluginState(juce::AudioPluginInstance* plugin, const juce::MemoryBlock& state);
    
    // Format managers
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }
    
    // Scan progress
    std::function<void(int current, int total, const juce::String& pluginName)> onScanProgress;
    std::function<void(bool success, int pluginsFound)> onScanComplete;
    
    // Plugin validation
    bool validatePlugin(const PluginInfo& info);
    
    // Database
    void savePluginDatabase();
    void loadPluginDatabase();

private:
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    
    std::vector<juce::File> searchPaths;
    std::vector<juce::String> blacklist;
    std::vector<juce::String> favorites;
    std::vector<juce::String> recentPlugins;
    
    juce::File databaseFile;
    
    void initializeFormatManager();
    PluginInfo createPluginInfo(const juce::PluginDescription& desc);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHosting)
};

/**
 * @brief Plugin Wrapper - Wraps AudioPluginInstance with additional features
 */
class PluginWrapper
{
public:
    PluginWrapper(std::unique_ptr<juce::AudioPluginInstance> plugin);
    ~PluginWrapper();
    
    juce::AudioPluginInstance* getPlugin() { return plugin.get(); }
    
    // Processing
    void prepareToPlay(double sampleRate, int blockSize);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void releaseResources();
    
    // Bypass
    void setBypass(bool bypass) { isBypassed = bypass; }
    bool getBypass() const { return isBypassed; }
    
    // Dry/Wet mix
    void setDryWet(float mix) { dryWetMix = juce::jlimit(0.0f, 1.0f, mix); }
    float getDryWet() const { return dryWetMix; }
    
    // Latency compensation
    int getLatencySamples() const { return plugin->getLatencySamples(); }
    void setLatencyCompensation(bool enable) { compensateLatency = enable; }
    
    // CPU metering
    float getCPUUsage() const { return cpuUsage; }
    
    // Editor
    bool hasEditor() const { return plugin->hasEditor(); }
    juce::AudioProcessorEditor* createEditor() { return plugin->createEditor(); }
    
    // Parameters
    int getNumParameters() const { return plugin->getNumParameters(); }
    float getParameter(int index) const { return plugin->getParameter(index); }
    void setParameter(int index, float value);
    juce::String getParameterName(int index) const { return plugin->getParameterName(index); }
    
    // Presets
    void loadPreset(const juce::File& file);
    void savePreset(const juce::File& file);
    int getCurrentProgram() const { return plugin->getCurrentProgram(); }
    void setCurrentProgram(int program);
    juce::String getProgramName(int program) const { return plugin->getProgramName(program); }
    
    // Automation
    void beginParameterChangeGesture(int index);
    void endParameterChangeGesture(int index);

private:
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    
    bool isBypassed = false;
    float dryWetMix = 1.0f;
    bool compensateLatency = true;
    float cpuUsage = 0.0f;
    
    juce::AudioBuffer<float> dryBuffer;
    juce::Time processingStartTime;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWrapper)
};

/**
 * @brief Plugin Chain - Multiple plugins in series
 */
class PluginChain
{
public:
    PluginChain();
    ~PluginChain();
    
    // Plugin management
    void addPlugin(std::unique_ptr<PluginWrapper> plugin);
    void insertPlugin(int index, std::unique_ptr<PluginWrapper> plugin);
    void removePlugin(int index);
    void clearPlugins();
    void movePlugin(int fromIndex, int toIndex);
    
    int getNumPlugins() const { return static_cast<int>(plugins.size()); }
    PluginWrapper* getPlugin(int index);
    
    // Processing
    void prepareToPlay(double sampleRate, int blockSize);
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    void releaseResources();
    
    // State
    juce::ValueTree serialize() const;
    void deserialize(const juce::ValueTree& tree, PluginHosting& hosting);
    
    // CPU monitoring
    float getTotalCPUUsage() const;

private:
    std::vector<std::unique_ptr<PluginWrapper>> plugins;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChain)
};

/**
 * @brief Plugin Browser Component
 */
class PluginBrowserComponent : public juce::Component,
                               public juce::TextEditor::Listener
{
public:
    PluginBrowserComponent(PluginHosting& hosting);
    ~PluginBrowserComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Callbacks
    std::function<void(const PluginHosting::PluginInfo&)> onPluginSelected;
    std::function<void(const PluginHosting::PluginInfo&)> onPluginDoubleClicked;

private:
    PluginHosting& hosting;
    
    juce::TextEditor searchBox;
    juce::ComboBox typeFilter;  // All, Instruments, Effects
    juce::<ComboBox manufacturerFilter;
    juce::ListBox pluginList;
    juce::TextButton scanButton;
    juce::TextButton favoriteButton;
    juce::Label infoLabel;
    
    std::vector<PluginHosting::PluginInfo> currentPlugins;
    
    void updatePluginList();
    void textEditorTextChanged(juce::TextEditor&) override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserComponent)
};

/**
 * @brief Preset Manager - Save/Load Plugin Presets
 */
class PresetManager
{
public:
    struct Preset {
        juce::String name;
        juce::String pluginName;
        juce::String author;
        juce::String category;
        juce::StringArray tags;
        juce::MemoryBlock data;
        juce::Time createdTime;
        juce::Time modifiedTime;
        bool favorite = false;
    };
    
    PresetManager();
    ~PresetManager();
    
    // Preset management
    void savePreset(const Preset& preset);
    bool loadPreset(const juce::String& name, Preset& preset);
    void deletePreset(const juce::String& name);
    void renamePreset(const juce::String& oldName, const juce::String& newName);
    
    // Search and filter
    std::vector<Preset> getAllPresets() const;
    std::vector<Preset> getPresetsForPlugin(const juce::String& pluginName) const;
    std::vector<Preset> getPresetsByCategory(const juce::String& category) const;
    std::vector<Preset> searchPresets(const juce::String& query) const;
    std::vector<Preset> getFavoritePresets() const;
    
    // Categories
    std::vector<juce::String> getCategories() const;
    void addCategory(const juce::String& category);
    
    // Import/Export
    bool importPreset(const juce::File& file);
    bool exportPreset(const juce::String& name, const juce::File& destination);
    bool importPresetPack(const juce::File& file);
    bool exportPresetPack(const std::vector<juce::String>& presetNames, const juce::File& destination);
    
    // Favorites
    void addToFavorites(const juce::String& name);
    void removeFromFavorites(const juce::String& name);
    
    // Database
    void setPresetDirectory(const juce::File& directory);
    juce::File getPresetDirectory() const { return presetDirectory; }
    void scanPresetDirectory();

private:
    juce::File presetDirectory;
    std::map<juce::String, Preset> presets;
    
    void loadPresetDatabase();
    void savePresetDatabase();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};

} // namespace OmegaStudio
