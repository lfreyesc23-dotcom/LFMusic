#pragma once

#include <JuceHeader.h>
#include "FLStudioLookAndFeel.h"

namespace OmegaStudio {
namespace GUI {

using Omega::GUI::FLColors;

//==============================================================================
/** 
 * Panel lateral de biblioteca estilo FL Studio 2025
 */
class LibraryBrowserPanel : public juce::Component,
                            public juce::FileDragAndDropTarget {
public:
    LibraryBrowserPanel() {
        // Barra de búsqueda
        searchBox_.setTextToShowWhenEmpty("Search sounds...", juce::Colours::grey);
        searchBox_.setColour(juce::TextEditor::backgroundColourId, FLColors::DarkBg);
        searchBox_.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        searchBox_.onTextChange = [this]() { filterContent(); };
        addAndMakeVisible(searchBox_);
        
        // Pestañas de categorías
        categoryTabs_.setTabBarDepth(30);
        categoryTabs_.setColour(juce::TabbedComponent::backgroundColourId, FLColors::PanelBg);
        categoryTabs_.setColour(juce::TabbedComponent::outlineColourId, FLColors::DarkBg);
        
        // Pestaña Beats
        auto* beatsPanel = new juce::Component();
        beatsTree_ = std::make_unique<juce::TreeView>();
        beatsTree_->setColour(juce::TreeView::backgroundColourId, FLColors::DarkBg);
        beatsTree_->setRootItem(createBeatsTree().release());
        beatsTree_->setRootItemVisible(false);
        beatsPanel->addAndMakeVisible(beatsTree_.get());
        auto resizeBeatsFn = [this, beatsPanel]() {
            beatsTree_->setBounds(beatsPanel->getLocalBounds());
        };
        // Llamar directamente en el constructor de la tab
        categoryTabs_.addTab("Beats", FLColors::Orange, beatsPanel, true);
        
        // Pestaña Samples
        auto* samplesPanel = new juce::Component();
        samplesTree_ = std::make_unique<juce::TreeView>();
        samplesTree_->setColour(juce::TreeView::backgroundColourId, FLColors::DarkBg);
        samplesTree_->setRootItem(createSamplesTree().release());
        samplesTree_->setRootItemVisible(false);
        samplesPanel->addAndMakeVisible(samplesTree_.get());
        auto resizeSamplesFn = [this, samplesPanel]() {
            samplesTree_->setBounds(samplesPanel->getLocalBounds());
        };
        categoryTabs_.addTab("Samples", FLColors::Cyan, samplesPanel, true);
        
        // Pestaña Loops
        auto* loopsPanel = new juce::Component();
        loopsTree_ = std::make_unique<juce::TreeView>();
        loopsTree_->setColour(juce::TreeView::backgroundColourId, FLColors::DarkBg);
        loopsTree_->setRootItem(createLoopsTree().release());
        loopsTree_->setRootItemVisible(false);
        loopsPanel->addAndMakeVisible(loopsTree_.get());
        auto resizeLoopsFn = [this, loopsPanel]() {
            loopsTree_->setBounds(loopsPanel->getLocalBounds());
        };
        categoryTabs_.addTab("Loops", FLColors::Green, loopsPanel, true);
        
        // Pestaña Presets
        auto* presetsPanel = new juce::Component();
        presetsTree_ = std::make_unique<juce::TreeView>();
        presetsTree_->setColour(juce::TreeView::backgroundColourId, FLColors::DarkBg);
        presetsTree_->setRootItem(createPresetsTree().release());
        presetsTree_->setRootItemVisible(false);
        presetsPanel->addAndMakeVisible(presetsTree_.get());
        auto resizePresetsFn = [this, presetsPanel]() {
            presetsTree_->setBounds(presetsPanel->getLocalBounds());
        };
        categoryTabs_.addTab("Presets", FLColors::Purple, presetsPanel, true);
        
        // Pestaña Plugins
        auto* pluginsPanel = new juce::Component();
        pluginsTree_ = std::make_unique<juce::TreeView>();
        pluginsTree_->setColour(juce::TreeView::backgroundColourId, FLColors::DarkBg);
        pluginsTree_->setRootItem(createPluginsTree().release());
        pluginsTree_->setRootItemVisible(false);
        pluginsPanel->addAndMakeVisible(pluginsTree_.get());
        auto resizePluginsFn = [this, pluginsPanel]() {
            pluginsTree_->setBounds(pluginsPanel->getLocalBounds());
        };
        categoryTabs_.addTab("Plugins", FLColors::Blue, pluginsPanel, true);
        
        addAndMakeVisible(categoryTabs_);
        
        // Preview de waveform
        previewPanel_.setColour(juce::Label::backgroundColourId, juce::Colours::black);
        previewPanel_.setText("Drag & Drop files here\nor select from library", juce::dontSendNotification);
        previewPanel_.setColour(juce::Label::textColourId, juce::Colours::grey);
        previewPanel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(previewPanel_);
        
        // Botones de acción
        importButton_.setButtonText("Import Files");
        importButton_.setColour(juce::TextButton::buttonColourId, FLColors::Blue);
        importButton_.onClick = [this]() { importFiles(); };
        addAndMakeVisible(importButton_);
        
        scanButton_.setButtonText("Scan Folders");
        scanButton_.setColour(juce::TextButton::buttonColourId, FLColors::Green);
        scanButton_.onClick = [this]() { scanFolders(); };
        addAndMakeVisible(scanButton_);
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(FLColors::PanelBg);
        
        // Título
        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("LIBRARY", 10, 5, getWidth() - 20, 25, juce::Justification::centredLeft);
        
        // Línea separadora derecha
        g.setColour(FLColors::DarkBg);
        g.drawLine(getWidth() - 1, 0, getWidth() - 1, getHeight(), 2.0f);
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(5);
        
        // Título
        bounds.removeFromTop(30);
        
        // Barra de búsqueda
        searchBox_.setBounds(bounds.removeFromTop(30));
        bounds.removeFromTop(5);
        
        // Preview panel en la parte inferior
        auto bottomArea = bounds.removeFromBottom(120);
        previewPanel_.setBounds(bottomArea.removeFromTop(80));
        bottomArea.removeFromTop(5);
        
        // Botones
        auto buttonArea = bottomArea;
        importButton_.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2).reduced(2));
        scanButton_.setBounds(buttonArea.reduced(2));
        
        // Pestañas ocupan el resto
        categoryTabs_.setBounds(bounds);
    }
    
    // FileDragAndDropTarget implementation
    bool isInterestedInFileDrag(const juce::StringArray& files) override {
        for (const auto& file : files) {
            if (file.endsWith(".wav") || file.endsWith(".mp3") || 
                file.endsWith(".aif") || file.endsWith(".flac"))
                return true;
        }
        return false;
    }
    
    void filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/) override {
        for (const auto& filePath : files) {
            juce::File file(filePath);
            if (onFileDropped)
                onFileDropped(file);
        }
    }
    
    // Callbacks
    std::function<void(const juce::File&)> onFileDropped;
    std::function<void(const juce::String&)> onSampleSelected;
    
private:
    class LibraryTreeItem : public juce::TreeViewItem {
    public:
        LibraryTreeItem(const juce::String& name, const juce::String& path = {})
            : itemName_(name), itemPath_(path) {}
        
        bool mightContainSubItems() override { return !itemPath_.isNotEmpty(); }
        
        void paintItem(juce::Graphics& g, int width, int height) override {
            if (isSelected())
                g.fillAll(FLColors::Orange.withAlpha(0.3f));
            
            g.setColour(juce::Colours::white);
            g.setFont(12.0f);
            
            juce::String icon = itemPath_.isEmpty() ? "📁 " : "🎵 ";
            g.drawText(icon + itemName_, 4, 0, width - 4, height, juce::Justification::centredLeft);
        }
        
        void itemClicked(const juce::MouseEvent&) override {
            if (itemPath_.isNotEmpty() && owner_) {
                if (owner_->onSampleSelected)
                    owner_->onSampleSelected(itemPath_);
            }
        }
        
        void setOwner(LibraryBrowserPanel* owner) { owner_ = owner; }
        
    private:
        juce::String itemName_;
        juce::String itemPath_;
        LibraryBrowserPanel* owner_ = nullptr;
    };
    
    std::unique_ptr<juce::TreeViewItem> createBeatsTree() {
        auto root = std::make_unique<LibraryTreeItem>("Beats");
        
        auto trapBeats = std::make_unique<LibraryTreeItem>("Trap");
        trapBeats->addSubItem(new LibraryTreeItem("Hard Trap 140", "presets/beats/trap_hard_140.wav"));
        trapBeats->addSubItem(new LibraryTreeItem("Dark Trap 130", "presets/beats/trap_dark_130.wav"));
        root->addSubItem(trapBeats.release());
        
        auto lofiBeats = std::make_unique<LibraryTreeItem>("Lo-Fi");
        lofiBeats->addSubItem(new LibraryTreeItem("Chill Beat 85", "presets/beats/lofi_chill_85.wav"));
        lofiBeats->addSubItem(new LibraryTreeItem("Jazzy Beat 90", "presets/beats/lofi_jazzy_90.wav"));
        root->addSubItem(lofiBeats.release());
        
        auto drillBeats = std::make_unique<LibraryTreeItem>("Drill");
        drillBeats->addSubItem(new LibraryTreeItem("UK Drill 140", "presets/beats/drill_uk_140.wav"));
        drillBeats->addSubItem(new LibraryTreeItem("NY Drill 145", "presets/beats/drill_ny_145.wav"));
        root->addSubItem(drillBeats.release());
        
        return root;
    }
    
    std::unique_ptr<juce::TreeViewItem> createSamplesTree() {
        auto root = std::make_unique<LibraryTreeItem>("Samples");
        
        auto drums = std::make_unique<LibraryTreeItem>("Drums");
        drums->addSubItem(new LibraryTreeItem("Kick 1", "samples/kick_01.wav"));
        drums->addSubItem(new LibraryTreeItem("Snare 1", "samples/snare_01.wav"));
        drums->addSubItem(new LibraryTreeItem("Hi-Hat 1", "samples/hihat_01.wav"));
        root->addSubItem(drums.release());
        
        auto vocals = std::make_unique<LibraryTreeItem>("Vocals");
        vocals->addSubItem(new LibraryTreeItem("Vocal Chop 1", "samples/vocal_chop_01.wav"));
        vocals->addSubItem(new LibraryTreeItem("Vocal Pad 1", "samples/vocal_pad_01.wav"));
        root->addSubItem(vocals.release());
        
        return root;
    }
    
    std::unique_ptr<juce::TreeViewItem> createLoopsTree() {
        auto root = std::make_unique<LibraryTreeItem>("Loops");
        
        auto melodicLoops = std::make_unique<LibraryTreeItem>("Melodic");
        melodicLoops->addSubItem(new LibraryTreeItem("Piano Loop C", "loops/piano_c_major.wav"));
        melodicLoops->addSubItem(new LibraryTreeItem("Guitar Loop Am", "loops/guitar_a_minor.wav"));
        root->addSubItem(melodicLoops.release());
        
        return root;
    }
    
    std::unique_ptr<juce::TreeViewItem> createPresetsTree() {
        auto root = std::make_unique<LibraryTreeItem>("Presets");
        
        auto synthPresets = std::make_unique<LibraryTreeItem>("Synths");
        synthPresets->addSubItem(new LibraryTreeItem("Dark Lead", "presets/synth_dark_lead.preset"));
        synthPresets->addSubItem(new LibraryTreeItem("Bright Pad", "presets/synth_bright_pad.preset"));
        root->addSubItem(synthPresets.release());
        
        return root;
    }
    
    std::unique_ptr<juce::TreeViewItem> createPluginsTree() {
        auto root = std::make_unique<LibraryTreeItem>("Plugins");
        
        auto effects = std::make_unique<LibraryTreeItem>("Effects");
        effects->addSubItem(new LibraryTreeItem("Reverb", ""));
        effects->addSubItem(new LibraryTreeItem("Delay", ""));
        effects->addSubItem(new LibraryTreeItem("Distortion", ""));
        root->addSubItem(effects.release());
        
        auto instruments = std::make_unique<LibraryTreeItem>("Instruments");
        instruments->addSubItem(new LibraryTreeItem("Sampler", ""));
        instruments->addSubItem(new LibraryTreeItem("Synth", ""));
        root->addSubItem(instruments.release());
        
        return root;
    }
    
    void filterContent() {
        // TODO: Implementar filtrado
        juce::ignoreUnused(searchBox_);
    }
    
    void importFiles() {
        fileChooser_ = std::make_unique<juce::FileChooser>("Select audio files to import");
        auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        
        fileChooser_->launchAsync(flags, [this](const juce::FileChooser& chooser) {
            auto files = chooser.getResults();
            for (const auto& file : files) {
                if (onFileDropped)
                    onFileDropped(file);
            }
        });
    }
    
    void scanFolders() {
        // TODO: Implementar escaneo de carpetas
    }
    
    juce::TextEditor searchBox_;
    juce::TabbedComponent categoryTabs_{juce::TabbedButtonBar::TabsAtTop};
    
    std::unique_ptr<juce::TreeView> beatsTree_;
    std::unique_ptr<juce::TreeView> samplesTree_;
    std::unique_ptr<juce::TreeView> loopsTree_;
    std::unique_ptr<juce::TreeView> presetsTree_;
    std::unique_ptr<juce::TreeView> pluginsTree_;
    
    juce::Label previewPanel_;
    juce::TextButton importButton_;
    juce::TextButton scanButton_;
    
    std::unique_ptr<juce::FileChooser> fileChooser_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryBrowserPanel)
};

} // namespace GUI
} // namespace OmegaStudio
