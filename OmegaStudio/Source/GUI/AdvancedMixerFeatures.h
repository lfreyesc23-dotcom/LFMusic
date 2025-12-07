//==============================================================================
// AdvancedMixerFeatures.h
// FL Studio 2025 Advanced Mixer Features
// Presets, FX dock panel, send levels visuales, sidechain routing visual
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <map>
#include <vector>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Mixer Preset - Estado completo del mixer guardable
//==============================================================================
struct MixerPreset {
    juce::String name;
    struct ChannelState {
        float volume = 1.0f;
        float pan = 0.0f;
        bool muted = false;
        bool soloed = false;
        std::vector<juce::String> effects; // IDs de effects activos
        std::map<int, float> sendLevels; // Send index -> level
    };
    std::map<int, ChannelState> channels;
    
    juce::var toVar() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", name);
        
        juce::Array<juce::var> channelsArray;
        for (const auto& pair : channels) {
            auto* channelObj = new juce::DynamicObject();
            channelObj->setProperty("index", pair.first);
            channelObj->setProperty("volume", pair.second.volume);
            channelObj->setProperty("pan", pair.second.pan);
            channelObj->setProperty("muted", pair.second.muted);
            channelObj->setProperty("soloed", pair.second.soloed);
            channelsArray.add(juce::var(channelObj));
        }
        obj->setProperty("channels", channelsArray);
        
        return juce::var(obj);
    }
};

//==============================================================================
// FX Dock Panel - Panel dockeable para FX chain
//==============================================================================
class FXDockPanel : public juce::Component {
public:
    FXDockPanel() {
        addAndMakeVisible(header);
        header.setText("FX CHAIN", juce::dontSendNotification);
        header.setJustificationType(juce::Justification::centred);
        header.setColour(juce::Label::backgroundColourId, juce::Colour(0xff3a3a3a));
        header.setColour(juce::Label::textColourId, juce::Colours::white);
        
        addFXButton.setButtonText("+ ADD FX");
        addFXButton.onClick = [this] { showFXBrowser(); };
        addAndMakeVisible(addFXButton);
        
        setSize(300, 600);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2b2b2b));
        
        // Slots para FX
        auto bounds = getLocalBounds().reduced(8);
        bounds.removeFromTop(56); // Header + add button
        
        g.setColour(juce::Colour(0xff1a1a1a));
        for (int i = 0; i < 10; ++i) {
            auto slotBounds = bounds.removeFromTop(50);
            g.fillRoundedRectangle(slotBounds.toFloat(), 4.0f);
            
            // Slot number
            g.setColour(juce::Colour(0xff6a6a6a));
            g.setFont(juce::Font(10.0f, juce::Font::bold));
            g.drawText(juce::String(i + 1), slotBounds.removeFromLeft(24), 
                      juce::Justification::centred);
            
            // Empty slot text
            g.setColour(juce::Colour(0xff4a4a4a));
            g.setFont(juce::Font(11.0f));
            g.drawText("Empty", slotBounds, juce::Justification::centred);
            
            bounds.removeFromTop(4);
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(8);
        header.setBounds(bounds.removeFromTop(28));
        bounds.removeFromTop(4);
        addFXButton.setBounds(bounds.removeFromTop(24));
    }
    
private:
    juce::Label header;
    juce::TextButton addFXButton;
    
    void showFXBrowser() {
        // Show FX browser dialog
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXDockPanel)
};

//==============================================================================
// Send Level Meter - Medidor visual de send levels
//==============================================================================
class SendLevelMeter : public juce::Component,
                       private juce::Timer {
public:
    SendLevelMeter(int sendIndex) : index(sendIndex) {
        startTimer(30); // 30ms refresh
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds, 2.0f);
        
        // Level bar
        float levelHeight = bounds.getHeight() * level;
        auto levelBounds = bounds.removeFromBottom(levelHeight);
        
        juce::Colour levelColour = juce::Colour(0xff36c9ff);
        g.setColour(levelColour);
        g.fillRoundedRectangle(levelBounds, 2.0f);
        
        // Peak indicator
        if (peakLevel > 0.01f) {
            float peakY = bounds.getBottom() - (bounds.getHeight() * peakLevel);
            g.setColour(levelColour.brighter(0.4f));
            g.fillRect(bounds.getX(), peakY - 1, bounds.getWidth(), 2.0f);
        }
        
        // Border
        g.setColour(juce::Colour(0xff0a0a0a));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 2.0f, 1.0f);
        
        // Send label
        g.setColour(juce::Colour(0xff9a9a9a));
        g.setFont(juce::Font(8.0f, juce::Font::bold));
        g.drawText(juce::String(index + 1), getLocalBounds().removeFromTop(12), 
                  juce::Justification::centred);
    }
    
    void setLevel(float newLevel) {
        level = juce::jlimit(0.0f, 1.0f, newLevel);
        if (level > peakLevel) {
            peakLevel = level;
            peakHoldCounter = 0;
        }
        repaint();
    }
    
private:
    int index;
    float level = 0.0f;
    float peakLevel = 0.0f;
    int peakHoldCounter = 0;
    
    void timerCallback() override {
        // Peak hold decay
        if (++peakHoldCounter > 60) {
            peakLevel *= 0.97f;
            if (peakLevel < 0.01f) peakLevel = 0.0f;
            repaint();
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendLevelMeter)
};

//==============================================================================
// Sidechain Routing Visualizer - Visualización de ruteo sidechain
//==============================================================================
class SidechainRoutingVisualizer : public juce::Component {
public:
    struct SidechainConnection {
        int sourceChannel;
        int targetChannel;
        juce::Colour colour = juce::Colour(0xffff8736);
    };
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        // Draw connections
        for (const auto& connection : connections) {
            drawConnection(g, connection);
        }
        
        // Draw channel indicators
        drawChannelIndicators(g);
    }
    
    void addConnection(int source, int target) {
        SidechainConnection conn;
        conn.sourceChannel = source;
        conn.targetChannel = target;
        connections.push_back(conn);
        repaint();
    }
    
    void clearConnections() {
        connections.clear();
        repaint();
    }
    
private:
    std::vector<SidechainConnection> connections;
    int numChannels = 128;
    
    void drawConnection(juce::Graphics& g, const SidechainConnection& conn) {
        float sourceX = getChannelX(conn.sourceChannel);
        float targetX = getChannelX(conn.targetChannel);
        float y = (float)getHeight() / 2;
        
        // Draw curved line
        juce::Path path;
        path.startNewSubPath(sourceX, 10);
        path.cubicTo(sourceX, y, targetX, y, targetX, (float)getHeight() - 10);
        
        // Animated glow effect
        g.setColour(conn.colour.withAlpha(0.3f));
        g.strokePath(path, juce::PathStrokeType(4.0f));
        
        g.setColour(conn.colour);
        g.strokePath(path, juce::PathStrokeType(2.0f));
        
        // Arrow at target
        drawArrow(g, targetX, (float)getHeight() - 10, conn.colour);
    }
    
    void drawChannelIndicators(juce::Graphics& g) {
        for (int i = 0; i < numChannels; ++i) {
            float x = getChannelX(i);
            
            // Top indicator
            g.setColour(juce::Colour(0xff3a3a3a));
            g.fillEllipse(x - 4, 6, 8, 8);
            
            // Bottom indicator
            g.fillEllipse(x - 4, (float)getHeight() - 14, 8, 8);
            
            // Channel number (every 8 channels)
            if (i % 8 == 0) {
                g.setColour(juce::Colour(0xff9a9a9a));
                g.setFont(juce::Font(8.0f));
                g.drawText(juce::String(i + 1), (int)x - 10, 16, 20, 12, 
                          juce::Justification::centred);
            }
        }
    }
    
    void drawArrow(juce::Graphics& g, float x, float y, juce::Colour colour) {
        juce::Path arrow;
        arrow.startNewSubPath(x, y);
        arrow.lineTo(x - 4, y - 6);
        arrow.lineTo(x + 4, y - 6);
        arrow.closeSubPath();
        
        g.setColour(colour);
        g.fillPath(arrow);
    }
    
    float getChannelX(int channelIndex) const {
        return (float)getWidth() * ((float)channelIndex / (float)numChannels);
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainRoutingVisualizer)
};

//==============================================================================
// Mixer Snapshot Manager - Sistema de snapshots con morphing
//==============================================================================
class MixerSnapshotManager {
public:
    struct Snapshot {
        juce::String name;
        MixerPreset preset;
        juce::Colour colour = juce::Colour(0xff4a90ff);
        juce::int64 timestamp;
    };
    
    void captureSnapshot(const juce::String& name, const MixerPreset& preset) {
        Snapshot snapshot;
        snapshot.name = name;
        snapshot.preset = preset;
        snapshot.timestamp = juce::Time::currentTimeMillis();
        
        snapshots.push_back(snapshot);
        
        if (onSnapshotCaptured) {
            onSnapshotCaptured(snapshot);
        }
    }
    
    void recallSnapshot(size_t index, float morphTime = 0.0f) {
        if (index < snapshots.size()) {
            if (onSnapshotRecalled) {
                onSnapshotRecalled(snapshots[index], morphTime);
            }
        }
    }
    
    void morphBetweenSnapshots(size_t from, size_t to, float amount) {
        if (from < snapshots.size() && to < snapshots.size()) {
            // Interpolate between two snapshots
            if (onSnapshotMorph) {
                onSnapshotMorph(snapshots[from], snapshots[to], amount);
            }
        }
    }
    
    const std::vector<Snapshot>& getSnapshots() const { return snapshots; }
    
    std::function<void(const Snapshot&)> onSnapshotCaptured;
    std::function<void(const Snapshot&, float)> onSnapshotRecalled;
    std::function<void(const Snapshot&, const Snapshot&, float)> onSnapshotMorph;
    
private:
    std::vector<Snapshot> snapshots;
};

//==============================================================================
// Mixer Preset Browser - Browser de mixer presets
//==============================================================================
class MixerPresetBrowser : public juce::Component {
public:
    MixerPresetBrowser() {
        addAndMakeVisible(presetList);
        presetList.setModel(this);
        
        saveButton.setButtonText("💾 Save");
        saveButton.onClick = [this] { saveCurrentPreset(); };
        addAndMakeVisible(saveButton);
        
        loadButton.setButtonText("📂 Load");
        loadButton.onClick = [this] { loadSelectedPreset(); };
        addAndMakeVisible(loadButton);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2b2b2b));
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(8);
        
        auto buttonArea = bounds.removeFromBottom(32);
        saveButton.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2 - 4));
        buttonArea.removeFromLeft(8);
        loadButton.setBounds(buttonArea);
        
        bounds.removeFromBottom(8);
        presetList.setBounds(bounds);
    }
    
private:
    juce::ListBox presetList;
    juce::TextButton saveButton;
    juce::TextButton loadButton;
    
    void saveCurrentPreset() {
        // Save dialog
    }
    
    void loadSelectedPreset() {
        // Load selected
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPresetBrowser)
};

} // namespace GUI
} // namespace OmegaStudio
