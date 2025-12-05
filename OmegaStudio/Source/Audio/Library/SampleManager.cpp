/**
 * @file SampleManager.cpp
 * @brief Implementation of sample library management system
 */

#include "SampleManager.h"
#include <algorithm>
#include <cmath>

namespace omega {

// ============================================================================
// Sample Implementation
// ============================================================================

Sample::Sample(const SampleMetadata& metadata)
    : m_metadata(metadata) {
}

bool Sample::load() {
    if (m_loaded) {
        return true;
    }

    auto reader = createReader();
    if (!reader) {
        return false;
    }

    // Allocate buffer
    m_buffer = std::make_unique<juce::AudioBuffer<float>>(
        static_cast<int>(reader->numChannels),
        static_cast<int>(reader->lengthInSamples)
    );

    // Read audio data
    reader->read(m_buffer.get(), 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

    m_loaded = true;
    return true;
}

void Sample::unload() {
    m_buffer.reset();
    m_loaded = false;
}

std::unique_ptr<juce::AudioFormatReader> Sample::createReader() const {
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    return std::unique_ptr<juce::AudioFormatReader>(
        formatManager.createReaderFor(m_metadata.filePath)
    );
}

// ============================================================================
// SampleLibrary Implementation
// ============================================================================

SampleLibrary::SampleLibrary(const juce::String& name)
    : m_name(name) {
}

bool SampleLibrary::addSample(std::shared_ptr<Sample> sample) {
    if (!sample) {
        return false;
    }

    juce::ScopedLock lock(m_lock);
    const auto& uuid = sample->getMetadata().uuid;
    m_samples[uuid] = sample;
    return true;
}

bool SampleLibrary::removeSample(const juce::String& uuid) {
    juce::ScopedLock lock(m_lock);
    return m_samples.erase(uuid) > 0;
}

std::shared_ptr<Sample> SampleLibrary::getSample(const juce::String& uuid) const {
    juce::ScopedLock lock(m_lock);
    auto it = m_samples.find(uuid);
    return (it != m_samples.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Sample>> SampleLibrary::getAllSamples() const {
    juce::ScopedLock lock(m_lock);
    std::vector<std::shared_ptr<Sample>> result;
    result.reserve(m_samples.size());
    
    for (const auto& pair : m_samples) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::shared_ptr<Sample>> SampleLibrary::getSamplesByCategory(const juce::String& category) const {
    juce::ScopedLock lock(m_lock);
    std::vector<std::shared_ptr<Sample>> result;
    
    for (const auto& pair : m_samples) {
        if (pair.second->getMetadata().category == category) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<Sample>> SampleLibrary::searchSamples(const juce::String& query) const {
    juce::ScopedLock lock(m_lock);
    std::vector<std::shared_ptr<Sample>> result;
    
    juce::String lowerQuery = query.toLowerCase();
    
    for (const auto& pair : m_samples) {
        const auto& metadata = pair.second->getMetadata();
        
        // Search in name
        if (metadata.name.toLowerCase().contains(lowerQuery)) {
            result.push_back(pair.second);
            continue;
        }
        
        // Search in category
        if (metadata.category.toLowerCase().contains(lowerQuery)) {
            result.push_back(pair.second);
            continue;
        }
        
        // Search in tags
        for (const auto& tag : metadata.tags) {
            if (tag.toLowerCase().contains(lowerQuery)) {
                result.push_back(pair.second);
                break;
            }
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<Sample>> SampleLibrary::filterByTags(const std::set<juce::String>& tags) const {
    juce::ScopedLock lock(m_lock);
    std::vector<std::shared_ptr<Sample>> result;
    
    for (const auto& pair : m_samples) {
        const auto& sampleTags = pair.second->getMetadata().tags;
        
        // Check if sample has all required tags
        bool hasAllTags = true;
        for (const auto& tag : tags) {
            if (sampleTags.find(tag) == sampleTags.end()) {
                hasAllTags = false;
                break;
            }
        }
        
        if (hasAllTags) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

std::set<juce::String> SampleLibrary::getAllCategories() const {
    juce::ScopedLock lock(m_lock);
    std::set<juce::String> categories;
    
    for (const auto& pair : m_samples) {
        categories.insert(pair.second->getMetadata().category);
    }
    
    return categories;
}

std::set<juce::String> SampleLibrary::getAllTags() const {
    juce::ScopedLock lock(m_lock);
    std::set<juce::String> tags;
    
    for (const auto& pair : m_samples) {
        const auto& sampleTags = pair.second->getMetadata().tags;
        tags.insert(sampleTags.begin(), sampleTags.end());
    }
    
    return tags;
}

// ============================================================================
// SampleManager Implementation
// ============================================================================

SampleManager::SampleManager() {
    m_formatManager.registerBasicFormats();
    m_threadPool = std::make_unique<juce::ThreadPool>(4); // 4 background threads
}

SampleManager::~SampleManager() {
    clearCache();
}

void SampleManager::initialize(int maxMemoryMB) {
    m_maxMemoryMB = maxMemoryMB;
    
    // Create default library
    createLibrary("Default");
}

int SampleManager::scanDirectory(const juce::File& directory,
                                bool recursive,
                                ScanProgressCallback progressCallback) {
    if (!directory.exists() || !directory.isDirectory()) {
        return 0;
    }

    // Find all audio files
    std::vector<juce::File> audioFiles;
    
    juce::Array<juce::File> files;
    directory.findChildFiles(files,
                            juce::File::findFiles,
                            recursive,
                            "*");

    for (const auto& file : files) {
        if (isAudioFile(file)) {
            audioFiles.push_back(file);
        }
    }

    // Import files
    int imported = 0;
    for (size_t i = 0; i < audioFiles.size(); ++i) {
        if (progressCallback) {
            progressCallback(static_cast<int>(i), 
                           static_cast<int>(audioFiles.size()),
                           audioFiles[i].getFileName());
        }

        if (!importFile(audioFiles[i]).isEmpty()) {
            imported++;
        }
    }

    return imported;
}

juce::String SampleManager::importFile(const juce::File& file,
                                      const juce::String& category,
                                      bool autoAnalyze) {
    if (!isAudioFile(file)) {
        return {};
    }

    // Extract metadata
    SampleMetadata metadata = extractMetadata(file);
    metadata.category = category;
    metadata.uuid = generateUUID();

    // Create sample
    auto sample = std::make_shared<Sample>(metadata);

    // Add to default library
    auto* library = getLibrary("Default");
    if (!library) {
        library = createLibrary("Default");
    }

    if (!library->addSample(sample)) {
        return {};
    }

    // Auto-analyze if requested
    if (autoAnalyze && m_autoAnalysis) {
        analyzeSample(metadata.uuid);
    }

    return metadata.uuid;
}

SampleLibrary* SampleManager::createLibrary(const juce::String& name) {
    juce::ScopedLock lock(m_lock);
    
    if (m_libraries.find(name) != m_libraries.end()) {
        return nullptr; // Already exists
    }

    auto library = std::make_unique<SampleLibrary>(name);
    auto* ptr = library.get();
    m_libraries[name] = std::move(library);
    
    return ptr;
}

SampleLibrary* SampleManager::getLibrary(const juce::String& name) const {
    juce::ScopedLock lock(m_lock);
    auto it = m_libraries.find(name);
    return (it != m_libraries.end()) ? it->second.get() : nullptr;
}

std::vector<SampleLibrary*> SampleManager::getAllLibraries() const {
    juce::ScopedLock lock(m_lock);
    std::vector<SampleLibrary*> result;
    result.reserve(m_libraries.size());
    
    for (const auto& pair : m_libraries) {
        result.push_back(pair.second.get());
    }
    
    return result;
}

bool SampleManager::removeLibrary(const juce::String& name) {
    juce::ScopedLock lock(m_lock);
    return m_libraries.erase(name) > 0;
}

std::shared_ptr<Sample> SampleManager::getSample(const juce::String& uuid) const {
    juce::ScopedLock lock(m_lock);
    
    for (const auto& pair : m_libraries) {
        auto sample = pair.second->getSample(uuid);
        if (sample) {
            return sample;
        }
    }
    
    return nullptr;
}

bool SampleManager::loadSample(const juce::String& uuid) {
    auto sample = getSample(uuid);
    if (!sample) {
        return false;
    }

    if (sample->isLoaded()) {
        return true;
    }

    // Check memory limits
    manageCacheSize();

    // Load sample
    bool loaded = sample->load();
    
    if (loaded) {
        // Update LRU cache
        m_lruCache.push_back(uuid);
        
        // Update memory usage
        const auto* buffer = sample->getBuffer();
        if (buffer) {
            int memoryMB = (buffer->getNumSamples() * buffer->getNumChannels() * sizeof(float)) / (1024 * 1024);
            m_currentMemoryUsage += memoryMB;
        }
    }

    return loaded;
}

void SampleManager::unloadSample(const juce::String& uuid) {
    auto sample = getSample(uuid);
    if (!sample || !sample->isLoaded()) {
        return;
    }

    // Calculate memory to free
    const auto* buffer = sample->getBuffer();
    if (buffer) {
        int memoryMB = (buffer->getNumSamples() * buffer->getNumChannels() * sizeof(float)) / (1024 * 1024);
        m_currentMemoryUsage -= memoryMB;
    }

    sample->unload();

    // Remove from LRU cache
    m_lruCache.erase(std::remove(m_lruCache.begin(), m_lruCache.end(), uuid), m_lruCache.end());
}

void SampleManager::preloadSamples(const std::vector<juce::String>& uuids) {
    for (const auto& uuid : uuids) {
        m_threadPool->addJob([this, uuid]() {
            loadSample(uuid);
        });
    }
}

std::vector<std::shared_ptr<Sample>> SampleManager::globalSearch(const juce::String& query) const {
    std::vector<std::shared_ptr<Sample>> results;
    
    juce::ScopedLock lock(m_lock);
    for (const auto& pair : m_libraries) {
        auto libraryResults = pair.second->searchSamples(query);
        results.insert(results.end(), libraryResults.begin(), libraryResults.end());
    }
    
    return results;
}

bool SampleManager::analyzeSample(const juce::String& uuid) {
    auto sample = getSample(uuid);
    if (!sample) {
        return false;
    }

    // Load sample if not loaded
    bool wasLoaded = sample->isLoaded();
    if (!wasLoaded) {
        if (!sample->load()) {
            return false;
        }
    }

    // Detect BPM and key
    bool bpmSuccess = detectBPM(sample.get());
    bool keySuccess = detectKey(sample.get());

    // Unload if we loaded it
    if (!wasLoaded) {
        sample->unload();
    }

    return bpmSuccess || keySuccess;
}

juce::Image SampleManager::generateThumbnail(const juce::String& uuid, int width, int height) {
    // Check cache first
    {
        juce::ScopedLock lock(m_thumbnailLock);
        auto it = m_thumbnailCache.find(uuid);
        if (it != m_thumbnailCache.end()) {
            return it->second;
        }
    }

    auto sample = getSample(uuid);
    if (!sample) {
        return {};
    }

    // Create reader
    auto reader = sample->createReader();
    if (!reader) {
        return {};
    }

    // Create thumbnail
    juce::Image thumbnail(juce::Image::RGB, width, height, true);
    juce::Graphics g(thumbnail);

    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::lightblue);

    // Sample waveform at regular intervals
    int64_t totalSamples = reader->lengthInSamples;
    int samplesPerPixel = std::max(1LL, totalSamples / width);

    juce::AudioBuffer<float> buffer(1, samplesPerPixel);

    for (int x = 0; x < width; ++x) {
        int64_t startSample = x * samplesPerPixel;
        
        reader->read(&buffer, 0, samplesPerPixel, startSample, true, false);

        // Find peak in this segment
        float peak = 0.0f;
        for (int i = 0; i < samplesPerPixel; ++i) {
            peak = std::max(peak, std::abs(buffer.getSample(0, i)));
        }

        // Draw line
        int lineHeight = static_cast<int>(peak * height / 2);
        int centerY = height / 2;
        
        g.drawVerticalLine(x, centerY - lineHeight, centerY + lineHeight);
    }

    // Cache thumbnail
    {
        juce::ScopedLock lock(m_thumbnailLock);
        m_thumbnailCache[uuid] = thumbnail;
    }

    return thumbnail;
}

bool SampleManager::saveDatabase(const juce::File& databaseFile) {
    juce::ScopedLock lock(m_lock);

    juce::var jsonData = juce::var(new juce::DynamicObject());
    auto* root = jsonData.getDynamicObject();

    // Save libraries
    juce::var librariesArray;
    for (const auto& libPair : m_libraries) {
        juce::var libraryObj = juce::var(new juce::DynamicObject());
        auto* lib = libraryObj.getDynamicObject();
        
        lib->setProperty("name", libPair.first);

        // Save samples
        juce::var samplesArray;
        for (const auto& sample : libPair.second->getAllSamples()) {
            juce::var sampleObj = juce::var(new juce::DynamicObject());
            auto* smp = sampleObj.getDynamicObject();
            
            const auto& meta = sample->getMetadata();
            smp->setProperty("uuid", meta.uuid);
            smp->setProperty("name", meta.name);
            smp->setProperty("filePath", meta.filePath.getFullPathName());
            smp->setProperty("category", meta.category);
            smp->setProperty("bpm", meta.bpm);
            smp->setProperty("key", meta.key);
            
            samplesArray.append(sampleObj);
        }
        
        lib->setProperty("samples", samplesArray);
        librariesArray.append(libraryObj);
    }
    
    root->setProperty("libraries", librariesArray);

    // Write to file
    juce::String jsonString = juce::JSON::toString(jsonData);
    return databaseFile.replaceWithText(jsonString);
}

bool SampleManager::loadDatabase(const juce::File& databaseFile) {
    if (!databaseFile.existsAsFile()) {
        return false;
    }

    juce::String jsonString = databaseFile.loadFileAsString();
    juce::var jsonData = juce::JSON::parse(jsonString);

    if (!jsonData.isObject()) {
        return false;
    }

    juce::ScopedLock lock(m_lock);

    // Clear existing libraries
    m_libraries.clear();

    // Load libraries
    auto* root = jsonData.getDynamicObject();
    juce::var librariesArray = root->getProperty("libraries");

    for (int i = 0; i < librariesArray.size(); ++i) {
        auto libraryObj = librariesArray[i];
        auto* lib = libraryObj.getDynamicObject();
        
        juce::String libName = lib->getProperty("name");
        auto* library = createLibrary(libName);

        // Load samples
        juce::var samplesArray = lib->getProperty("samples");
        for (int j = 0; j < samplesArray.size(); ++j) {
            auto sampleObj = samplesArray[j];
            auto* smp = sampleObj.getDynamicObject();

            SampleMetadata meta;
            meta.uuid = smp->getProperty("uuid");
            meta.name = smp->getProperty("name");
            meta.filePath = juce::File(smp->getProperty("filePath").toString());
            meta.category = smp->getProperty("category");
            meta.bpm = smp->getProperty("bpm");
            meta.key = smp->getProperty("key");

            auto sample = std::make_shared<Sample>(meta);
            library->addSample(sample);
        }
    }

    return true;
}

int SampleManager::getTotalSampleCount() const {
    juce::ScopedLock lock(m_lock);
    int count = 0;
    
    for (const auto& pair : m_libraries) {
        count += pair.second->getSampleCount();
    }
    
    return count;
}

int SampleManager::getLoadedSampleCount() const {
    return static_cast<int>(m_lruCache.size());
}

float SampleManager::getMemoryUsageMB() const {
    return static_cast<float>(m_currentMemoryUsage.load());
}

void SampleManager::clearCache() {
    for (const auto& uuid : m_lruCache) {
        unloadSample(uuid);
    }
    m_lruCache.clear();
}

bool SampleManager::isAudioFile(const juce::File& file) const {
    juce::String extension = file.getFileExtension().toLowerCase();
    return extension == ".wav" || extension == ".aiff" || extension == ".aif" ||
           extension == ".mp3" || extension == ".flac" || extension == ".ogg";
}

SampleMetadata SampleManager::extractMetadata(const juce::File& file) {
    SampleMetadata metadata;
    metadata.name = file.getFileNameWithoutExtension();
    metadata.filePath = file;
    metadata.dateAdded = juce::Time::getCurrentTime();

    // Read audio properties
    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        m_formatManager.createReaderFor(file)
    );

    if (reader) {
        metadata.sampleRate = reader->sampleRate;
        metadata.numChannels = static_cast<int>(reader->numChannels);
        metadata.lengthInSamples = reader->lengthInSamples;
        metadata.lengthInSeconds = reader->lengthInSamples / reader->sampleRate;
    }

    return metadata;
}

juce::String SampleManager::generateUUID() const {
    return juce::Uuid().toString();
}

bool SampleManager::detectBPM(Sample* sample) {
    if (!sample || !sample->isLoaded()) {
        return false;
    }

    BPMDetector detector;
    float bpm = detector.detectBPM(*sample->getBuffer(), sample->getMetadata().sampleRate);

    if (bpm > 0.0f) {
        SampleMetadata meta = sample->getMetadata();
        meta.bpm = bpm;
        sample->updateMetadata(meta);
        return true;
    }

    return false;
}

bool SampleManager::detectKey(Sample* sample) {
    if (!sample || !sample->isLoaded()) {
        return false;
    }

    KeyDetector detector;
    int key = detector.detectKey(*sample->getBuffer(), sample->getMetadata().sampleRate);

    if (key >= 0) {
        SampleMetadata meta = sample->getMetadata();
        meta.key = key;
        meta.keyName = KeyDetector::getKeyName(key);
        sample->updateMetadata(meta);
        return true;
    }

    return false;
}

void SampleManager::manageCacheSize() {
    // Unload oldest samples if over memory limit
    while (m_currentMemoryUsage.load() > m_maxMemoryMB && !m_lruCache.empty()) {
        juce::String oldestUuid = m_lruCache.front();
        unloadSample(oldestUuid);
    }
}

// ============================================================================
// BPMDetector Implementation
// ============================================================================

BPMDetector::BPMDetector() = default;

float BPMDetector::detectBPM(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    // Simplified BPM detection using energy-based onset detection
    // For production, use librosa or essentia for better accuracy

    const int hopSize = 512;
    const int numFrames = buffer.getNumSamples() / hopSize;
    
    std::vector<float> energy(numFrames);

    // Calculate energy for each frame
    for (int frame = 0; frame < numFrames; ++frame) {
        float frameEnergy = 0.0f;
        int startSample = frame * hopSize;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < hopSize && (startSample + i) < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, startSample + i);
                frameEnergy += sample * sample;
            }
        }

        energy[frame] = std::sqrt(frameEnergy / hopSize);
    }

    // Detect onsets
    detectOnsets(energy);

    // Calculate BPM from onsets
    return calculateBPMFromOnsets();
}

void BPMDetector::detectOnsets(const std::vector<float>& energy) {
    m_onsets.clear();

    // Simple threshold-based onset detection
    const float threshold = 0.1f;

    for (size_t i = 1; i < energy.size() - 1; ++i) {
        float diff = energy[i] - energy[i - 1];
        if (diff > threshold && energy[i] > energy[i + 1]) {
            m_onsets.push_back(static_cast<float>(i));
        }
    }
}

float BPMDetector::calculateBPMFromOnsets() {
    if (m_onsets.size() < 2) {
        return 0.0f;
    }

    // Calculate inter-onset intervals
    std::vector<float> intervals;
    for (size_t i = 1; i < m_onsets.size(); ++i) {
        intervals.push_back(m_onsets[i] - m_onsets[i - 1]);
    }

    // Find median interval
    std::sort(intervals.begin(), intervals.end());
    float medianInterval = intervals[intervals.size() / 2];

    // Convert to BPM (assuming 512 hop size at 44100 Hz)
    float bpm = (60.0f * 44100.0f) / (medianInterval * 512.0f);

    // Clamp to reasonable range
    return juce::jlimit(kMinBPM, kMaxBPM, bpm);
}

// ============================================================================
// KeyDetector Implementation
// ============================================================================

KeyDetector::KeyDetector() {
    m_chromaProfile.fill(0.0f);
}

int KeyDetector::detectKey(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    calculateChroma(buffer, sampleRate);
    return findBestKeyMatch();
}

juce::String KeyDetector::getKeyName(int keyNumber) {
    static const char* keyNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    
    if (keyNumber >= 0 && keyNumber < 12) {
        return keyNames[keyNumber];
    }
    
    return "Unknown";
}

void KeyDetector::calculateChroma(const juce::AudioBuffer<float>& buffer, double sampleRate) {
    // Simplified chroma calculation
    // For production, use proper FFT-based chroma extraction

    m_chromaProfile.fill(0.0f);

    // This is a placeholder - real implementation would use FFT
    // and map frequency bins to pitch classes
}

int KeyDetector::findBestKeyMatch() const {
    // Compare with major key profiles using correlation
    float bestCorrelation = -1.0f;
    int bestKey = -1;

    for (int key = 0; key < 12; ++key) {
        float correlation = 0.0f;
        
        for (int i = 0; i < 12; ++i) {
            int noteIndex = (i + key) % 12;
            correlation += m_chromaProfile[noteIndex] * kMajorProfile[i];
        }

        if (correlation > bestCorrelation) {
            bestCorrelation = correlation;
            bestKey = key;
        }
    }

    return bestKey;
}

} // namespace omega
