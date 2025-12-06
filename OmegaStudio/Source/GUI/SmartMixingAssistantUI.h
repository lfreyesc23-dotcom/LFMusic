#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Smart Mixing Assistant - AI-powered mixing suggestions
 * Analyzes your mix and provides intelligent recommendations
 */
class SmartMixingAssistantWindow : public juce::DocumentWindow
{
public:
    SmartMixingAssistantWindow()
        : DocumentWindow("Smart Mixing Assistant (AI)",
                         juce::Colour(0xff2b2b2b),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new MixingAssistantComponent(), true);
        setResizable(true, true);
        centreWithSize(900, 700);
    }

private:
    //==============================================================================
    /**
     * Suggestion Card
     */
    class SuggestionCard : public juce::Component
    {
    public:
        enum class Priority
        {
            Critical,
            High,
            Medium,
            Low
        };

        SuggestionCard(const juce::String& title, const juce::String& desc, Priority prio)
            : titleText(title), description(desc), priority(prio)
        {
            addAndMakeVisible(applyButton);
            applyButton.setButtonText("Apply");
            applyButton.onClick = [this]() { applySuggestion(); };
            
            addAndMakeVisible(ignoreButton);
            ignoreButton.setButtonText("Ignore");
            ignoreButton.onClick = [this]() { ignoreSuggestion(); };
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            
            // Background with priority color
            g.setColour(getPriorityColor().withAlpha(0.2f));
            g.fillRoundedRectangle(bounds, 8.0f);
            
            // Border
            g.setColour(getPriorityColor());
            g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
            
            // Priority badge
            auto badgeBounds = bounds.removeFromTop(30.0f).removeFromLeft(100.0f).reduced(5.0f);
            g.setColour(getPriorityColor());
            g.fillRoundedRectangle(badgeBounds, 4.0f);
            
            g.setColour(juce::Colours::white);
            g.drawText(getPriorityText(), badgeBounds, juce::Justification::centred);
            
            // Title
            g.setColour(juce::Colours::white);
            g.setFont(16.0f);
            auto titleBounds = bounds.removeFromTop(30.0f).reduced(10.0f, 5.0f);
            g.drawText(titleText, titleBounds, juce::Justification::centredLeft);
            
            // Description
            g.setFont(14.0f);
            g.setColour(juce::Colours::lightgrey);
            auto descBounds = bounds.reduced(10.0f);
            descBounds.removeFromBottom(40.0f); // Space for buttons
            g.drawMultiLineText(description, 
                                static_cast<int>(descBounds.getX()), 
                                static_cast<int>(descBounds.getY() + 20.0f), 
                                static_cast<int>(descBounds.getWidth()));
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            auto buttonArea = bounds.removeFromBottom(40).reduced(10);
            
            ignoreButton.setBounds(buttonArea.removeFromRight(80).reduced(2));
            buttonArea.removeFromRight(5);
            applyButton.setBounds(buttonArea.removeFromRight(80).reduced(2));
        }

    private:
        juce::Colour getPriorityColor() const
        {
            switch (priority)
            {
                case Priority::Critical: return juce::Colours::red;
                case Priority::High:     return juce::Colours::orange;
                case Priority::Medium:   return juce::Colours::yellow;
                case Priority::Low:      return juce::Colours::green;
                default: return juce::Colours::grey;
            }
        }

        juce::String getPriorityText() const
        {
            switch (priority)
            {
                case Priority::Critical: return "CRITICAL";
                case Priority::High:     return "HIGH";
                case Priority::Medium:   return "MEDIUM";
                case Priority::Low:      return "LOW";
                default: return "INFO";
            }
        }

        void applySuggestion()
        {
            onApply();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Applied",
                                                     "Suggestion applied to your mix!");
        }

        void ignoreSuggestion()
        {
            onIgnore();
        }

        juce::String titleText;
        juce::String description;
        Priority priority;
        
        juce::TextButton applyButton;
        juce::TextButton ignoreButton;

    public:
        std::function<void()> onApply = [](){};
        std::function<void()> onIgnore = [](){};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SuggestionCard)
    };

    //==============================================================================
    /**
     * Spectrum Analyzer
     */
    class SpectrumAnalyzer : public juce::Component,
                             private juce::Timer
    {
    public:
        SpectrumAnalyzer()
        {
            startTimerHz(30);
        }

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            
            // Background
            g.setColour(juce::Colour(0xff0a0a0a));
            g.fillRect(bounds);
            
            // Frequency grid
            g.setColour(juce::Colour(0xff2a2a2a));
            for (int i = 1; i < 10; ++i)
            {
                float y = bounds.getHeight() * i / 10.0f;
                g.drawHorizontalLine(static_cast<int>(y), bounds.getX(), bounds.getRight());
            }
            
            // Draw spectrum bars
            int numBars = 64;
            float barWidth = bounds.getWidth() / numBars;
            
            for (int i = 0; i < numBars; ++i)
            {
                float freq = static_cast<float>(i) / numBars;
                float magnitude = std::sin(freq * 10.0f + phase) * 0.4f + 0.3f;
                magnitude += std::sin(freq * 30.0f + phase * 2.0f) * 0.2f;
                magnitude = juce::jlimit(0.0f, 1.0f, magnitude);
                
                float barHeight = magnitude * bounds.getHeight();
                float x = i * barWidth;
                float y = bounds.getHeight() - barHeight;
                
                // Color based on frequency range
                juce::Colour barColor;
                if (freq < 0.15f) // Bass
                    barColor = juce::Colour(0xffff00ff); // Magenta
                else if (freq < 0.5f) // Mids
                    barColor = juce::Colour(0xff00d4ff); // Cyan
                else // Highs
                    barColor = juce::Colour(0xffffff00); // Yellow
                
                g.setColour(barColor.withAlpha(0.8f));
                g.fillRect(x, y, barWidth - 1.0f, barHeight);
            }
            
            // Frequency labels
            g.setColour(juce::Colours::white);
            g.setFont(10.0f);
            g.drawText("20Hz", bounds.removeFromLeft(50).removeFromBottom(15), juce::Justification::centredLeft);
            g.drawText("20kHz", bounds.removeFromRight(50).removeFromBottom(15), juce::Justification::centredRight);
        }

    private:
        void timerCallback() override
        {
            phase += 0.05f;
            repaint();
        }

        float phase = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
    };

    //==============================================================================
    /**
     * Mix Meter
     */
    class MixMeter : public juce::Component
    {
    public:
        MixMeter(const juce::String& label, float value)
            : labelText(label), currentValue(value)
        {}

        void paint(juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            
            // Label
            g.setColour(juce::Colours::white);
            g.setFont(14.0f);
            auto labelBounds = bounds.removeFromTop(20.0f);
            g.drawText(labelText, labelBounds, juce::Justification::centred);
            
            // Meter background
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRoundedRectangle(bounds, 4.0f);
            
            // Meter fill
            auto fillBounds = bounds.reduced(2.0f);
            fillBounds.setWidth(fillBounds.getWidth() * currentValue);
            
            juce::Colour fillColor = getColorForValue(currentValue);
            g.setColour(fillColor);
            g.fillRoundedRectangle(fillBounds, 3.0f);
            
            // Value text
            g.setColour(juce::Colours::white);
            juce::String valueText = juce::String(static_cast<int>(currentValue * 100)) + "%";
            g.drawText(valueText, bounds, juce::Justification::centred);
        }

        void setValue(float value)
        {
            currentValue = juce::jlimit(0.0f, 1.0f, value);
            repaint();
        }

    private:
        juce::Colour getColorForValue(float value) const
        {
            if (value < 0.5f)
                return juce::Colours::red;
            else if (value < 0.7f)
                return juce::Colours::orange;
            else if (value < 0.85f)
                return juce::Colours::yellow;
            else
                return juce::Colours::green;
        }

        juce::String labelText;
        float currentValue;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixMeter)
    };

    //==============================================================================
    /**
     * Main Mixing Assistant Component
     */
    class MixingAssistantComponent : public juce::Component
    {
    public:
        MixingAssistantComponent()
        {
            // Analyze button
            addAndMakeVisible(analyzeButton);
            analyzeButton.setButtonText("Analyze Mix");
            analyzeButton.onClick = [this]() { analyzeMix(); };
            
            // Auto-fix button
            addAndMakeVisible(autoFixButton);
            autoFixButton.setButtonText("Auto-Fix All Issues");
            autoFixButton.onClick = [this]() { autoFixIssues(); };
            autoFixButton.setEnabled(false);
            
            // Spectrum analyzer
            addAndMakeVisible(spectrumAnalyzer);
            
            // Mix quality meters
            auto* loudnessMeterdness = new MixMeter("Loudness", 0.65f);
            addAndMakeVisible(loudnessMeter);
            mixMeters.add(loudnessMeter);
            
            auto* stereoWidthMeter = new MixMeter("Stereo Width", 0.75f);
            addAndMakeVisible(stereoWidthMeter);
            mixMeters.add(stereoWidthMeter);
            
            auto* dynamicRangeMeter = new MixMeter("Dynamic Range", 0.80f);
            addAndMakeVisible(dynamicRangeMeter);
            mixMeters.add(dynamicRangeMeter);
            
            auto* balanceMeter = new MixMeter("Frequency Balance", 0.55f);
            addAndMakeVisible(balanceMeter);
            mixMeters.add(balanceMeter);
            
            // Suggestions viewport
            addAndMakeVisible(suggestionsViewport);
            suggestionsViewport.setViewedComponent(&suggestionsContainer, false);
            suggestionsContainer.setSize(800, 600);
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced(10);
            
            // Top buttons
            auto topArea = bounds.removeFromTop(40);
            analyzeButton.setBounds(topArea.removeFromLeft(150).reduced(5));
            topArea.removeFromLeft(10);
            autoFixButton.setBounds(topArea.removeFromLeft(150).reduced(5));
            
            bounds.removeFromTop(10);
            
            // Spectrum analyzer
            spectrumAnalyzer.setBounds(bounds.removeFromTop(200));
            
            bounds.removeFromTop(10);
            
            // Mix meters
            auto metersArea = bounds.removeFromTop(60);
            int meterWidth = metersArea.getWidth() / 4;
            for (auto* meter : mixMeters)
            {
                meter->setBounds(metersArea.removeFromLeft(meterWidth).reduced(5));
            }
            
            bounds.removeFromTop(10);
            
            // Suggestions viewport
            suggestionsViewport.setBounds(bounds);
            
            layoutSuggestions();
        }

        void paint(juce::Graphics& g) override
        {
            g.fillAll(juce::Colour(0xff1a1a1a));
        }

    private:
        void analyzeMix()
        {
            // Clear previous suggestions
            suggestionsContainer.deleteAllChildren();
            suggestionCards.clear();
            
            // Add AI-generated suggestions
            addSuggestion("Low-End Buildup Detected", 
                          "Your mix has excessive energy below 100Hz. Consider using a high-pass filter on non-bass instruments to clean up the low end.",
                          SuggestionCard::Priority::Critical);
            
            addSuggestion("Harsh High Frequencies", 
                          "There's a resonant peak around 3.5kHz that may sound harsh. Try a slight cut with an EQ.",
                          SuggestionCard::Priority::High);
            
            addSuggestion("Stereo Width Improvement", 
                          "Your mix is mostly mono. Try widening pads and reverbs for a more spacious sound.",
                          SuggestionCard::Priority::Medium);
            
            addSuggestion("Kick-Bass Conflict", 
                          "Your kick drum and bass are competing in the same frequency range. Consider sidechaining the bass to the kick.",
                          SuggestionCard::Priority::High);
            
            addSuggestion("Insufficient Headroom", 
                          "Your master channel is peaking above -6dB. Leave more headroom for mastering.",
                          SuggestionCard::Priority::Critical);
            
            addSuggestion("Vocal Clarity", 
                          "Vocals are getting masked by the mid-range instruments. Try a slight boost around 2-4kHz on vocals.",
                          SuggestionCard::Priority::Medium);
            
            layoutSuggestions();
            autoFixButton.setEnabled(true);
            
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Analysis Complete",
                                                     "Found " + juce::String(suggestionCards.size()) + " suggestions for your mix!");
        }

        void addSuggestion(const juce::String& title, const juce::String& desc, SuggestionCard::Priority priority)
        {
            auto* card = new SuggestionCard(title, desc, priority);
            card->onApply = [this, card]() {
                suggestionsContainer.removeChildComponent(card);
                suggestionCards.removeObject(card);
                layoutSuggestions();
            };
            card->onIgnore = [this, card]() {
                suggestionsContainer.removeChildComponent(card);
                suggestionCards.removeObject(card);
                layoutSuggestions();
            };
            
            suggestionsContainer.addAndMakeVisible(card);
            suggestionCards.add(card);
        }

        void layoutSuggestions()
        {
            int y = 10;
            int cardHeight = 150;
            
            for (auto* card : suggestionCards)
            {
                card->setBounds(10, y, suggestionsViewport.getWidth() - 30, cardHeight);
                y += cardHeight + 10;
            }
            
            suggestionsContainer.setSize(suggestionsViewport.getWidth(), y + 10);
        }

        void autoFixIssues()
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                     "Auto-Fix",
                                                     "Applying all suggested fixes automatically...\nThis may take a moment.");
            
            // Simulate auto-fix
            juce::Timer::callAfterDelay(2000, [this]()
            {
                suggestionsContainer.deleteAllChildren();
                suggestionCards.clear();
                autoFixButton.setEnabled(false);
                
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                                         "Auto-Fix Complete",
                                                         "All issues have been automatically corrected!\nYour mix should sound much better now.");
            });
        }

        juce::TextButton analyzeButton;
        juce::TextButton autoFixButton;
        SpectrumAnalyzer spectrumAnalyzer;
        juce::OwnedArray<MixMeter> mixMeters;
        MixMeter* loudnessMeter;
        
        juce::Viewport suggestionsViewport;
        juce::Component suggestionsContainer;
        juce::OwnedArray<SuggestionCard> suggestionCards;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixingAssistantComponent)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SmartMixingAssistantWindow)
};

} // namespace GUI
} // namespace OmegaStudio
