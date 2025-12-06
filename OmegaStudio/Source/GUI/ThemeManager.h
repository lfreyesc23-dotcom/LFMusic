#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Theme Manager - FL Studio style themes with customization
 */
class ThemeManager
{
public:
    enum class Theme
    {
        FLStudioOrange,
        Dark,
        Light,
        Blue,
        Purple,
        Custom
    };

    struct ColorScheme
    {
        juce::Colour background;
        juce::Colour backgroundDark;
        juce::Colour backgroundLight;
        juce::Colour accent;
        juce::Colour accentHover;
        juce::Colour text;
        juce::Colour textDark;
        juce::Colour border;
        juce::Colour warning;
        juce::Colour error;
        juce::Colour success;
        juce::Colour highlight;
        
        // Component specific
        juce::Colour buttonBackground;
        juce::Colour buttonHover;
        juce::Colour sliderBackground;
        juce::Colour sliderThumb;
        juce::Colour meterGreen;
        juce::Colour meterYellow;
        juce::Colour meterRed;
    };

    static ThemeManager& getInstance()
    {
        static ThemeManager instance;
        return instance;
    }

    void setTheme(Theme theme)
    {
        currentTheme = theme;
        
        switch (theme)
        {
            case Theme::FLStudioOrange:
                currentColors = getFLStudioOrangeColors();
                break;
            case Theme::Dark:
                currentColors = getDarkColors();
                break;
            case Theme::Light:
                currentColors = getLightColors();
                break;
            case Theme::Blue:
                currentColors = getBlueColors();
                break;
            case Theme::Purple:
                currentColors = getPurpleColors();
                break;
            case Theme::Custom:
                // Keep current custom colors
                break;
        }
        
        notifyListeners();
    }

    const ColorScheme& getColors() const { return currentColors; }
    Theme getCurrentTheme() const { return currentTheme; }

    void setCustomColors(const ColorScheme& colors)
    {
        currentColors = colors;
        currentTheme = Theme::Custom;
        notifyListeners();
    }

    // Font sizes
    float getTitleFontSize() const { return titleFontSize * fontScale; }
    float getHeaderFontSize() const { return headerFontSize * fontScale; }
    float getBodyFontSize() const { return bodyFontSize * fontScale; }
    float getSmallFontSize() const { return smallFontSize * fontScale; }

    void setFontScale(float scale)
    {
        fontScale = juce::jlimit(0.8f, 2.0f, scale);
        notifyListeners();
    }

    float getFontScale() const { return fontScale; }

    // High DPI
    void setHighDPI(bool enabled)
    {
        highDPI = enabled;
        notifyListeners();
    }

    bool isHighDPI() const { return highDPI; }

    // Listener interface
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void themeChanged() = 0;
    };

    void addListener(Listener* listener)
    {
        listeners.add(listener);
    }

    void removeListener(Listener* listener)
    {
        listeners.remove(listener);
    }

private:
    ThemeManager()
        : currentTheme(Theme::FLStudioOrange)
        , currentColors(getFLStudioOrangeColors())
    {}

    ~ThemeManager() = default;

    void notifyListeners()
    {
        listeners.call([](Listener& l) { l.themeChanged(); });
    }

    static ColorScheme getFLStudioOrangeColors()
    {
        return {
            juce::Colour(0xff2b2b2b),  // background
            juce::Colour(0xff1a1a1a),  // backgroundDark
            juce::Colour(0xff3a3a3a),  // backgroundLight
            juce::Colour(0xffff8800),  // accent (FL Studio orange)
            juce::Colour(0xffffaa00),  // accentHover
            juce::Colours::white,       // text
            juce::Colours::lightgrey,   // textDark
            juce::Colours::grey,        // border
            juce::Colours::yellow,      // warning
            juce::Colours::red,         // error
            juce::Colours::green,       // success
            juce::Colour(0xffff8800).withAlpha(0.3f), // highlight
            juce::Colour(0xff444444),  // buttonBackground
            juce::Colour(0xff555555),  // buttonHover
            juce::Colour(0xff333333),  // sliderBackground
            juce::Colour(0xffff8800),  // sliderThumb
            juce::Colours::green,       // meterGreen
            juce::Colours::yellow,      // meterYellow
            juce::Colours::red          // meterRed
        };
    }

    static ColorScheme getDarkColors()
    {
        return {
            juce::Colour(0xff1e1e1e),  // background
            juce::Colour(0xff0d0d0d),  // backgroundDark
            juce::Colour(0xff2d2d2d),  // backgroundLight
            juce::Colour(0xff00d4ff),  // accent (cyan)
            juce::Colour(0xff00e0ff),  // accentHover
            juce::Colours::white,       // text
            juce::Colours::lightgrey,   // textDark
            juce::Colours::darkgrey,    // border
            juce::Colours::yellow,      // warning
            juce::Colours::red,         // error
            juce::Colours::green,       // success
            juce::Colour(0xff00d4ff).withAlpha(0.3f), // highlight
            juce::Colour(0xff3a3a3a),  // buttonBackground
            juce::Colour(0xff4a4a4a),  // buttonHover
            juce::Colour(0xff2a2a2a),  // sliderBackground
            juce::Colour(0xff00d4ff),  // sliderThumb
            juce::Colours::green,       // meterGreen
            juce::Colours::yellow,      // meterYellow
            juce::Colours::red          // meterRed
        };
    }

    static ColorScheme getLightColors()
    {
        return {
            juce::Colour(0xfff5f5f5),  // background
            juce::Colour(0xffe0e0e0),  // backgroundDark
            juce::Colours::white,       // backgroundLight
            juce::Colour(0xff0078d7),  // accent (blue)
            juce::Colour(0xff0090ff),  // accentHover
            juce::Colours::black,       // text
            juce::Colours::darkgrey,    // textDark
            juce::Colours::grey,        // border
            juce::Colour(0xfff9a825),  // warning
            juce::Colour(0xffc62828),  // error
            juce::Colour(0xff2e7d32),  // success
            juce::Colour(0xff0078d7).withAlpha(0.2f), // highlight
            juce::Colour(0xffe0e0e0),  // buttonBackground
            juce::Colour(0xffd0d0d0),  // buttonHover
            juce::Colour(0xfff0f0f0),  // sliderBackground
            juce::Colour(0xff0078d7),  // sliderThumb
            juce::Colour(0xff2e7d32),  // meterGreen
            juce::Colour(0xfff9a825),  // meterYellow
            juce::Colour(0xffc62828)   // meterRed
        };
    }

    static ColorScheme getBlueColors()
    {
        return {
            juce::Colour(0xff1a2332),  // background
            juce::Colour(0xff0f1419),  // backgroundDark
            juce::Colour(0xff293241),  // backgroundLight
            juce::Colour(0xff3a86ff),  // accent
            juce::Colour(0xff5aa0ff),  // accentHover
            juce::Colours::white,       // text
            juce::Colours::lightgrey,   // textDark
            juce::Colour(0xff3a86ff).darker(), // border
            juce::Colours::yellow,      // warning
            juce::Colours::red,         // error
            juce::Colours::green,       // success
            juce::Colour(0xff3a86ff).withAlpha(0.3f), // highlight
            juce::Colour(0xff293241),  // buttonBackground
            juce::Colour(0xff3a86ff).darker(), // buttonHover
            juce::Colour(0xff1a2332),  // sliderBackground
            juce::Colour(0xff3a86ff),  // sliderThumb
            juce::Colours::green,       // meterGreen
            juce::Colours::yellow,      // meterYellow
            juce::Colours::red          // meterRed
        };
    }

    static ColorScheme getPurpleColors()
    {
        return {
            juce::Colour(0xff1a1a2e),  // background
            juce::Colour(0xff0f0f1e),  // backgroundDark
            juce::Colour(0xff252541),  // backgroundLight
            juce::Colour(0xff9d4edd),  // accent (purple)
            juce::Colour(0xffb565ff),  // accentHover
            juce::Colours::white,       // text
            juce::Colours::lightgrey,   // textDark
            juce::Colour(0xff9d4edd).darker(), // border
            juce::Colours::yellow,      // warning
            juce::Colours::red,         // error
            juce::Colours::green,       // success
            juce::Colour(0xff9d4edd).withAlpha(0.3f), // highlight
            juce::Colour(0xff252541),  // buttonBackground
            juce::Colour(0xff9d4edd).darker(), // buttonHover
            juce::Colour(0xff1a1a2e),  // sliderBackground
            juce::Colour(0xff9d4edd),  // sliderThumb
            juce::Colours::green,       // meterGreen
            juce::Colours::yellow,      // meterYellow
            juce::Colours::red          // meterRed
        };
    }

    Theme currentTheme;
    ColorScheme currentColors;
    
    float titleFontSize = 24.0f;
    float headerFontSize = 18.0f;
    float bodyFontSize = 14.0f;
    float smallFontSize = 12.0f;
    float fontScale = 1.0f;
    
    bool highDPI = false;
    
    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeManager)
};

//==============================================================================
/**
 * Theme Selector Component
 */
class ThemeSelectorComponent : public juce::Component,
                                private ThemeManager::Listener
{
public:
    ThemeSelectorComponent()
    {
        addAndMakeVisible(themeCombo);
        themeCombo.addItem("FL Studio Orange", 1);
        themeCombo.addItem("Dark", 2);
        themeCombo.addItem("Light", 3);
        themeCombo.addItem("Blue", 4);
        themeCombo.addItem("Purple", 5);
        themeCombo.setSelectedId(1);
        themeCombo.onChange = [this]() {
            auto theme = static_cast<ThemeManager::Theme>(themeCombo.getSelectedId() - 1);
            ThemeManager::getInstance().setTheme(theme);
        };
        
        addAndMakeVisible(fontScaleLabel);
        fontScaleLabel.setText("Font Scale:", juce::dontSendNotification);
        
        addAndMakeVisible(fontScaleSlider);
        fontScaleSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        fontScaleSlider.setRange(0.8, 2.0, 0.1);
        fontScaleSlider.setValue(1.0);
        fontScaleSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
        fontScaleSlider.onValueChange = [this]() {
            ThemeManager::getInstance().setFontScale(static_cast<float>(fontScaleSlider.getValue()));
        };
        
        ThemeManager::getInstance().addListener(this);
    }

    ~ThemeSelectorComponent() override
    {
        ThemeManager::getInstance().removeListener(this);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        
        auto row1 = bounds.removeFromTop(30);
        themeCombo.setBounds(row1);
        
        bounds.removeFromTop(10);
        
        auto row2 = bounds.removeFromTop(30);
        fontScaleLabel.setBounds(row2.removeFromLeft(100));
        fontScaleSlider.setBounds(row2);
    }

    void paint(juce::Graphics& g) override
    {
        auto& colors = ThemeManager::getInstance().getColors();
        g.fillAll(colors.background);
    }

private:
    void themeChanged() override
    {
        repaint();
    }

    juce::ComboBox themeCombo;
    juce::Label fontScaleLabel;
    juce::Slider fontScaleSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeSelectorComponent)
};

} // namespace GUI
} // namespace OmegaStudio
