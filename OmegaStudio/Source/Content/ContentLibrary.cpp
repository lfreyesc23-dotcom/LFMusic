#include "ContentLibrary.h"
#include <algorithm>
#include <cmath>

namespace omega {

//==============================================================================
// ContentLibrary Implementation
//==============================================================================

ContentLibrary::ContentLibrary()
    : deviceManager_(nullptr)
{
}

ContentLibrary::~ContentLibrary()
{
    stopPreview();
    unloadAll();
}

void ContentLibrary::initialize(const juce::File& libraryRoot)
{
    libraryRoot_ = libraryRoot;
    
    // Create directory structure if it doesn't exist
    libraryRoot_.createDirectory();
    libraryRoot_.getChildFile("808s").createDirectory();
    libraryRoot_.getChildFile("Kicks").createDirectory();
    libraryRoot_.getChildFile("Snares").createDirectory();
    libraryRoot_.getChildFile("HiHats").createDirectory();
    libraryRoot_.getChildFile("Claps").createDirectory();
    libraryRoot_.getChildFile("Percs").createDirectory();
    libraryRoot_.getChildFile("Loops").createDirectory();
    libraryRoot_.getChildFile("FX").createDirectory();
    
    scanLibrary();
}

void ContentLibrary::scanLibrary()
{
    sampleDatabase_.clear();
    
    // Scan each category directory
    scanDirectory(libraryRoot_.getChildFile("808s"), "808");
    scanDirectory(libraryRoot_.getChildFile("Kicks"), "Kick");
    scanDirectory(libraryRoot_.getChildFile("Snares"), "Snare");
    scanDirectory(libraryRoot_.getChildFile("HiHats"), "HiHat");
    scanDirectory(libraryRoot_.getChildFile("Claps"), "Clap");
    scanDirectory(libraryRoot_.getChildFile("Percs"), "Perc");
    scanDirectory(libraryRoot_.getChildFile("Loops"), "Loop");
    scanDirectory(libraryRoot_.getChildFile("FX"), "FX");
    
    updateCategoriesAndTags();
}

void ContentLibrary::refreshLibrary()
{
    scanLibrary();
}

void ContentLibrary::scanDirectory(const juce::File& directory, const juce::String& category)
{
    if (!directory.exists()) return;
    
    for (const auto& file : directory.findChildFiles(juce::File::findFiles, false, "*.wav;*.aif;*.aiff;*.mp3;*.flac"))
    {
        auto metadata = extractMetadata(file);
        metadata.category = category;
        
        auto sampleId = generateSampleId(file);
        sampleDatabase_[sampleId] = metadata;
    }
}

SampleMetadata ContentLibrary::extractMetadata(const juce::File& file)
{
    SampleMetadata metadata;
    metadata.name = file.getFileNameWithoutExtension();
    metadata.filePath = file;
    
    // Try to load and analyze audio
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(file));
    if (reader)
    {
        metadata.duration = static_cast<float>(reader->lengthInSamples) / static_cast<float>(reader->sampleRate);
        
        // Simple analysis: detect if tonal (808s have longer decay and more harmonic content)
        if (metadata.duration > 0.5f)
        {
            metadata.isTonal = true;
            
            // Extract key from filename if present (e.g., "808_C_Dark.wav")
            auto tokens = juce::StringArray::fromTokens(metadata.name, "_- ", "");
            for (const auto& token : tokens)
            {
                if (token.length() <= 2 && (token[0] >= 'A' && token[0] <= 'G'))
                {
                    metadata.key = token.toUpperCase();
                    
                    // Map key to MIDI note (C = 60)
                    const char note = token[0];
                    const bool sharp = token.length() > 1 && token[1] == '#';
                    int noteOffset = 0;
                    
                    if (note == 'C') noteOffset = 0;
                    else if (note == 'D') noteOffset = 2;
                    else if (note == 'E') noteOffset = 4;
                    else if (note == 'F') noteOffset = 5;
                    else if (note == 'G') noteOffset = 7;
                    else if (note == 'A') noteOffset = 9;
                    else if (note == 'B') noteOffset = 11;
                    
                    metadata.rootNote = 60 + noteOffset + (sharp ? 1 : 0);
                    break;
                }
            }
        }
    }
    
    // Extract genre and mood from parent directory or filename
    auto parentDir = file.getParentDirectory().getFileName();
    if (parentDir.containsIgnoreCase("Trap")) metadata.genre = "Trap";
    else if (parentDir.containsIgnoreCase("Drill")) metadata.genre = "Drill";
    else if (parentDir.containsIgnoreCase("House")) metadata.genre = "House";
    else if (parentDir.containsIgnoreCase("Techno")) metadata.genre = "Techno";
    
    if (metadata.name.containsIgnoreCase("Dark")) {
        metadata.mood = "Dark";
        metadata.tags.add("Dark");
    }
    if (metadata.name.containsIgnoreCase("Hard")) {
        metadata.tags.add("Hard");
    }
    if (metadata.name.containsIgnoreCase("Soft")) {
        metadata.tags.add("Soft");
    }
    
    return metadata;
}

juce::String ContentLibrary::generateSampleId(const juce::File& file)
{
    return file.getFullPathName();
}

void ContentLibrary::updateCategoriesAndTags()
{
    categories_.clear();
    genres_.clear();
    allTags_.clear();
    
    std::set<juce::String> uniqueCategories;
    std::set<juce::String> uniqueGenres;
    std::set<juce::String> uniqueTags;
    
    for (const auto& [id, metadata] : sampleDatabase_)
    {
        if (metadata.category.isNotEmpty())
            uniqueCategories.insert(metadata.category);
        if (metadata.genre.isNotEmpty())
            uniqueGenres.insert(metadata.genre);
        for (const auto& tag : metadata.tags)
            uniqueTags.insert(tag);
    }
    
    for (const auto& cat : uniqueCategories) categories_.add(cat);
    for (const auto& genre : uniqueGenres) genres_.add(genre);
    for (const auto& tag : uniqueTags) allTags_.add(tag);
}

std::shared_ptr<LoadedSample> ContentLibrary::loadSample(const juce::String& sampleId)
{
    // Check if already loaded
    auto it = loadedSamples_.find(sampleId);
    if (it != loadedSamples_.end())
        return it->second;
    
    // Find in database
    auto metaIt = sampleDatabase_.find(sampleId);
    if (metaIt == sampleDatabase_.end())
        return nullptr;
    
    return loadSampleByPath(metaIt->second.filePath);
}

std::shared_ptr<LoadedSample> ContentLibrary::loadSampleByPath(const juce::File& path)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formatManager.createReaderFor(path));
    if (!reader)
        return nullptr;
    
    auto loadedSample = std::make_shared<LoadedSample>();
    loadedSample->buffer = std::make_unique<juce::AudioBuffer<float>>(
        static_cast<int>(reader->numChannels),
        static_cast<int>(reader->lengthInSamples)
    );
    
    reader->read(loadedSample->buffer.get(), 0, static_cast<int>(reader->lengthInSamples), 0, true, true);
    
    loadedSample->sampleRate = reader->sampleRate;
    loadedSample->isLoaded = true;
    
    auto sampleId = generateSampleId(path);
    auto metaIt = sampleDatabase_.find(sampleId);
    if (metaIt != sampleDatabase_.end())
        loadedSample->metadata = metaIt->second;
    
    loadedSamples_[sampleId] = loadedSample;
    return loadedSample;
}

void ContentLibrary::preloadCategory(const juce::String& category)
{
    for (const auto& [id, metadata] : sampleDatabase_)
    {
        if (metadata.category == category)
            loadSample(id);
    }
}

void ContentLibrary::unloadAll()
{
    loadedSamples_.clear();
}

std::vector<SampleMetadata> ContentLibrary::searchSamples(
    const juce::String& query,
    const juce::String& category,
    const juce::String& genre,
    int minBpm,
    int maxBpm)
{
    std::vector<SampleMetadata> results;
    
    for (const auto& [id, metadata] : sampleDatabase_)
    {
        // Filter by category
        if (category.isNotEmpty() && metadata.category != category)
            continue;
        
        // Filter by genre
        if (genre.isNotEmpty() && metadata.genre != genre)
            continue;
        
        // Filter by BPM
        if (metadata.bpm > 0 && (metadata.bpm < minBpm || metadata.bpm > maxBpm))
            continue;
        
        // Search query
        if (query.isNotEmpty() && !matchesSearchQuery(metadata, query))
            continue;
        
        results.push_back(metadata);
    }
    
    return results;
}

bool ContentLibrary::matchesSearchQuery(const SampleMetadata& metadata, const juce::String& query)
{
    auto lowerQuery = query.toLowerCase();
    
    if (metadata.name.toLowerCase().contains(lowerQuery)) return true;
    if (metadata.category.toLowerCase().contains(lowerQuery)) return true;
    if (metadata.genre.toLowerCase().contains(lowerQuery)) return true;
    if (metadata.mood.toLowerCase().contains(lowerQuery)) return true;
    if (metadata.key.toLowerCase().contains(lowerQuery)) return true;
    
    for (const auto& tag : metadata.tags)
        if (tag.toLowerCase().contains(lowerQuery))
            return true;
    
    return false;
}

std::vector<SampleMetadata> ContentLibrary::getSamplesByCategory(const juce::String& category)
{
    return searchSamples("", category, "", 0, 999);
}

std::vector<SampleMetadata> ContentLibrary::getSamplesByGenre(const juce::String& genre)
{
    return searchSamples("", "", genre, 0, 999);
}

std::vector<SampleMetadata> ContentLibrary::getSamplesByKey(const juce::String& key)
{
    std::vector<SampleMetadata> results;
    for (const auto& [id, metadata] : sampleDatabase_)
    {
        if (metadata.key.equalsIgnoreCase(key))
            results.push_back(metadata);
    }
    return results;
}

std::vector<SampleMetadata> ContentLibrary::getSamplesByTag(const juce::String& tag)
{
    std::vector<SampleMetadata> results;
    for (const auto& [id, metadata] : sampleDatabase_)
    {
        if (metadata.tags.contains(tag))
            results.push_back(metadata);
    }
    return results;
}

juce::StringArray ContentLibrary::getCategories() const
{
    return categories_;
}

juce::StringArray ContentLibrary::getGenres() const
{
    return genres_;
}

juce::StringArray ContentLibrary::getTags() const
{
    return allTags_;
}

int ContentLibrary::getTotalSampleCount() const
{
    return static_cast<int>(sampleDatabase_.size());
}

void ContentLibrary::addToFavorites(const juce::String& sampleId)
{
    favorites_.insert(sampleId);
}

void ContentLibrary::removeFromFavorites(const juce::String& sampleId)
{
    favorites_.erase(sampleId);
}

bool ContentLibrary::isFavorite(const juce::String& sampleId) const
{
    return favorites_.find(sampleId) != favorites_.end();
}

std::vector<SampleMetadata> ContentLibrary::getFavorites() const
{
    std::vector<SampleMetadata> results;
    for (const auto& id : favorites_)
    {
        auto it = sampleDatabase_.find(id);
        if (it != sampleDatabase_.end())
            results.push_back(it->second);
    }
    return results;
}

void ContentLibrary::createCollection(const juce::String& name)
{
    collections_[name] = std::vector<juce::String>();
}

void ContentLibrary::deleteCollection(const juce::String& name)
{
    collections_.erase(name);
}

void ContentLibrary::addToCollection(const juce::String& collectionName, const juce::String& sampleId)
{
    collections_[collectionName].push_back(sampleId);
}

void ContentLibrary::removeFromCollection(const juce::String& collectionName, const juce::String& sampleId)
{
    auto& collection = collections_[collectionName];
    collection.erase(std::remove(collection.begin(), collection.end(), sampleId), collection.end());
}

std::vector<juce::String> ContentLibrary::getCollectionNames() const
{
    std::vector<juce::String> names;
    for (const auto& [name, samples] : collections_)
        names.push_back(name);
    return names;
}

std::vector<SampleMetadata> ContentLibrary::getCollectionSamples(const juce::String& name) const
{
    std::vector<SampleMetadata> results;
    auto it = collections_.find(name);
    if (it != collections_.end())
    {
        for (const auto& id : it->second)
        {
            auto metaIt = sampleDatabase_.find(id);
            if (metaIt != sampleDatabase_.end())
                results.push_back(metaIt->second);
        }
    }
    return results;
}

void ContentLibrary::setPreviewPlayer(juce::AudioDeviceManager* deviceManager)
{
    deviceManager_ = deviceManager;
}

void ContentLibrary::previewSample(const juce::String& sampleId)
{
    // TODO: Implement preview player
}

void ContentLibrary::stopPreview()
{
    // TODO: Implement preview stop
}

bool ContentLibrary::isPreviewPlaying() const
{
    return false;
}

//==============================================================================
// BuiltInSampleLibrary Implementation
//==============================================================================

void BuiltInSampleLibrary::initializeBuiltInSamples(ContentLibrary& library)
{
    // Generate 12 chromatic 808s (C to B)
    const char* notes[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const float frequencies[] = {32.7f, 34.6f, 36.7f, 38.9f, 41.2f, 43.7f, 46.2f, 49.0f, 51.9f, 55.0f, 58.3f, 61.7f};
    
    // Access library root through getter - temporarily disabled actual generation
    (void)library;  // Suppress unused warning
    (void)notes;
    (void)frequencies;
    // auto builtInDir = library.libraryRoot_.getChildFile("Built-In");
    // builtInDir.createDirectory();
    
    // Generate 808s in different flavors
    for (int i = 0; i < 12; ++i)
    {
        // Dark 808
        auto dark808 = generate808(frequencies[i], 2.0f, 0.7f, 0.3f, 48000.0);
        // Save to file...
        
        // Hard 808
        auto hard808 = generate808(frequencies[i], 1.5f, 0.9f, 0.6f, 48000.0);
        // Save to file...
        
        // Soft 808
        auto soft808 = generate808(frequencies[i], 2.5f, 0.5f, 0.1f, 48000.0);
        // Save to file...
    }
    
    // Generate kick variations
    auto trapKick = generateKick(55.0f, 0.8f, 0.15f, 48000.0);
    auto drillKick = generateKick(45.0f, 0.95f, 0.12f, 48000.0);
    auto houseKick = generateKick(60.0f, 0.6f, 0.2f, 48000.0);
    
    // Generate snare variations
    auto trapSnare = generateSnare(200.0f, 0.7f, 0.15f, 48000.0);
    auto drillSnare = generateSnare(180.0f, 0.8f, 0.12f, 48000.0);
    
    // Generate hi-hats
    auto closedHat = generateHiHat(false, 0.7f, 0.08f, 48000.0);
    auto openHat = generateHiHat(true, 0.6f, 0.4f, 48000.0);
    
    // Generate claps
    auto trapClap = generateClap(15.0f, 3, 48000.0);
    auto drillClap = generateClap(20.0f, 4, 48000.0);
}

std::unique_ptr<juce::AudioBuffer<float>> BuiltInSampleLibrary::generate808(
    float frequency,
    float decay,
    float punch,
    float distortion,
    double sampleRate)
{
    const int numSamples = static_cast<int>(decay * sampleRate);
    auto buffer = std::make_unique<juce::AudioBuffer<float>>(1, numSamples);
    
    float* data = buffer->getWritePointer(0);
    
    float phase = 0.0f;
    
    for (int i = 0; i < numSamples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        
        // Pitch envelope (starts higher, drops quickly)
        float pitchEnv = std::exp(-t * 15.0f * punch);
        float currentFreq = frequency * (1.0f + pitchEnv * 2.0f);
        
        // Amplitude envelope
        float ampEnv = std::exp(-t / decay);
        
        // Generate sine wave
        float sample = std::sin(phase * 2.0f * juce::MathConstants<float>::pi);
        
        // Add harmonics for punch
        sample += 0.3f * std::sin(phase * 4.0f * juce::MathConstants<float>::pi) * pitchEnv;
        
        // Apply envelopes
        sample *= ampEnv;
        
        // Soft clipping distortion
        if (distortion > 0.0f)
        {
            float distorted = std::tanh(sample * (1.0f + distortion * 5.0f));
            sample = sample * (1.0f - distortion) + distorted * distortion;
        }
        
        data[i] = sample * 0.8f;
        
        phase += currentFreq / static_cast<float>(sampleRate);
        if (phase >= 1.0f) phase -= 1.0f;
    }
    
    return buffer;
}

std::unique_ptr<juce::AudioBuffer<float>> BuiltInSampleLibrary::generateKick(
    float pitch,
    float punchAmount,
    float decayTime,
    double sampleRate)
{
    const int numSamples = static_cast<int>(decayTime * sampleRate);
    auto buffer = std::make_unique<juce::AudioBuffer<float>>(1, numSamples);
    
    float* data = buffer->getWritePointer(0);
    
    float phase = 0.0f;
    
    for (int i = 0; i < numSamples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        
        // Pitch envelope (steep drop)
        float pitchEnv = std::exp(-t * 50.0f);
        float currentFreq = pitch * (1.0f + pitchEnv * 3.0f * punchAmount);
        
        // Amplitude envelope
        float ampEnv = std::exp(-t / decayTime);
        
        // Click/punch at start
        float click = std::exp(-t * 200.0f) * punchAmount * 0.3f;
        
        // Sine wave
        float sample = std::sin(phase * 2.0f * juce::MathConstants<float>::pi);
        
        data[i] = (sample * ampEnv + click) * 0.9f;
        
        phase += currentFreq / static_cast<float>(sampleRate);
        if (phase >= 1.0f) phase -= 1.0f;
    }
    
    return buffer;
}

std::unique_ptr<juce::AudioBuffer<float>> BuiltInSampleLibrary::generateSnare(
    float tone,
    float snap,
    float decay,
    double sampleRate)
{
    const int numSamples = static_cast<int>(decay * sampleRate);
    auto buffer = std::make_unique<juce::AudioBuffer<float>>(1, numSamples);
    
    float* data = buffer->getWritePointer(0);
    
    float phase = 0.0f;
    juce::Random random;
    
    for (int i = 0; i < numSamples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        float ampEnv = std::exp(-t / decay);
        
        // Tonal component (body)
        float tonal = std::sin(phase * 2.0f * juce::MathConstants<float>::pi);
        tonal += 0.5f * std::sin(phase * 3.7f * juce::MathConstants<float>::pi);
        
        // Noise component (snare wires)
        float noise = random.nextFloat() * 2.0f - 1.0f;
        
        // Mix tonal and noise
        float sample = tonal * (1.0f - snap) + noise * snap;
        
        data[i] = sample * ampEnv * 0.6f;
        
        phase += tone / static_cast<float>(sampleRate);
        if (phase >= 1.0f) phase -= 1.0f;
    }
    
    return buffer;
}

std::unique_ptr<juce::AudioBuffer<float>> BuiltInSampleLibrary::generateHiHat(
    bool /* isOpen */,
    float brightness,
    float decay,
    double sampleRate)
{
    const int numSamples = static_cast<int>(decay * sampleRate);
    auto buffer = std::make_unique<juce::AudioBuffer<float>>(1, numSamples);
    
    float* data = buffer->getWritePointer(0);
    juce::Random random;
    
    for (int i = 0; i < numSamples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        
        // Generate filtered noise
        float noise = random.nextFloat() * 2.0f - 1.0f;
        
        // Simple high-pass (brightness)
        static float lastSample = 0.0f;
        float highPassed = noise - lastSample * 0.95f * brightness;
        lastSample = noise;
        
        // Envelope
        float ampEnv = std::exp(-t / decay);
        
        data[i] = highPassed * ampEnv * 0.5f;
    }
    
    return buffer;
}

std::unique_ptr<juce::AudioBuffer<float>> BuiltInSampleLibrary::generateClap(
    float spread,
    int layers,
    double sampleRate)
{
    const float totalDuration = 0.15f + spread / 1000.0f * layers;
    const int numSamples = static_cast<int>(totalDuration * sampleRate);
    auto buffer = std::make_unique<juce::AudioBuffer<float>>(1, numSamples);
    
    buffer->clear();
    float* data = buffer->getWritePointer(0);
    
    juce::Random random;
    
    // Generate multiple layers
    for (int layer = 0; layer < layers; ++layer)
    {
        float delay = layer * spread / 1000.0f;
        int startSample = static_cast<int>(delay * sampleRate);
        
        for (int i = 0; i < 3000 && (startSample + i) < numSamples; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(sampleRate);
            float noise = random.nextFloat() * 2.0f - 1.0f;
            float env = std::exp(-t * 40.0f);
            
            data[startSample + i] += noise * env * 0.3f;
        }
    }
    
    return buffer;
}

} // namespace omega
