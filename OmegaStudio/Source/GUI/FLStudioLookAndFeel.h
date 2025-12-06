//==============================================================================
// FLStudioLookAndFeel.h
// Professional FL Studio-inspired Look & Feel
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace Omega {
namespace GUI {

//==============================================================================
// Color Palette FL Studio-style
//==============================================================================
struct FLColors {
    // Primary Colors
    static const juce::Colour Orange;
    static const juce::Colour OrangeHover;
    static const juce::Colour OrangeActive;
    
    // Background Colors
    static const juce::Colour DarkBg;
    static const juce::Colour MediumBg;
    static const juce::Colour LightBg;
    static const juce::Colour PanelBg;
    
    // Text Colors
    static const juce::Colour TextPrimary;
    static const juce::Colour TextSecondary;
    static const juce::Colour TextDisabled;
    
    // Accent Colors
    static const juce::Colour Success;
    static const juce::Colour Warning;
    static const juce::Colour Danger;
    static const juce::Colour Info;
    
    // Additional Colors for new components
    static const juce::Colour Green;
    static const juce::Colour Blue;
    static const juce::Colour Cyan;
    static const juce::Colour Purple;
    
    // UI Elements
    static const juce::Colour Border;
    static const juce::Colour BorderLight;
    static const juce::Colour Shadow;
    static const juce::Colour Glow;
    
    // Channel Rack Colors (for different pattern types)
    static juce::Colour getChannelColor(int channelIndex);
    static juce::Colour getPatternColor(int patternIndex);
};

//==============================================================================
// FL Studio-style Look & Feel
//==============================================================================
class FLStudioLookAndFeel : public juce::LookAndFeel_V4 {
public:
    FLStudioLookAndFeel();
    ~FLStudioLookAndFeel() override = default;
    
    //==========================================================================
    // Button Drawing
    //==========================================================================
    void drawButtonBackground(juce::Graphics& g,
                             juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool isMouseOverButton,
                             bool isButtonDown) override;
    
    void drawButtonText(juce::Graphics& g,
                       juce::TextButton& button,
                       bool isMouseOverButton,
                       bool isButtonDown) override;
    
    void drawToggleButton(juce::Graphics& g,
                         juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;
    
    //==========================================================================
    // Slider Drawing (Knobs & Faders)
    //==========================================================================
    void drawRotarySlider(juce::Graphics& g,
                         int x, int y, int width, int height,
                         float sliderPosProportional,
                         float rotaryStartAngle,
                         float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    void drawLinearSlider(juce::Graphics& g,
                         int x, int y, int width, int height,
                         float sliderPos,
                         float minSliderPos,
                         float maxSliderPos,
                         const juce::Slider::SliderStyle style,
                         juce::Slider& slider) override;
    
    //==========================================================================
    // ComboBox Drawing
    //==========================================================================
    void drawComboBox(juce::Graphics& g,
                     int width, int height,
                     bool isButtonDown,
                     int buttonX, int buttonY,
                     int buttonW, int buttonH,
                     juce::ComboBox& box) override;
    
    void drawPopupMenuItem(juce::Graphics& g,
                          const juce::Rectangle<int>& area,
                          bool isSeparator,
                          bool isActive,
                          bool isHighlighted,
                          bool isTicked,
                          bool hasSubMenu,
                          const juce::String& text,
                          const juce::String& shortcutKeyText,
                          const juce::Drawable* icon,
                          const juce::Colour* textColourToUse) override;
    
    //==========================================================================
    // Label Drawing
    //==========================================================================
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
    //==========================================================================
    // Progress Bar
    //==========================================================================
    void drawProgressBar(juce::Graphics& g,
                        juce::ProgressBar& progressBar,
                        int width, int height,
                        double progress,
                        const juce::String& textToShow) override;
    
    //==========================================================================
    // Scrollbar
    //==========================================================================
    void drawScrollbar(juce::Graphics& g,
                      juce::ScrollBar& scrollbar,
                      int x, int y, int width, int height,
                      bool isScrollbarVertical,
                      int thumbStartPosition,
                      int thumbSize,
                      bool isMouseOver,
                      bool isMouseDown) override;
    
    //==========================================================================
    // TabBar
    //==========================================================================
    void drawTabButton(juce::TabBarButton& button,
                      juce::Graphics& g,
                      bool isMouseOver,
                      bool isMouseDown) override;
    
    //==========================================================================
    // Tooltip
    //==========================================================================
    void drawTooltip(juce::Graphics& g,
                    const juce::String& text,
                    int width, int height) override;
    
    //==========================================================================
    // Window Title Bar
    //==========================================================================
    void drawDocumentWindowTitleBar(juce::DocumentWindow& window,
                                   juce::Graphics& g,
                                   int w, int h,
                                   int titleSpaceX, int titleSpaceW,
                                   const juce::Image* icon,
                                   bool drawTitleTextOnLeft) override;
    
    //==========================================================================
    // Resize Bar
    //==========================================================================
    void drawResizableFrame(juce::Graphics& g,
                           int w, int h,
                           const juce::BorderSize<int>& border) override;
    
    //==========================================================================
    // Custom Methods
    //==========================================================================
    void drawGlowEffect(juce::Graphics& g,
                       juce::Rectangle<float> bounds,
                       juce::Colour colour,
                       float intensity = 0.5f);
    
    void drawNeuomorphicButton(juce::Graphics& g,
                              juce::Rectangle<float> bounds,
                              bool isPressed,
                              bool isHighlighted);
    
    juce::Font getCustomFont(float height);
    
private:
    juce::Font customFont;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FLStudioLookAndFeel)
};

} // namespace GUI
} // namespace Omega
