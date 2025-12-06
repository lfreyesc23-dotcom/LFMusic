#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Waveform Thumbnail Preview Component
 */
class WaveformPreview : public juce::Component,
                         private juce::ChangeListener
{
public:
    WaveformPreview()
        : thumbnail(512, formatManager, thumbnailCache)
    {
        thumbnail.addChangeListener(this);
        formatManager.registerBasicFormats();
    }

    ~WaveformPreview() override
    {
        thumbnail.removeChangeListener(this);
    }

    void setFile(const juce::File& file)
    {
        currentFile = file;
        thumbnail.setSource(new juce::FileInputSource(file));
        
        if (auto* reader = formatManager.createReaderFor(file))
        {
            sampleRate = reader->sampleRate;
            bitDepth = reader->bitsPerSample;
            numChannels = reader->numChannels;
            lengthInSamples = reader->lengthInSamples;
            delete reader;
        }
        
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        
        if (thumbnail.getTotalLength() > 0.0)
        {
            // Draw waveform
            g.setColour(juce::Colours::orange);
            thumbnail.drawChannels(g, getLocalBounds().reduced(2),
                                 0.0, thumbnail.getTotalLength(), 1.0f);
            
            // Draw info text
            g.setColour(juce::Colours::white);
            g.setFont(12.0f);
            
            juce::String info;
            info << currentFile.getFileName() << "\n";
            info << juce::String(sampleRate / 1000.0, 1) << " kHz, ";
            info << bitDepth << " bit, ";
            info << (numChannels == 1 ? "Mono" : "Stereo") << "\n";
            info << juce::String(lengthInSamples / sampleRate, 2) << " sec";
            
            g.drawMultiLineText(info, 10, getHeight() - 40, getWidth() - 20);
        }
        else
        {
            g.setColour(juce::Colours::grey);
            g.drawText("No file selected", getLocalBounds(), juce::Justification::centred);
        }
    }

    double getSampleRate() const { return sampleRate; }
    int getBitDepth() const { return bitDepth; }
    int getNumChannels() const { return numChannels; }
    int64_t getLengthInSamples() const { return lengthInSamples; }

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override
    {
        repaint();
    }

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 5 };
    juce::AudioThumbnail thumbnail;
    juce::File currentFile;
    
    double sampleRate = 0.0;
    int bitDepth = 0;
    int numChannels = 0;
    int64_t lengthInSamples = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformPreview)
};

//==============================================================================
/**
 * File Tree Item with Icons
 */
class BrowserTreeItem : public juce::TreeViewItem
{
public:
    BrowserTreeItem(const juce::File& f, bool isDir)
        : file(f), isDirectory(isDir)
    {
    }

    bool mightContainSubItems() override
    {
        return isDirectory;
    }

    void paintItem(juce::Graphics& g, int width, int height) override
    {
        if (isSelected())
            g.fillAll(juce::Colours::orange.withAlpha(0.3f));

        g.setColour(juce::Colours::white);
        
        // Draw icon
        auto iconBounds = juce::Rectangle<float>(4.0f, 2.0f, height - 4.0f, height - 4.0f);
        
        if (isDirectory)
        {
            g.setColour(juce::Colours::yellow);
            g.fillRect(iconBounds);
        }
        else
        {
            g.setColour(juce::Colours::lightblue);
            g.fillRoundedRectangle(iconBounds, 2.0f);
        }
        
        // Draw filename
        g.setColour(juce::Colours::white);
        g.drawText(file.getFileName(), 
                  height + 4, 0, width - height - 8, height,
                  juce::Justification::centredLeft, true);
    }

    void itemOpened(bool isNowOpen)
    {
        if (isNowOpen && getNumSubItems() == 0)
            refreshSubItems();
    }

    void itemClicked(const juce::MouseEvent& e)
    {
        if (e.mods.isLeftButtonDown() && !isDirectory)
        {
            if (onFileSelected)
                onFileSelected(file);
        }
    }

    void itemDoubleClicked(const juce::MouseEvent&)
    {
        if (!isDirectory && onFileDoubleClicked)
            onFileDoubleClicked(file);
    }

    std::function<void(const juce::File&)> onFileSelected;
    std::function<void(const juce::File&)> onFileDoubleClicked;

private:
    void refreshSubItems()
    {
        clearSubItems();
        
        if (isDirectory)
        {
            auto files = file.findChildFiles(juce::File::findFilesAndDirectories, false);
            
            for (auto& f : files)
            {
                auto* item = new BrowserTreeItem(f, f.isDirectory());
                item->onFileSelected = onFileSelected;
                item->onFileDoubleClicked = onFileDoubleClicked;
                addSubItem(item);
            }
        }
    }

    juce::File file;
    bool isDirectory;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserTreeItem)
};

//==============================================================================
/**
 * Browser Component - FL Studio style file browser
 */
class BrowserComponent : public juce::Component,
                         private juce::Timer
{
public:
    BrowserComponent()
    {
        setupUI();
        loadDefaultLocations();
        startTimerHz(10); // Preview playback updates
    }

    ~BrowserComponent() override
    {
        stopTimer();
        stopPreview();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Search bar at top
        searchBox.setBounds(bounds.removeFromTop(30).reduced(5));
        
        // Favorites bar
        auto favArea = bounds.removeFromTop(30);
        int favWidth = favArea.getWidth() / favoriteButtons.size();
        for (auto* btn : favoriteButtons)
        {
            btn->setBounds(favArea.removeFromLeft(favWidth).reduced(2));
        }
        
        // Split: Tree view (60%) | Preview (40%)
        auto treeArea = bounds.removeFromLeft(static_cast<int>(bounds.getWidth() * 0.6f));
        
        treeView.setBounds(treeArea);
        
        // Preview area
        auto previewBounds = bounds.reduced(5);
        waveformPreview.setBounds(previewBounds.removeFromTop(previewBounds.getHeight() - 80));
        
        // Preview controls
        auto controlsArea = previewBounds.removeFromTop(40).reduced(5);
        playPreviewButton.setBounds(controlsArea.removeFromLeft(60));
        controlsArea.removeFromLeft(5);
        stopPreviewButton.setBounds(controlsArea.removeFromLeft(60));
        controlsArea.removeFromLeft(10);
        autoPreviewToggle.setBounds(controlsArea);
        
        // File info
        fileInfoLabel.setBounds(previewBounds);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
    }

private:
    void setupUI()
    {
        // Search box
        addAndMakeVisible(searchBox);
        searchBox.setTextToShowWhenEmpty("Search files...", juce::Colours::grey);
        searchBox.onTextChange = [this]() {
            filterTree(searchBox.getText());
        };
        
        // Tree view
        addAndMakeVisible(treeView);
        treeView.setDefaultOpenness(false);
        treeView.setMultiSelectEnabled(false);
        
        // Waveform preview
        addAndMakeVisible(waveformPreview);
        
        // Preview controls
        addAndMakeVisible(playPreviewButton);
        playPreviewButton.setButtonText("Play");
        playPreviewButton.onClick = [this]() { playPreview(); };
        
        addAndMakeVisible(stopPreviewButton);
        stopPreviewButton.setButtonText("Stop");
        stopPreviewButton.onClick = [this]() { stopPreview(); };
        
        addAndMakeVisible(autoPreviewToggle);
        autoPreviewToggle.setButtonText("Auto Preview");
        autoPreviewToggle.setClickingTogglesState(true);
        autoPreviewToggle.setToggleState(true, juce::dontSendNotification);
        
        // File info
        addAndMakeVisible(fileInfoLabel);
        fileInfoLabel.setJustificationType(juce::Justification::topLeft);
        fileInfoLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        
        // Format manager
        formatManager.registerBasicFormats();
    }

    void loadDefaultLocations()
    {
        // Add favorite locations
        auto desktopDir = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
        auto documentsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
        auto musicDir = juce::File::getSpecialLocation(juce::File::userMusicDirectory);
        
        addFavoriteLocation("Desktop", desktopDir);
        addFavoriteLocation("Documents", documentsDir);
        addFavoriteLocation("Music", musicDir);
        
        // Default to Music folder
        setRootDirectory(musicDir);
    }

    void addFavoriteLocation(const juce::String& name, const juce::File& directory)
    {
        auto* button = new juce::TextButton(name);
        button->onClick = [this, directory]() {
            setRootDirectory(directory);
        };
        addAndMakeVisible(button);
        favoriteButtons.add(button);
    }

    void setRootDirectory(const juce::File& directory)
    {
        if (!directory.exists() || !directory.isDirectory())
            return;
        
        currentRoot = directory;
        
        auto* rootItem = new BrowserTreeItem(directory, true);
        rootItem->onFileSelected = [this](const juce::File& file) {
            handleFileSelected(file);
        };
        rootItem->onFileDoubleClicked = [this](const juce::File& file) {
            handleFileDoubleClicked(file);
        };
        
        treeView.setRootItem(rootItem);
        rootItem->setOpen(true);
    }

    void handleFileSelected(const juce::File& file)
    {
        selectedFile = file;
        
        if (file.existsAsFile())
        {
            // Update waveform preview
            waveformPreview.setFile(file);
            
            // Update file info
            juce::String info;
            info << "File: " << file.getFileName() << "\n";
            info << "Size: " << juce::File::descriptionOfSizeInBytes(file.getSize()) << "\n";
            info << "Modified: " << file.getLastModificationTime().toString(true, true);
            fileInfoLabel.setText(info, juce::dontSendNotification);
            
            // Auto preview if enabled
            if (autoPreviewToggle.getToggleState())
            {
                juce::Timer::callAfterDelay(100, [this]() { playPreview(); });
            }
        }
    }

    void handleFileDoubleClicked(const juce::File& file)
    {
        DBG("Double clicked: " << file.getFullPathName());
        // This would trigger drag-to-playlist or insert into project
        if (onFileActivated)
            onFileActivated(file);
    }

    void filterTree(const juce::String& searchText)
    {
        // TODO: Implement search filtering
        DBG("Filtering for: " << searchText);
    }

    void playPreview()
    {
        stopPreview();
        
        if (selectedFile.existsAsFile())
        {
            auto* reader = formatManager.createReaderFor(selectedFile);
            if (reader != nullptr)
            {
                previewSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
                previewPlayer.setSource(previewSource.get());
                
                deviceManager.initialiseWithDefaultDevices(0, 2);
                previewPlayer.start();
            }
        }
    }

    void stopPreview()
    {
        if (previewSource != nullptr)
        {
            previewPlayer.stop();
            previewPlayer.setSource(nullptr);
            previewSource.reset();
            deviceManager.closeAudioDevice();
        }
    }

    void timerCallback() override
    {
        // Update preview playback position if needed
    }

    // UI Components
    juce::TextEditor searchBox;
    juce::TreeView treeView;
    juce::OwnedArray<juce::TextButton> favoriteButtons;
    
    WaveformPreview waveformPreview;
    juce::TextButton playPreviewButton;
    juce::TextButton stopPreviewButton;
    juce::ToggleButton autoPreviewToggle;
    juce::Label fileInfoLabel;
    
    // Audio preview
    juce::AudioFormatManager formatManager;
    juce::AudioDeviceManager deviceManager;
    juce::AudioTransportSource previewPlayer;
    std::unique_ptr<juce::AudioFormatReaderSource> previewSource;
    
    juce::File currentRoot;
    juce::File selectedFile;

public:
    std::function<void(const juce::File&)> onFileActivated;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserComponent)
};

//==============================================================================
/**
 * Browser Window
 */
class BrowserWindow : public juce::DocumentWindow
{
public:
    BrowserWindow()
        : DocumentWindow("Browser",
                        juce::Colours::darkgrey,
                        DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(&browser, true);
        setResizable(true, false);
        centreWithSize(900, 600);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    BrowserComponent browser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserWindow)
};

} // namespace GUI
} // namespace OmegaStudio
