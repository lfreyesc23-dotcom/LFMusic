//==============================================================================
// ParametricEQComponent.h - Visual EQ Component with Frequency Response
// FL Studio Killer - Professional DAW
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "../Audio/DSP/ParametricEQ.h"
#include "../Audio/Analysis/SpectrumAnalyzer.h"
#include "../Workflow/AutomationClip.h"
#include "../Audio/Synthesis/ModulationMatrix.h"
#include "../Audio/VocalProductionSuite.h"

namespace OmegaStudio {

//==============================================================================
/** Visual EQ Component con visualización de curva de frecuencia */
class ParametricEQComponent : public juce::Component,
                              private juce::Timer
{
public:
    ParametricEQComponent(ParametricEQ& eq);
    ~ParametricEQComponent() override = default;
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
private:
    //==========================================================================
    ParametricEQ& eq_;
    
    // Band controls
    struct BandControl {
        int bandIndex;
        juce::Point<float> position; // Normalized 0-1
        bool dragging = false;
        
        juce::Rectangle<float> getBounds() const;
    };
    
    std::vector<BandControl> bandControls_;
    int selectedBand_ = 0;
    
    // Visualization
    void drawFrequencyResponse(juce::Graphics& g, juce::Rectangle<int> area);
    void drawGridLines(juce::Graphics& g, juce::Rectangle<int> area);
    void drawBandControls(juce::Graphics& g, juce::Rectangle<int> area);
    
    // Interaction
    int getBandAtPoint(juce::Point<int> point);
    void updateBandFromPosition(int bandIndex, juce::Point<float> position);
    
    // Convert between frequency/gain and pixels
    float frequencyToX(float freq, int width) const;
    float xToFrequency(float x, int width) const;
    float gainToY(float gain, int height) const;
    float yToGain(float y, int height) const;
    
    // Timer
    void timerCallback() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParametricEQComponent)
};

//==============================================================================
/** Spectrum Analyzer Component */
class SpectrumAnalyzerComponent : public juce::Component,
                                   private juce::Timer
{
public:
    SpectrumAnalyzerComponent();
    ~SpectrumAnalyzerComponent() override = default;
    
    // Setup
    void setAnalyzer(SpectrumAnalyzer* analyzer);
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Display options
    void setShowGrid(bool show) { showGrid_ = show; }
    void setShowPeaks(bool show) { showPeaks_ = show; }
    void setColorGradient(juce::Colour low, juce::Colour high);
    
private:
    //==========================================================================
    SpectrumAnalyzer* analyzer_ = nullptr;
    
    bool showGrid_ = true;
    bool showPeaks_ = true;
    
    juce::Colour lowColor_ = juce::Colours::blue;
    juce::Colour highColor_ = juce::Colours::red;
    
    std::vector<float> peakHold_;
    
    // Drawing
    void drawSpectrum(juce::Graphics& g, juce::Rectangle<int> area);
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void drawFrequencyLabels(juce::Graphics& g, juce::Rectangle<int> area);
    
    // Timer
    void timerCallback() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerComponent)
};

//==============================================================================
/** Automation Clip Editor */
class AutomationClipEditor : public juce::Component
{
public:
    AutomationClipEditor();
    ~AutomationClipEditor() override = default;
    
    // Setup
    void setClip(AutomationClip* clip);
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    
    // Tools
    enum class Tool { Pencil, Line, Curve, Erase };
    void setTool(Tool tool) { currentTool_ = tool; }
    
private:
    //==========================================================================
    AutomationClip* clip_ = nullptr;
    Tool currentTool_ = Tool::Pencil;
    
    int selectedPoint_ = -1;
    bool draggingPoint_ = false;
    
    // Drawing
    void drawGrid(juce::Graphics& g, juce::Rectangle<int> area);
    void drawAutomationCurve(juce::Graphics& g, juce::Rectangle<int> area);
    void drawPoints(juce::Graphics& g, juce::Rectangle<int> area);
    
    // Interaction
    int getPointAtPosition(juce::Point<int> pos);
    void addPointAtPosition(juce::Point<int> pos);
    void movePoint(int index, juce::Point<int> newPos);
    
    // Conversion
    juce::Point<float> pixelsToNormalized(juce::Point<int> pixels) const;
    juce::Point<int> normalizedToPixels(juce::Point<float> normalized) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationClipEditor)
};

//==============================================================================
/** Modulation Matrix Visual Editor */
class ModulationMatrixComponent : public juce::Component
{
public:
    ModulationMatrixComponent(ModulationMatrix& matrix);
    ~ModulationMatrixComponent() override = default;
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    
private:
    //==========================================================================
    ModulationMatrix& matrix_;
    
    // Layout
    int gridSize_ = 40;
    juce::Rectangle<int> matrixArea_;
    
    // Drawing
    void drawMatrix(juce::Graphics& g);
    void drawSourceLabels(juce::Graphics& g);
    void drawDestinationLabels(juce::Graphics& g);
    void drawConnections(juce::Graphics& g);
    
    // Interaction
    void handleCellClick(int sourceIndex, int destIndex);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrixComponent)
};

//==============================================================================
/** Professional Meter Component */
class MeterComponent : public juce::Component,
                       private juce::Timer
{
public:
    enum class Type { Peak, RMS, LUFS, TruePeak, Phase };
    
    MeterComponent(Type type);
    ~MeterComponent() override = default;
    
    // Setup
    void setLevel(float level);
    void setPeak(float peak);
    void setOrientation(bool horizontal);
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    //==========================================================================
    Type type_;
    bool horizontal_ = false;
    
    float currentLevel_ = 0.0f;
    float peakLevel_ = 0.0f;
    float displayLevel_ = 0.0f;
    
    // Colors
    juce::Colour getColorForLevel(float level) const;
    
    // Timer
    void timerCallback() override;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeterComponent)
};

//==============================================================================
/** Chord Progression Browser */
class ChordProgressionBrowser : public juce::Component,
                                public juce::TableListBoxModel
{
public:
    struct Progression {
        juce::String name;
        juce::String chords;
        juce::String genre;
    };
    
    ChordProgressionBrowser();
    ~ChordProgressionBrowser() override = default;
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // TableListBoxModel
    int getNumRows() override;
    void paintRowBackground(juce::Graphics& g, int rowNumber, int width, int height,
                           bool rowIsSelected) override;
    void paintCell(juce::Graphics& g, int rowNumber, int columnId,
                  int width, int height, bool rowIsSelected) override;
    void cellClicked(int rowNumber, int columnId, const juce::MouseEvent&) override;
    
    // Callback
    std::function<void(const Progression&)> onProgressionSelected;
    
private:
    //==========================================================================
    juce::TableListBox table_;
    juce::TextEditor searchBox_;
    juce::ComboBox genreFilter_;
    
    std::vector<Progression> progressions_;
    std::vector<Progression> filteredProgressions_;
    Progression selectedProgression_;
    
    void initializeProgressions();
    void filterProgressions();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChordProgressionBrowser)
};

//==============================================================================
/** Vocal Tuner Panel */
class VocalTunerPanel : public juce::Component,
                        private juce::Timer
{
public:
    VocalTunerPanel(VocalTuner& tuner);
    ~VocalTunerPanel() override = default;
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    //==========================================================================
    VocalTuner& tuner_;
    
    void timerCallback() override;
    void drawPitchDisplay(juce::Graphics& g, juce::Rectangle<int> area);
    
    void setupControls();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalTunerPanel)
};

//==============================================================================
/** Drum Step Sequencer UI */
class DrumSequencerComponent : public juce::Component
{
public:
    DrumSequencerComponent();
    ~DrumSequencerComponent() override = default;
    
    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    
    // Pattern
    void setPattern(const std::array<std::array<bool, 32>, 16>& pattern);
    std::array<std::array<bool, 32>, 16> getPattern() const;
    
    // Playback
    void setCurrentStep(int step) { currentStep_ = step; repaint(); }
    
private:
    //==========================================================================
    std::array<std::array<bool, 32>, 16> pattern_;
    int currentStep_ = 0;
    
    // Layout
    int stepWidth_ = 20;
    int trackHeight_ = 20;
    
    // Drawing
    void drawGrid(juce::Graphics& g);
    void drawSteps(juce::Graphics& g);
    void drawPlayhead(juce::Graphics& g);
    
    // Interaction
    void toggleStep(int track, int step);
    juce::Point<int> getStepAtPosition(juce::Point<int> pos);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSequencerComponent)
};

} // namespace OmegaStudio
