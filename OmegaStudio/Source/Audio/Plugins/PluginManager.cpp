/*
  ==============================================================================
    PluginManager.cpp
    Implementation del plugin host VST3/AU
  ==============================================================================
*/

#include "PluginManager.h"

namespace OmegaStudio {

//==============================================================================
// PluginDescription Implementation
//==============================================================================

juce::var PluginDescription::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    obj->setProperty("name", name);
    obj->setProperty("manufacturerName", manufacturerName);
    obj->setProperty("version", version);
    obj->setProperty("category", category);
    obj->setProperty("pluginFormatName", pluginFormatName);
    obj->setProperty("fileOrIdentifier", fileOrIdentifier);
    obj->setProperty("uid", uid);
    obj->setProperty("isInstrument", isInstrument);
    obj->setProperty("numInputChannels", numInputChannels);
    obj->setProperty("numOutputChannels", numOutputChannels);
    
    // Serialize JUCE description
    juce::MemoryOutputStream stream;
    if (auto xml = juceDescription.createXml())
        xml->writeTo(stream, {});
    obj->setProperty("juceDescription", stream.toUTF8());
    
    return juce::var(obj.get());
}

PluginDescription PluginDescription::fromVar(const juce::var& v) {
    PluginDescription desc;
    
    if (auto* obj = v.getDynamicObject()) {
        desc.name = obj->getProperty("name").toString();
        desc.manufacturerName = obj->getProperty("manufacturerName").toString();
        desc.version = obj->getProperty("version").toString();
        desc.category = obj->getProperty("category").toString();
        desc.pluginFormatName = obj->getProperty("pluginFormatName").toString();
        desc.fileOrIdentifier = obj->getProperty("fileOrIdentifier").toString();
        desc.uid = obj->getProperty("uid").toString();
        desc.isInstrument = obj->getProperty("isInstrument");
        desc.numInputChannels = obj->getProperty("numInputChannels");
        desc.numOutputChannels = obj->getProperty("numOutputChannels");
        
        // Deserialize JUCE description
        juce::MemoryBlock block;
        block.fromBase64Encoding(obj->getProperty("juceDescription").toString());
        if (auto xml = juce::parseXML(block.toString()))
            desc.juceDescription.loadFromXml(*xml);
    }
    
    return desc;
}

//==============================================================================
// PluginState Implementation
//==============================================================================

juce::var PluginState::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    
    obj->setProperty("pluginUID", pluginUID);
    obj->setProperty("stateData", stateData.toBase64Encoding());
    obj->setProperty("bypassed", bypassed);
    
    juce::DynamicObject::Ptr params = new juce::DynamicObject();
    for (const auto& [index, value] : parameterValues)
        params->setProperty(juce::String(index), value);
    obj->setProperty("parameters", juce::var(params.get()));
    
    return juce::var(obj.get());
}

PluginState PluginState::fromVar(const juce::var& v) {
    PluginState state;
    
    if (auto* obj = v.getDynamicObject()) {
        state.pluginUID = obj->getProperty("pluginUID").toString();
        state.stateData.fromBase64Encoding(obj->getProperty("stateData").toString());
        state.bypassed = obj->getProperty("bypassed");
        
        if (auto* params = obj->getProperty("parameters").getDynamicObject()) {
            for (const auto& prop : params->getProperties()) {
                int index = prop.name.toString().getIntValue();
                float value = prop.value;
                state.parameterValues[index] = value;
            }
        }
    }
    
    return state;
}

//==============================================================================
// PluginInstance Implementation
//==============================================================================

PluginInstance::PluginInstance(std::unique_ptr<juce::AudioPluginInstance> instance)
    : plugin(std::move(instance)) {}

PluginInstance::~PluginInstance() = default;

juce::String PluginInstance::getName() const {
    return plugin ? plugin->getName() : "";
}

juce::String PluginInstance::getUID() const {
    if (!plugin) return {};
    return plugin->getPluginDescription().createIdentifierString();
}

int PluginInstance::getNumParameters() const {
    return plugin ? plugin->getParameters().size() : 0;
}

juce::String PluginInstance::getParameterName(int index) const {
    if (!plugin || index < 0 || index >= getNumParameters())
        return {};
    return plugin->getParameterName(index);
}

float PluginInstance::getParameter(int index) const {
    if (!plugin || index < 0 || index >= getNumParameters())
        return 0.0f;
    return plugin->getParameters()[index]->getValue();
}

void PluginInstance::setParameter(int index, float value) {
    if (!plugin || index < 0 || index >= getNumParameters())
        return;
    plugin->getParameters()[index]->setValue(value);
}

PluginState PluginInstance::getState() const {
    PluginState state;
    
    if (!plugin) return state;
    
    state.pluginUID = getUID();
    state.bypassed = bypassed;
    
    // Get binary state
    plugin->getStateInformation(state.stateData);
    
    // Backup parameters
    for (int i = 0; i < getNumParameters(); ++i)
        state.parameterValues[i] = getParameter(i);
    
    return state;
}

void PluginInstance::setState(const PluginState& state) {
    if (!plugin) return;
    
    bypassed = state.bypassed;
    
    // Set binary state
    plugin->setStateInformation(state.stateData.getData(), 
                                static_cast<int>(state.stateData.getSize()));
    
    // Restore parameters (fallback)
    for (const auto& [index, value] : state.parameterValues)
        setParameter(index, value);
}

int PluginInstance::getLatencySamples() const {
    return plugin ? plugin->getLatencySamples() : 0;
}

void PluginInstance::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    if (!plugin || bypassed) return;
    
    plugin->processBlock(buffer, midiMessages);
}

bool PluginInstance::hasEditor() const {
    return plugin && plugin->hasEditor();
}

juce::AudioProcessorEditor* PluginInstance::createEditor() {
    return plugin ? plugin->createEditorIfNeeded() : nullptr;
}

//==============================================================================
// PluginManager Implementation
//==============================================================================

PluginManager& PluginManager::getInstance() {
    static PluginManager instance;
    return instance;
}

PluginManager::PluginManager() {
    // Add default formats
    formatManager.addDefaultFormats();
    
    // Load conocidos
    loadKnownPluginList();
}

PluginManager::~PluginManager() {
    saveKnownPluginList();
}

void PluginManager::scanForPlugins(bool rescan) {
    if (scanning) return;
    
    scanning = true;
    scanProgress = 0.0f;
    
    listeners.call([](Listener& l) { l.pluginScanStarted(); });
    
    // Scan en background thread
    juce::Thread::launch([this, rescan]() {
        if (rescan)
            knownPluginList.clear();
        
        // Get formats - copy to avoid binding to temporary
        auto formats = formatManager.getFormats();
        float progressPerFormat = 1.0f / formats.size();
        
        for (int i = 0; i < formats.size(); ++i) {
            auto* format = formats.getUnchecked(i);
            
            // Get default locations - copy to avoid binding to temporary
            juce::FileSearchPath searchPaths = format->getDefaultLocationsToSearch();
            
            for (int pathIdx = 0; pathIdx < searchPaths.getNumPaths(); ++pathIdx) {
                juce::File pathFile = searchPaths[pathIdx];
                juce::PluginDirectoryScanner scanner(
                    knownPluginList, *format, 
                    juce::FileSearchPath(pathFile.getFullPathName()),
                    true, juce::File(), true
                );
                
                juce::String pluginBeingScanned;
                while (scanner.scanNextFile(true, pluginBeingScanned)) {
                    // Update progress
                    float formatProgress = scanner.getProgress();
                    scanProgress = (i + formatProgress) * progressPerFormat;
                }
            }
        }
        
        scanProgress = 1.0f;
        scanning = false;
        
        saveKnownPluginList();
        
        listeners.call([](Listener& l) { 
            l.pluginScanFinished(); 
            l.pluginListChanged();
        });
    });
}

std::vector<PluginDescription> PluginManager::getAvailablePlugins() const {
    std::vector<PluginDescription> plugins;
    
    for (const auto& type : knownPluginList.getTypes()) {
        if (!isBlacklisted(type.createIdentifierString())) {
            PluginDescription desc;
            desc.juceDescription = type;
            desc.name = type.name;
            desc.manufacturerName = type.manufacturerName;
            desc.version = type.version;
            desc.category = type.category;
            desc.pluginFormatName = type.pluginFormatName;
            desc.fileOrIdentifier = type.fileOrIdentifier;
            desc.uid = type.createIdentifierString();
            desc.isInstrument = type.isInstrument;
            desc.numInputChannels = type.numInputChannels;
            desc.numOutputChannels = type.numOutputChannels;
            
            plugins.push_back(desc);
        }
    }
    
    return plugins;
}

std::vector<PluginDescription> PluginManager::getInstruments() const {
    auto all = getAvailablePlugins();
    std::vector<PluginDescription> instruments;
    
    for (const auto& desc : all) {
        if (desc.isInstrument)
            instruments.push_back(desc);
    }
    
    return instruments;
}

std::vector<PluginDescription> PluginManager::getEffects() const {
    auto all = getAvailablePlugins();
    std::vector<PluginDescription> effects;
    
    for (const auto& desc : all) {
        if (!desc.isInstrument)
            effects.push_back(desc);
    }
    
    return effects;
}

std::vector<PluginDescription> PluginManager::searchPlugins(const juce::String& query) const {
    auto all = getAvailablePlugins();
    std::vector<PluginDescription> results;
    
    juce::String lowerQuery = query.toLowerCase();
    
    for (const auto& desc : all) {
        if (desc.name.toLowerCase().contains(lowerQuery) ||
            desc.manufacturerName.toLowerCase().contains(lowerQuery) ||
            desc.category.toLowerCase().contains(lowerQuery)) {
            results.push_back(desc);
        }
    }
    
    return results;
}

std::unique_ptr<PluginInstance> PluginManager::loadPlugin(const juce::String& uid) {
    for (const auto& type : knownPluginList.getTypes()) {
        if (type.createIdentifierString() == uid)
            return loadPlugin(PluginDescription{.juceDescription = type, .uid = uid});
    }
    return nullptr;
}

std::unique_ptr<PluginInstance> PluginManager::loadPlugin(const PluginDescription& desc) {
    if (isBlacklisted(desc.uid))
        return nullptr;
    
    juce::String errorMessage;
    
    auto plugin = formatManager.createPluginInstance(
        desc.juceDescription,
        44100.0, 512,
        errorMessage
    );
    
    if (!plugin) {
        DBG("Failed to load plugin: " << errorMessage);
        return nullptr;
    }
    
    return std::make_unique<PluginInstance>(std::move(plugin));
}

void PluginManager::addFormat(std::unique_ptr<juce::AudioPluginFormat> format) {
    formatManager.addFormat(format.release());
}

void PluginManager::addToBlacklist(const juce::String& uid) {
    if (!blacklist.contains(uid))
        blacklist.add(uid);
}

bool PluginManager::isBlacklisted(const juce::String& uid) const {
    return blacklist.contains(uid);
}

void PluginManager::addListener(Listener* listener) {
    listeners.add(listener);
}

void PluginManager::removeListener(Listener* listener) {
    listeners.remove(listener);
}

void PluginManager::saveKnownPluginList() {
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto pluginListFile = appData.getChildFile("OmegaStudio/PluginList.xml");
    
    pluginListFile.getParentDirectory().createDirectory();
    
    if (auto xml = knownPluginList.createXml())
        xml->writeTo(pluginListFile);
}

void PluginManager::loadKnownPluginList() {
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    auto pluginListFile = appData.getChildFile("OmegaStudio/PluginList.xml");
    
    if (pluginListFile.existsAsFile()) {
        if (auto xml = juce::parseXML(pluginListFile))
            knownPluginList.recreateFromXml(*xml);
    }
}

int PluginManager::getLoadedPluginCount() const {
    // No tenemos un contador global de plugins cargados
    // Por ahora retornamos el número de plugins conocidos
    return knownPluginList.getNumTypes();
}

//==============================================================================
// PluginDelayCompensation Implementation
//==============================================================================

PluginDelayCompensation::PluginDelayCompensation() = default;

void PluginDelayCompensation::registerPlugin(const juce::String& pluginUID, int latencySamples) {
    pluginLatencies[pluginUID] = latencySamples;
    recomputeCompensation();
}

void PluginDelayCompensation::unregisterPlugin(const juce::String& pluginUID) {
    pluginLatencies.erase(pluginUID);
    recomputeCompensation();
}

int PluginDelayCompensation::getCompensationFor(const juce::String& pluginUID) const {
    auto it = pluginLatencies.find(pluginUID);
    if (it != pluginLatencies.end())
        return maxLatency - it->second;
    return 0;
}

void PluginDelayCompensation::recomputeCompensation() {
    maxLatency = 0;
    for (const auto& [uid, latency] : pluginLatencies)
        maxLatency = juce::jmax(maxLatency, latency);
}

//==============================================================================
// PluginChain Implementation
//==============================================================================

PluginChain::PluginChain() = default;
PluginChain::~PluginChain() = default;

void PluginChain::addPlugin(std::unique_ptr<PluginInstance> plugin) {
    if (plugin) {
        plugin->getPlugin()->prepareToPlay(sampleRate, blockSize);
        plugins.push_back(std::move(plugin));
    }
}

void PluginChain::insertPlugin(int index, std::unique_ptr<PluginInstance> plugin) {
    if (plugin && index >= 0 && index <= getNumPlugins()) {
        plugin->getPlugin()->prepareToPlay(sampleRate, blockSize);
        plugins.insert(plugins.begin() + index, std::move(plugin));
    }
}

void PluginChain::removePlugin(int index) {
    if (index >= 0 && index < getNumPlugins()) {
        plugins[index]->getPlugin()->releaseResources();
        plugins.erase(plugins.begin() + index);
    }
}

void PluginChain::movePlugin(int fromIndex, int toIndex) {
    if (fromIndex >= 0 && fromIndex < getNumPlugins() &&
        toIndex >= 0 && toIndex < getNumPlugins()) {
        auto plugin = std::move(plugins[fromIndex]);
        plugins.erase(plugins.begin() + fromIndex);
        plugins.insert(plugins.begin() + toIndex, std::move(plugin));
    }
}

void PluginChain::clearPlugins() {
    for (auto& plugin : plugins)
        plugin->getPlugin()->releaseResources();
    plugins.clear();
}

PluginInstance* PluginChain::getPlugin(int index) {
    return (index >= 0 && index < getNumPlugins()) ? plugins[index].get() : nullptr;
}

const PluginInstance* PluginChain::getPlugin(int index) const {
    return (index >= 0 && index < getNumPlugins()) ? plugins[index].get() : nullptr;
}

void PluginChain::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    for (auto& plugin : plugins) {
        if (plugin && !plugin->isBypassed())
            plugin->process(buffer, midiMessages);
    }
}

int PluginChain::getTotalLatency() const {
    int total = 0;
    for (const auto& plugin : plugins) {
        if (plugin && !plugin->isBypassed())
            total += plugin->getLatencySamples();
    }
    return total;
}

juce::var PluginChain::getState() const {
    juce::var array;
    for (const auto& plugin : plugins)
        array.append(plugin->getState().toVar());
    return array;
}

void PluginChain::setState(const juce::var& state) {
    // TODO: Restore plugin chain from state
}

void PluginChain::prepareToPlay(double newSampleRate, int maximumExpectedSamplesPerBlock) {
    sampleRate = newSampleRate;
    blockSize = maximumExpectedSamplesPerBlock;
    
    for (auto& plugin : plugins) {
        if (plugin)
            plugin->getPlugin()->prepareToPlay(sampleRate, blockSize);
    }
}

void PluginChain::releaseResources() {
    for (auto& plugin : plugins) {
        if (plugin)
            plugin->getPlugin()->releaseResources();
    }
}

//==============================================================================
// PluginPresetManager Implementation
//==============================================================================

juce::var PluginPresetManager::Preset::toVar() const {
    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("author", author);
    obj->setProperty("category", category);
    obj->setProperty("pluginUID", pluginUID);
    obj->setProperty("state", state.toVar());
    return juce::var(obj.get());
}

PluginPresetManager::Preset PluginPresetManager::Preset::fromVar(const juce::var& v) {
    Preset preset;
    if (auto* obj = v.getDynamicObject()) {
        preset.name = obj->getProperty("name").toString();
        preset.author = obj->getProperty("author").toString();
        preset.category = obj->getProperty("category").toString();
        preset.pluginUID = obj->getProperty("pluginUID").toString();
        preset.state = PluginState::fromVar(obj->getProperty("state"));
    }
    return preset;
}

PluginPresetManager::PluginPresetManager() {
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
    presetDirectory = appData.getChildFile("OmegaStudio/Presets");
    presetDirectory.createDirectory();
    
    loadPresetCache();
}

void PluginPresetManager::savePreset(const juce::String& pluginUID, const Preset& preset) {
    auto pluginDir = presetDirectory.getChildFile(pluginUID);
    pluginDir.createDirectory();
    
    auto presetFile = pluginDir.getChildFile(preset.name + ".preset");
    auto jsonString = juce::JSON::toString(preset.toVar(), true);
    presetFile.replaceWithText(jsonString);
    
    presetCache[pluginUID].push_back(preset);
}

std::vector<PluginPresetManager::Preset> PluginPresetManager::getPresetsFor(const juce::String& pluginUID) const {
    auto it = presetCache.find(pluginUID);
    return it != presetCache.end() ? it->second : std::vector<Preset>();
}

bool PluginPresetManager::loadPreset(PluginInstance& plugin, const juce::String& presetName) {
    auto presets = getPresetsFor(plugin.getUID());
    
    for (const auto& preset : presets) {
        if (preset.name == presetName) {
            plugin.setState(preset.state);
            return true;
        }
    }
    
    return false;
}

bool PluginPresetManager::deletePreset(const juce::String& pluginUID, const juce::String& presetName) {
    auto pluginDir = presetDirectory.getChildFile(pluginUID);
    auto presetFile = pluginDir.getChildFile(presetName + ".preset");
    
    if (presetFile.existsAsFile()) {
        presetFile.deleteFile();
        loadPresetCache(); // Refresh
        return true;
    }
    
    return false;
}

juce::File PluginPresetManager::getPresetDirectory() const {
    return presetDirectory;
}

void PluginPresetManager::loadPresetCache() {
    presetCache.clear();
    
    for (auto& pluginDir : presetDirectory.findChildFiles(juce::File::findDirectories, false)) {
        juce::String pluginUID = pluginDir.getFileName();
        
        for (auto& presetFile : pluginDir.findChildFiles(juce::File::findFiles, false, "*.preset")) {
            auto jsonString = presetFile.loadFileAsString();
            auto json = juce::JSON::parse(jsonString);
            
            if (!json.isVoid()) {
                auto preset = Preset::fromVar(json);
                presetCache[pluginUID].push_back(preset);
            }
        }
    }
}

void PluginPresetManager::savePresetCache() {
    // Cache is saved on-demand when presets are created
}

} // namespace OmegaStudio
