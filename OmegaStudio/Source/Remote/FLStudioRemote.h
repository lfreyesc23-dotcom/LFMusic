#pragma once
#include <JuceHeader.h>
#include <memory>

namespace OmegaStudio {

/**
 * @brief FL Studio Remote - Control DAW from Phone/Tablet (FL Studio 2025)
 * TCP/WebSocket server for remote control
 */
class FLStudioRemote
{
public:
    struct RemoteCommand {
        enum class Type {
            Play,
            Stop,
            Record,
            SetBPM,
            SetVolume,
            SetPan,
            MuteChannel,
            SoloChannel,
            TriggerPattern,
            TriggerClip,
            SetParameter,
            Undo,
            Redo,
            Save,
            GetStatus
        };
        
        Type type;
        juce::var parameters;
    };

    struct ConnectionInfo {
        juce::String clientIP;
        int clientPort;
        juce::String deviceName;
        bool authenticated = false;
        int64_t connectedTime;
    };

    FLStudioRemote();
    ~FLStudioRemote();
    
    // Server control
    bool startServer(int port = 9999);
    void stopServer();
    bool isServerRunning() const { return serverRunning; }
    int getServerPort() const { return serverPort; }
    
    // WebSocket support
    bool startWebSocketServer(int port = 9998);
    void stopWebSocketServer();
    
    // Connections
    std::vector<ConnectionInfo> getConnectedClients() const;
    void disconnectClient(const juce::String& clientIP);
    void disconnectAllClients();
    
    // Authentication
    void setRequireAuthentication(bool require) { requireAuth = require; }
    void setPassword(const juce::String& password) { authPassword = password; }
    
    // Command handling
    void processCommand(const RemoteCommand& command);
    std::function<void(const RemoteCommand&)> onCommandReceived;
    
    // Send updates to clients
    void sendTransportUpdate(bool playing, double position);
    void sendMixerUpdate(int channel, float volume, float pan);
    void sendParameterUpdate(const juce::String& param, float value);
    void sendStatusUpdate(const juce::var& status);
    
    // Discovery (broadcast for auto-discovery)
    void enableDiscovery(bool enable);
    bool isDiscoveryEnabled() const { return discoveryEnabled; }

private:
    class TCPServer;
    class WebSocketServer;
    
    std::unique_ptr<TCPServer> tcpServer;
    std::unique_ptr<WebSocketServer> wsServer;
    
    bool serverRunning = false;
    bool wsServerRunning = false;
    int serverPort = 9999;
    int wsPort = 9998;
    bool requireAuth = false;
    juce::String authPassword;
    bool discoveryEnabled = true;
    
    std::vector<ConnectionInfo> clients;
    
    void broadcastDiscovery();
    bool authenticateClient(const juce::String& clientIP, const juce::String& password);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudioRemote)
};

/**
 * @brief Sound Content Library (FL Studio)
 * Extensive library of samples, loops, and presets
 */
class SoundContentLibrary
{
public:
    enum class ContentType {
        Sample,
        Loop,
        Preset,
        Project,
        MIDIFile,
        AudioEffect,
        Instrument
    };
    
    struct ContentItem {
        juce::String name;
        juce::String path;
        ContentType type;
        juce::String category;
        juce::StringArray tags;
        int bpm = 0;
        juce::String key;
        int duration = 0;  // seconds
        juce::String author;
        int rating = 0;    // 0-5 stars
        bool favorite = false;
        juce::Image thumbnail;
        int64_t fileSize;
        juce::String format;
    };
    
    SoundContentLibrary();
    ~SoundContentLibrary();
    
    // Library management
    void addContentPath(const juce::File& path, bool recursive = true);
    void removeContentPath(const juce::File& path);
    void scanLibrary();
    void clearLibrary();
    
    // Search and filter
    std::vector<ContentItem> search(const juce::String& query);
    std::vector<ContentItem> filterByType(ContentType type);
    std::vector<ContentItem> filterByCategory(const juce::String& category);
    std::vector<ContentItem> filterByTags(const juce::StringArray& tags);
    std::vector<ContentItem> filterByBPM(int minBPM, int maxBPM);
    std::vector<ContentItem> filterByKey(const juce::String& key);
    std::vector<ContentItem> getFavorites();
    
    // Categories
    std::vector<juce::String> getCategories() const;
    std::vector<juce::String> getTags() const;
    
    // Content browser
    struct BrowserSettings {
        ContentType filter = ContentType::Sample;
        juce::String searchQuery;
        juce::String category;
        juce::StringArray tags;
        int sortBy = 0;  // 0=Name, 1=Date, 2=Rating
        bool ascending = true;
        bool showFavoritesOnly = false;
    };
    std::vector<ContentItem> browse(const BrowserSettings& settings);
    
    // Preview
    void previewAudio(const ContentItem& item);
    void stopPreview();
    bool isPreviewing() const { return previewing; }
    
    // Favorites
    void addToFavorites(const ContentItem& item);
    void removeFromFavorites(const ContentItem& item);
    
    // Metadata
    void setRating(const ContentItem& item, int rating);
    void addTag(const ContentItem& item, const juce::String& tag);
    void removeTag(const ContentItem& item, const juce::String& tag);
    
    // FL Cloud integration
    void enableFLCloud(bool enable) { flCloudEnabled = enable; }
    bool isFLCloudEnabled() const { return flCloudEnabled; }
    void syncWithFLCloud();
    void downloadFromFLCloud(const ContentItem& item);
    
    // Pack management
    struct SoundPack {
        juce::String name;
        juce::String description;
        std::vector<ContentItem> items;
        juce::String author;
        juce::Image cover;
        int64_t totalSize;
    };
    void installSoundPack(const juce::File& packFile);
    void uninstallSoundPack(const juce::String& packName);
    std::vector<SoundPack> getInstalledPacks() const;
    
    // Recently used
    std::vector<ContentItem> getRecentlyUsed(int count = 20);
    void addToRecentlyUsed(const ContentItem& item);
    
    // Export
    bool exportSelection(const std::vector<ContentItem>& items, const juce::File& destination);

private:
    std::vector<juce::File> contentPaths;
    std::vector<ContentItem> library;
    std::vector<SoundPack> installedPacks;
    std::vector<ContentItem> recentlyUsed;
    
    bool flCloudEnabled = false;
    bool previewing = false;
    
    // Database
    juce::File databaseFile;
    void loadDatabase();
    void saveDatabase();
    
    // Scanning
    void scanDirectory(const juce::File& dir, bool recursive);
    ContentItem createContentItem(const juce::File& file);
    void analyzeAudioFile(ContentItem& item);
    void extractThumbnail(ContentItem& item);
    
    // Audio preview
    std::unique_ptr<juce::AudioFormatReaderSource> previewSource;
    juce::AudioTransportSource previewTransport;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundContentLibrary)
};

/**
 * @brief Content Browser Component
 */
class ContentBrowserComponent : public juce::Component,
                                public juce::TextEditor::Listener
{
public:
    ContentBrowserComponent(SoundContentLibrary& library);
    ~ContentBrowserComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Callbacks
    std::function<void(const SoundContentLibrary::ContentItem&)> onItemSelected;
    std::function<void(const SoundContentLibrary::ContentItem&)> onItemDoubleClicked;

private:
    SoundContentLibrary& library;
    
    juce::TextEditor searchBox;
    juce::ComboBox typeFilter;
    juce::ComboBox categoryFilter;
    juce::ListBox contentList;
    juce::Label previewLabel;
    juce::TextButton previewButton;
    juce::TextButton favoriteButton;
    
    std::vector<SoundContentLibrary::ContentItem> currentItems;
    
    void updateContentList();
    void textEditorTextChanged(juce::TextEditor&) override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ContentBrowserComponent)
};

} // namespace OmegaStudio
