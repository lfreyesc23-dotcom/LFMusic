/*
  ==============================================================================
    PluginManager.h
    
    Sistema profesional de hosting de plugins VST3/AU:
    - Plugin scanning y discovery
    - Plugin loading (VST3/AU/VST)
    - Parameter automation
    - Plugin delay compensation (PDC)
    - Preset management
    - GUI hosting
    
    Enterprise-grade plugin host compatible con toda la industria
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <map>

namespace OmegaStudio {

//==============================================================================
/** Descripción de un plugin descubierto */
struct PluginDescription {
    juce::String name;
    juce::String manufacturerName;
    juce::String version;
    juce::String category;
    juce::String pluginFormatName; // "VST3", "AudioUnit", "VST"
    juce::String fileOrIdentifier;
    juce::String uid; // Unique ID
    
    bool isInstrument { false };
    int numInputChannels { 2 };
    int numOutputChannels { 2 };
    
    // JUCE native description
    juce::PluginDescription juceDescription;
    
    juce::var toVar() const;
    static PluginDescription fromVar(const juce::var& v);
};

//==============================================================================
/** Estado de un plugin instanciado */
struct PluginState {
    juce::String pluginUID;
    juce::MemoryBlock stateData; // Binary state del plugin
    std::map<int, float> parameterValues; // Backup de parámetros
    bool bypassed { false };
    
    juce::var toVar() const;
    static PluginState fromVar(const juce::var& v);
};

//==============================================================================
/** Wrapper para un plugin cargado */
class PluginInstance {
public:
    PluginInstance(std::unique_ptr<juce::AudioPluginInstance> instance);
    ~PluginInstance();
    
    // Access
    juce::AudioPluginInstance* getPlugin() { return plugin.get(); }
    const juce::AudioPluginInstance* getPlugin() const { return plugin.get(); }
    
    // Info
    juce::String getName() const;
    juce::String getUID() const;
    int getNumParameters() const;
    juce::String getParameterName(int index) const;
    float getParameter(int index) const;
    void setParameter(int index, float value);
    
    // State
    PluginState getState() const;
    void setState(const PluginState& state);
    
    // Latency
    int getLatencySamples() const;
    
    // Bypass
    bool isBypassed() const { return bypassed; }
    void setBypassed(bool shouldBeBypassed) { bypassed = shouldBeBypassed; }
    
    // Processing (RT-safe)
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    
    // GUI
    bool hasEditor() const;
    juce::AudioProcessorEditor* createEditor();
    
private:
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    bool bypassed { false };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginInstance)
};

//==============================================================================
/** Manager de plugins - Singleton */
class PluginManager {
public:
    static PluginManager& getInstance();
    
    // Scanning
    void scanForPlugins(bool rescan = false);
    bool isScanning() const { return scanning; }
    float getScanProgress() const { return scanProgress; }
    
    // Discovery
    std::vector<PluginDescription> getAvailablePlugins() const;
    std::vector<PluginDescription> getInstruments() const;
    std::vector<PluginDescription> getEffects() const;
    std::vector<PluginDescription> searchPlugins(const juce::String& query) const;
    
    // Loading
    std::unique_ptr<PluginInstance> loadPlugin(const juce::String& uid);
    std::unique_ptr<PluginInstance> loadPlugin(const PluginDescription& desc);
    
    // Formats
    void addFormat(std::unique_ptr<juce::AudioPluginFormat> format);
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }
    
    // Known plugins list (cache)
    juce::KnownPluginList& getKnownPluginList() { return knownPluginList; }
    const juce::KnownPluginList& getKnownPluginList() const { return knownPluginList; }
    
    // Blacklist (plugins problemáticos)
    void addToBlacklist(const juce::String& uid);
    bool isBlacklisted(const juce::String& uid) const;
    
    // Listeners
    struct Listener {
        virtual ~Listener() = default;
        virtual void pluginScanStarted() {}
        virtual void pluginScanFinished() {}
        virtual void pluginListChanged() {}
    };
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
    // Stats
    int getLoadedPluginCount() const;
    int getScannedPluginCount() const { return knownPluginList.getNumTypes(); }
    
private:
    PluginManager();
    ~PluginManager();
    
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPluginList;
    
    std::atomic<bool> scanning { false };
    std::atomic<float> scanProgress { 0.0f };
    
    juce::StringArray blacklist;
    juce::ListenerList<Listener> listeners;
    
    void scanDirectory(juce::AudioPluginFormat& format, const juce::String& dir);
    void saveKnownPluginList();
    void loadKnownPluginList();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginManager)
};

//==============================================================================
/** Plugin Delay Compensation (PDC) Manager */
class PluginDelayCompensation {
public:
    PluginDelayCompensation();
    
    // Registrar plugins y sus latencias
    void registerPlugin(const juce::String& pluginUID, int latencySamples);
    void unregisterPlugin(const juce::String& pluginUID);
    
    // Calcular compensación total
    int getMaxLatency() const { return maxLatency; }
    int getCompensationFor(const juce::String& pluginUID) const;
    
    // Recompute cuando cambia algo
    void recomputeCompensation();
    
private:
    std::map<juce::String, int> pluginLatencies;
    int maxLatency { 0 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginDelayCompensation)
};

//==============================================================================
/** Plugin Chain - Cadena de plugins en un track */
class PluginChain {
public:
    PluginChain();
    ~PluginChain();
    
    // Gestión de plugins
    void addPlugin(std::unique_ptr<PluginInstance> plugin);
    void insertPlugin(int index, std::unique_ptr<PluginInstance> plugin);
    void removePlugin(int index);
    void movePlugin(int fromIndex, int toIndex);
    void clearPlugins();
    
    // Access
    int getNumPlugins() const { return static_cast<int>(plugins.size()); }
    PluginInstance* getPlugin(int index);
    const PluginInstance* getPlugin(int index) const;
    
    // Processing (RT-safe)
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);
    
    // Latency
    int getTotalLatency() const;
    
    // State
    juce::var getState() const;
    void setState(const juce::var& state);
    
    // Prepare
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock);
    void releaseResources();
    
private:
    std::vector<std::unique_ptr<PluginInstance>> plugins;
    double sampleRate { 44100.0 };
    int blockSize { 512 };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginChain)
};

//==============================================================================
/** Preset Manager - Gestión de presets de plugins */
class PluginPresetManager {
public:
    struct Preset {
        juce::String name;
        juce::String author;
        juce::String category;
        juce::String pluginUID;
        PluginState state;
        
        juce::var toVar() const;
        static Preset fromVar(const juce::var& v);
    };
    
    PluginPresetManager();
    
    // Presets
    void savePreset(const juce::String& pluginUID, const Preset& preset);
    std::vector<Preset> getPresetsFor(const juce::String& pluginUID) const;
    bool loadPreset(PluginInstance& plugin, const juce::String& presetName);
    bool deletePreset(const juce::String& pluginUID, const juce::String& presetName);
    
    // User preset directory
    juce::File getPresetDirectory() const;
    
private:
    juce::File presetDirectory;
    std::map<juce::String, std::vector<Preset>> presetCache;
    
    void loadPresetCache();
    void savePresetCache();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginPresetManager)
};

} // namespace OmegaStudio
