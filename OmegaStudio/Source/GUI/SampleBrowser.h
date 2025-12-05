#pragma once

#include <JuceHeader.h>
#include "../Audio/Library/SampleManager.h"
#include <vector>
#include <memory>
#include <functional>

namespace OmegaStudio {
namespace GUI {

/**
 * @class WaveformThumbnail
 * @brief Renderiza waveform thumbnail de un sample
 */
class WaveformThumbnail : public juce::Component {
public:
    WaveformThumbnail() {
        thumbnail_ = std::make_unique<juce::AudioThumbnail>(
            128, formatManager_, thumbnailCache_
        );
        
        formatManager_.registerBasicFormats();
    }
    
    void setFile(const juce::File& file) {
        thumbnail_->setSource(new juce::FileInputSource(file));
    }
    
    void setAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate) {
        thumbnail_->reset(1, sampleRate);
        thumbnail_->addBlock(0, buffer, 0, buffer.getNumSamples());
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1a1a1a));
        
        if (thumbnail_->getNumChannels() > 0) {
            g.setColour(juce::Colours::cyan);
            thumbnail_->drawChannels(g, getLocalBounds().reduced(2), 
                                    0.0, thumbnail_->getTotalLength(), 1.0f);
        }
    }
    
private:
    juce::AudioFormatManager formatManager_;
    juce::AudioThumbnailCache thumbnailCache_ { 2 };
    std::unique_ptr<juce::AudioThumbnail> thumbnail_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformThumbnail)
};

/**
 * @class SampleListItem
 * @brief Item individual en la lista de samples
 */
class SampleListItem : public juce::Component {
public:
    SampleListItem(const SimpleSampleData& sample) 
        : sample_(sample) {
        
        thumbnail_.setFile(juce::File(sample_.filePath));
        addAndMakeVisible(thumbnail_);
        setSize(300, 60);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds();
        
        // Background
        if (isMouseOver_ || selected_) {
            g.setColour(juce::Colour(0xff2a4a6a));
        } else {
            g.setColour(juce::Colour(0xff1a1a1a));
        }
        g.fillRect(bounds);
        
        // Info text area
        auto textArea = bounds.removeFromLeft(150);
        
        g.setColour(juce::Colours::white);
        g.setFont(14.0f);
        g.drawText(sample_.name, 
                  textArea.removeFromTop(20).reduced(5), 
                  juce::Justification::centredLeft);
        
        // Metadata
        g.setFont(11.0f);
        g.setColour(juce::Colours::grey);
        
        juce::String info = juce::String(sample_.bpm, 1) + " BPM | " + sample_.key;
        g.drawText(info, textArea.removeFromTop(18).reduced(5), 
                  juce::Justification::centredLeft);
        
        // Category
        g.drawText(sample_.category, 
                  textArea.reduced(5), 
                  juce::Justification::centredLeft);
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        bounds.removeFromLeft(150);  // Skip text area
        thumbnail_.setBounds(bounds.reduced(2));
    }
    
    void mouseEnter(const juce::MouseEvent&) override {
        isMouseOver_ = true;
        repaint();
    }
    
    void mouseExit(const juce::MouseEvent&) override {
        isMouseOver_ = false;
        repaint();
    }
    
    void setSelected(bool selected) {
        selected_ = selected;
        repaint();
    }
    
    bool isSelected() const { return selected_; }
    const ::OmegaStudio::Audio::Library::Sample& getSample() const { return sample_; }
    
private:
    ::OmegaStudio::Audio::Library::Sample sample_;
    WaveformThumbnail thumbnail_;
    bool isMouseOver_ { false };
    bool selected_ { false };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleListItem)
};

/**
 * @class SamplePreviewPlayer
 * @brief Reproductor de preview para samples
 */
class SamplePreviewPlayer : public juce::AudioSource {
public:
    SamplePreviewPlayer() {
        formatManager_.registerBasicFormats();
    }
    
    ~SamplePreviewPlayer() override {
        stop();
    }
    
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        transportSource_.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }
    
    void releaseResources() override {
        transportSource_.releaseResources();
    }
    
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override {
        if (!isPlaying_) {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        
        transportSource_.getNextAudioBlock(bufferToFill);
        
        // Auto-stop when finished
        if (!transportSource_.isPlaying()) {
            stop();
        }
    }
    
    void loadSample(const juce::File& file) {
        stop();
        
        auto* reader = formatManager_.createReaderFor(file);
        if (reader != nullptr) {
            auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
            transportSource_.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
            readerSource_ = std::move(newSource);
            currentFile_ = file;
        }
    }
    
    void play() {
        if (readerSource_ != nullptr) {
            transportSource_.setPosition(0.0);
            transportSource_.start();
            isPlaying_ = true;
        }
    }
    
    void stop() {
        transportSource_.stop();
        isPlaying_ = false;
    }
    
    void togglePlayPause() {
        if (isPlaying_) {
            stop();
        } else {
            play();
        }
    }
    
    bool isPlaying() const { return isPlaying_; }
    double getCurrentPosition() const { return transportSource_.getCurrentPosition(); }
    double getTotalLength() const { return transportSource_.getLengthInSeconds(); }
    
private:
    juce::AudioFormatManager formatManager_;
    juce::AudioTransportSource transportSource_;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource_;
    juce::File currentFile_;
    bool isPlaying_ { false };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplePreviewPlayer)
};

/**
 * @class SampleBrowserComponent
 * @brief Navegador de samples con categorías, búsqueda, preview y drag & drop
 */
class SampleBrowserComponent : public juce::Component,
                                public juce::TextEditor::Listener,
                                public juce::Button::Listener,
                                public juce::Timer {
public:
    SampleBrowserComponent() {
        
        // Search box
        searchBox_.setTextToShowWhenEmpty("Search samples...", juce::Colours::grey);
        searchBox_.addListener(this);
        addAndMakeVisible(searchBox_);
        
        // Category filter buttons
        addCategoryButton("All", "");
        addCategoryButton("Drums", "Drums");
        addCategoryButton("Bass", "Bass");
        addCategoryButton("Synth", "Synth");
        addCategoryButton("FX", "FX");
        addCategoryButton("Vocals", "Vocals");
        addCategoryButton("Loops", "Loops");
        
        // Sort combo
        sortCombo_.addItem("Name", 1);
        sortCombo_.addItem("BPM", 2);
        sortCombo_.addItem("Key", 3);
        sortCombo_.addItem("Date Added", 4);
        sortCombo_.setSelectedId(1);
        sortCombo_.onChange = [this]() { refreshSampleList(); };
        addAndMakeVisible(sortCombo_);
        
        // Viewport for samples
        viewport_.setViewedComponent(&sampleContainer_, false);
        viewport_.setScrollBarsShown(true, false);
        addAndMakeVisible(viewport_);
        
        // Preview player controls
        playButton_.setButtonText("►");
        playButton_.addListener(this);
        addAndMakeVisible(playButton_);
        
        stopButton_.setButtonText("■");
        stopButton_.addListener(this);
        addAndMakeVisible(stopButton_);
        
        // Audio device for preview
        audioDeviceManager_.initialiseWithDefaultDevices(0, 2);
        audioSourcePlayer_.setSource(&previewPlayer_);
        audioDeviceManager_.addAudioCallback(&audioSourcePlayer_);
        
        // Refresh initial list
        refreshSampleList();
        
        // Start timer for playback updates
        startTimer(50);
        
        setSize(400, 600);
    }
    
    ~SampleBrowserComponent() override {
        audioDeviceManager_.removeAudioCallback(&audioSourcePlayer_);
        audioSourcePlayer_.setSource(nullptr);
        stopTimer();
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff0a0a0a));
        
        // Preview section background
        auto previewArea = getPreviewArea();
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(previewArea);
        g.setColour(juce::Colour(0xff2a2a2a));
        g.drawRect(previewArea);
        
        // Preview info
        if (selectedSample_ != nullptr) {
            g.setColour(juce::Colours::white);
            g.setFont(14.0f);
            
            auto textArea = previewArea.reduced(10);
            textArea.removeFromTop(40);  // Skip controls
            
            auto metadata = selectedSample_->getSample().getMetadata();
            g.drawText("BPM: " + juce::String(metadata.bpm, 1), 
                      textArea.removeFromTop(20), 
                      juce::Justification::centredLeft);
            g.drawText("Key: " + juce::String(metadata.key), 
                      textArea.removeFromTop(20), 
                      juce::Justification::centredLeft);
            g.drawText("Category: " + juce::String(metadata.category), 
                      textArea.removeFromTop(20), 
                      juce::Justification::centredLeft);
            
            // Playback progress
            if (previewPlayer_.isPlaying()) {
                double progress = previewPlayer_.getCurrentPosition() / 
                                std::max(0.001, previewPlayer_.getTotalLength());
                auto progressBar = textArea.removeFromTop(30).reduced(0, 10);
                
                g.setColour(juce::Colour(0xff2a2a2a));
                g.fillRect(progressBar);
                
                g.setColour(juce::Colours::cyan);
                g.fillRect(progressBar.withWidth(progressBar.getWidth() * progress));
            }
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(5);
        
        // Search box
        auto searchArea = bounds.removeFromTop(30);
        searchBox_.setBounds(searchArea);
        
        bounds.removeFromTop(5);
        
        // Category buttons
        auto categoryArea = bounds.removeFromTop(30);
        int buttonWidth = categoryArea.getWidth() / categoryButtons_.size();
        for (auto* button : categoryButtons_) {
            button->setBounds(categoryArea.removeFromLeft(buttonWidth).reduced(2));
        }
        
        bounds.removeFromTop(5);
        
        // Sort combo
        auto sortArea = bounds.removeFromTop(25);
        sortCombo_.setBounds(sortArea);
        
        bounds.removeFromTop(5);
        
        // Preview area at bottom
        auto previewArea = bounds.removeFromBottom(150);
        
        auto controlsArea = previewArea.removeFromTop(40).reduced(10);
        playButton_.setBounds(controlsArea.removeFromLeft(50));
        controlsArea.removeFromLeft(5);
        stopButton_.setBounds(controlsArea.removeFromLeft(50));
        
        // Sample list viewport
        viewport_.setBounds(bounds);
        
        // Update container height
        updateSampleContainerSize();
    }
    
    void textEditorTextChanged(juce::TextEditor& editor) override {
        if (&editor == &searchBox_) {
            refreshSampleList();
        }
    }
    
    void buttonClicked(juce::Button* button) override {
        if (button == &playButton_) {
            if (selectedSample_ != nullptr) {
                previewPlayer_.play();
                repaint();
            }
        } else if (button == &stopButton_) {
            previewPlayer_.stop();
            repaint();
        } else {
            // Category button
            for (auto* catButton : categoryButtons_) {
                if (button == catButton) {
                    currentCategory_ = catButton->getName();
                    refreshSampleList();
                    break;
                }
            }
        }
    }
    
    void timerCallback() override {
        if (previewPlayer_.isPlaying()) {
            repaint(getPreviewArea());
        }
    }
    
    // Drag & Drop source
    void mouseDrag(const juce::MouseEvent& e) override {
        if (selectedSample_ != nullptr && e.getDistanceFromDragStart() > 10) {
            // Start drag & drop
            juce::DragAndDropContainer::performExternalDragDropOfFiles(
                {selectedSample_->getSample().getFilePath()},
                true, this
            );
        }
    }
    
private:
    void addCategoryButton(const juce::String& label, const juce::String& category) {
        auto* button = new juce::TextButton(label);
        button->setName(category);
        button->addListener(this);
        categoryButtons_.add(button);
        addAndMakeVisible(button);
    }
    
    void refreshSampleList() {
        // Clear existing items
        sampleContainer_.deleteAllChildren();
        sampleItems_.clear();
        
        // Get filtered samples (stub - integrate with your sample library)
        std::vector<SimpleSampleData> samples;
        // TODO: Populate from actual sample library
        
        // Sort
        sortSamples(samples);
        
        // Create list items
        int yPos = 0;
        for (const auto& sample : samples) {
            auto* item = new SampleListItem(sample);
            item->setTopLeftPosition(0, yPos);
            item->addMouseListener(this, false);
            sampleItems_.add(item);
            sampleContainer_.addAndMakeVisible(item);
            yPos += item->getHeight() + 2;
        }
        
        updateSampleContainerSize();
    }
    
    void sortSamples(std::vector<SimpleSampleData>& samples) {
        int sortMode = sortCombo_.getSelectedId();
        
        std::sort(samples.begin(), samples.end(), 
            [sortMode](const SimpleSampleData& a, const SimpleSampleData& b) {
                switch (sortMode) {
                    case 1: return a.name < b.name;  // Name
                    case 2: return a.bpm < b.bpm;    // BPM
                    case 3: return a.key < b.key;    // Key
                    default: return false;
                }
            }
        );
    }
    
    void updateSampleContainerSize() {
        int totalHeight = 0;
        for (auto* item : sampleItems_) {
            totalHeight += item->getHeight() + 2;
        }
        sampleContainer_.setSize(viewport_.getWidth() - 20, totalHeight);
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        // Check if clicked on a sample item
        for (auto* item : sampleItems_) {
            if (item->getBounds().contains(e.getEventRelativeTo(&sampleContainer_).getPosition())) {
                selectSample(item);
                break;
            }
        }
    }
    
    void mouseDoubleClick(const juce::MouseEvent& e) override {
        // Double click to preview
        if (selectedSample_ != nullptr) {
            previewPlayer_.togglePlayPause();
            repaint();
        }
    }
    
    void selectSample(SampleListItem* item) {
        if (selectedSample_ != nullptr) {
            selectedSample_->setSelected(false);
        }
        
        selectedSample_ = item;
        selectedSample_->setSelected(true);
        
        // Load into preview player
        juce::File file(item->getSample().filePath);
        previewPlayer_.loadSample(file);
        
        repaint();
    }
    
    juce::Rectangle<int> getPreviewArea() const {
        return getLocalBounds().removeFromBottom(150).reduced(5);
    }
    
    // UI Components
    juce::TextEditor searchBox_;
    juce::ComboBox sortCombo_;
    juce::OwnedArray<juce::TextButton> categoryButtons_;
    juce::Viewport viewport_;
    juce::Component sampleContainer_;
    juce::OwnedArray<SampleListItem> sampleItems_;
    juce::TextButton playButton_;
    juce::TextButton stopButton_;
    
    // State
    juce::String currentCategory_;
    SampleListItem* selectedSample_ { nullptr };
    
    // Audio preview
    SamplePreviewPlayer previewPlayer_;
    juce::AudioDeviceManager audioDeviceManager_;
    juce::AudioSourcePlayer audioSourcePlayer_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBrowserComponent)
};

} // namespace GUI
} // namespace OmegaStudio
