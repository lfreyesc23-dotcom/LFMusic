//==============================================================================
// ThemeSystem.h
// FL Studio 2025 Complete Theme System
// Light/Dark/Custom themes, accent colors, font scaling, icon packs
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <map>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Color Scheme - Esquema de colores completo
//==============================================================================
struct ColorScheme {
    juce::String name;
    
    // Background colors
    juce::Colour backgroundDark = juce::Colour(0xff1a1a1a);
    juce::Colour backgroundMedium = juce::Colour(0xff2b2b2b);
    juce::Colour backgroundLight = juce::Colour(0xff3a3a3a);
    
    // Accent colors
    juce::Colour accentPrimary = juce::Colour(0xffff8736);
    juce::Colour accentSecondary = juce::Colour(0xff4a90ff);
    
    // Text colors
    juce::Colour textPrimary = juce::Colour(0xffdddddd);
    juce::Colour textSecondary = juce::Colour(0xff9a9a9a);
    juce::Colour textDisabled = juce::Colour(0xff6a6a6a);
    
    // UI element colors
    juce::Colour buttonNormal = juce::Colour(0xff3a3a3a);
    juce::Colour buttonHover = juce::Colour(0xff4a4a4a);
    juce::Colour buttonPressed = juce::Colour(0xff2a2a2a);
    juce::Colour buttonActive = juce::Colour(0xffff8736);
    
    // Level meter colors
    juce::Colour meterGreen = juce::Colour(0xff36ff8c);
    juce::Colour meterYellow = juce::Colour(0xfffff036);
    juce::Colour meterRed = juce::Colour(0xffff3636);
    
    // Track colors (palette)
    std::vector<juce::Colour> trackColors = {
        juce::Colour(0xffff3636), // Red
        juce::Colour(0xffff8736), // Orange
        juce::Colour(0xfffff036), // Yellow
        juce::Colour(0xff36ff8c), // Green
        juce::Colour(0xff36c9ff), // Cyan
        juce::Colour(0xff4a90ff), // Blue
        juce::Colour(0xff9d36ff), // Purple
        juce::Colour(0xffff36c9), // Pink
    };
    
    juce::var toVar() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", name);
        obj->setProperty("backgroundDark", backgroundDark.toString());
        obj->setProperty("backgroundMedium", backgroundMedium.toString());
        obj->setProperty("backgroundLight", backgroundLight.toString());
        obj->setProperty("accentPrimary", accentPrimary.toString());
        obj->setProperty("accentSecondary", accentSecondary.toString());
        obj->setProperty("textPrimary", textPrimary.toString());
        obj->setProperty("textSecondary", textSecondary.toString());
        return juce::var(obj);
    }
    
    static ColorScheme fromVar(const juce::var& v) {
        ColorScheme scheme;
        if (auto* obj = v.getDynamicObject()) {
            scheme.name = obj->getProperty("name").toString();
            scheme.backgroundDark = juce::Colour::fromString(obj->getProperty("backgroundDark").toString());
            scheme.backgroundMedium = juce::Colour::fromString(obj->getProperty("backgroundMedium").toString());
            scheme.backgroundLight = juce::Colour::fromString(obj->getProperty("backgroundLight").toString());
            scheme.accentPrimary = juce::Colour::fromString(obj->getProperty("accentPrimary").toString());
            scheme.accentSecondary = juce::Colour::fromString(obj->getProperty("accentSecondary").toString());
            scheme.textPrimary = juce::Colour::fromString(obj->getProperty("textPrimary").toString());
            scheme.textSecondary = juce::Colour::fromString(obj->getProperty("textSecondary").toString());
        }
        return scheme;
    }
    
    static ColorScheme getDefaultDark() {
        ColorScheme scheme;
        scheme.name = "FL Studio Dark";
        return scheme; // Uses default values
    }
    
    static ColorScheme getLight() {
        ColorScheme scheme;
        scheme.name = "FL Studio Light";
        scheme.backgroundDark = juce::Colour(0xfff0f0f0);
        scheme.backgroundMedium = juce::Colour(0xfffafafa);
        scheme.backgroundLight = juce::Colour(0xffffffff);
        scheme.textPrimary = juce::Colour(0xff1a1a1a);
        scheme.textSecondary = juce::Colour(0xff6a6a6a);
        scheme.textDisabled = juce::Colour(0xff9a9a9a);
        scheme.buttonNormal = juce::Colour(0xffe0e0e0);
        scheme.buttonHover = juce::Colour(0xffd0d0d0);
        scheme.buttonPressed = juce::Colour(0xffc0c0c0);
        return scheme;
    }
    
    static ColorScheme getBlue() {
        ColorScheme scheme;
        scheme.name = "Blue Night";
        scheme.backgroundDark = juce::Colour(0xff0a1628);
        scheme.backgroundMedium = juce::Colour(0xff1a2a3a);
        scheme.backgroundLight = juce::Colour(0xff2a3a4a);
        scheme.accentPrimary = juce::Colour(0xff4a90ff);
        scheme.accentSecondary = juce::Colour(0xff36c9ff);
        return scheme;
    }
    
    static ColorScheme getPurple() {
        ColorScheme scheme;
        scheme.name = "Purple Haze";
        scheme.backgroundDark = juce::Colour(0xff1a0a28);
        scheme.backgroundMedium = juce::Colour(0xff2a1a3a);
        scheme.backgroundLight = juce::Colour(0xff3a2a4a);
        scheme.accentPrimary = juce::Colour(0xff9d36ff);
        scheme.accentSecondary = juce::Colour(0xffff36c9);
        return scheme;
    }
};

//==============================================================================
// Font Settings - Configuración de fuentes
//==============================================================================
struct FontSettings {
    float scaleFactor = 1.0f; // 1.0 = 100%, 1.5 = 150%, 2.0 = 200%
    juce::String fontFamily = "Arial";
    bool useBoldForHeaders = true;
    
    float getScaledSize(float baseSize) const {
        return baseSize * scaleFactor;
    }
    
    juce::Font getFont(float baseSize, bool bold = false) const {
        return juce::Font(fontFamily, getScaledSize(baseSize), 
                         bold ? juce::Font::bold : juce::Font::plain);
    }
};

//==============================================================================
// Icon Pack - Conjunto de iconos
//==============================================================================
struct IconPack {
    juce::String name;
    std::map<juce::String, juce::String> icons;
    
    static IconPack getDefault() {
        IconPack pack;
        pack.name = "Default Emoji";
        pack.icons = {
            {"play", "▶️"},
            {"stop", "⏹️"},
            {"record", "⏺️"},
            {"loop", "🔁"},
            {"metronome", "🎵"},
            {"mixer", "🎚️"},
            {"pianoRoll", "🎹"},
            {"browser", "📁"},
            {"playlist", "📝"},
            {"effects", "🎛️"},
            {"instruments", "🎸"},
            {"save", "💾"},
            {"load", "📂"},
            {"export", "📤"},
            {"settings", "⚙️"},
            {"help", "❓"},
            {"ai", "🤖"},
            {"automation", "⚡"},
            {"sidechain", "🔗"},
            {"volume", "🔊"},
            {"mute", "🔇"},
            {"solo", "🎤"},
        };
        return pack;
    }
    
    static IconPack getMinimal() {
        IconPack pack;
        pack.name = "Minimal Unicode";
        pack.icons = {
            {"play", "▶"},
            {"stop", "■"},
            {"record", "●"},
            {"loop", "⟲"},
            {"metronome", "♩"},
            {"mixer", "≡"},
            {"pianoRoll", "♫"},
            {"browser", "☰"},
            {"playlist", "≣"},
            {"effects", "⚡"},
            {"instruments", "♪"},
            {"save", "💾"},
            {"load", "⇓"},
            {"export", "⇑"},
            {"settings", "⚙"},
            {"help", "?"},
            {"ai", "⚡"},
            {"automation", "~"},
            {"sidechain", "⇄"},
            {"volume", "♪"},
            {"mute", "✕"},
            {"solo", "◉"},
        };
        return pack;
    }
    
    juce::String getIcon(const juce::String& key) const {
        auto it = icons.find(key);
        return it != icons.end() ? it->second : "?";
    }
};

//==============================================================================
// Theme - Tema completo
//==============================================================================
struct Theme {
    juce::String name;
    ColorScheme colors;
    FontSettings fonts;
    IconPack icons;
    
    juce::var toVar() const {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("name", name);
        obj->setProperty("colors", colors.toVar());
        obj->setProperty("fontScale", fonts.scaleFactor);
        obj->setProperty("fontFamily", fonts.fontFamily);
        obj->setProperty("iconPack", icons.name);
        return juce::var(obj);
    }
    
    static Theme getDefault() {
        Theme theme;
        theme.name = "FL Studio 2025 Dark";
        theme.colors = ColorScheme::getDefaultDark();
        theme.icons = IconPack::getDefault();
        return theme;
    }
    
    static Theme getLight() {
        Theme theme;
        theme.name = "FL Studio 2025 Light";
        theme.colors = ColorScheme::getLight();
        theme.icons = IconPack::getDefault();
        return theme;
    }
};

//==============================================================================
// Theme Manager - Gestor de temas
//==============================================================================
class ThemeManager : public juce::ChangeBroadcaster {
public:
    static ThemeManager& getInstance() {
        static ThemeManager instance;
        return instance;
    }
    
    void setTheme(const Theme& newTheme) {
        currentTheme = newTheme;
        saveTheme();
        sendChangeMessage();
    }
    
    const Theme& getCurrentTheme() const {
        return currentTheme;
    }
    
    const ColorScheme& getColors() const {
        return currentTheme.colors;
    }
    
    const FontSettings& getFonts() const {
        return currentTheme.fonts;
    }
    
    const IconPack& getIcons() const {
        return currentTheme.icons;
    }
    
    void setAccentColor(juce::Colour colour) {
        currentTheme.colors.accentPrimary = colour;
        sendChangeMessage();
    }
    
    void setFontScale(float scale) {
        currentTheme.fonts.scaleFactor = juce::jlimit(0.75f, 2.0f, scale);
        sendChangeMessage();
    }
    
    std::vector<Theme> getAvailableThemes() const {
        return {
            Theme::getDefault(),
            Theme::getLight(),
            // Add more predefined themes
        };
    }
    
    void loadTheme() {
        juce::File themeFile = getThemeFile();
        
        if (themeFile.existsAsFile()) {
            auto json = juce::JSON::parse(themeFile);
            if (auto* obj = json.getDynamicObject()) {
                currentTheme.name = obj->getProperty("name").toString();
                currentTheme.colors = ColorScheme::fromVar(obj->getProperty("colors"));
                currentTheme.fonts.scaleFactor = obj->getProperty("fontScale");
                currentTheme.fonts.fontFamily = obj->getProperty("fontFamily").toString();
                
                // Load icon pack
                juce::String iconPackName = obj->getProperty("iconPack").toString();
                if (iconPackName == "Minimal Unicode") {
                    currentTheme.icons = IconPack::getMinimal();
                } else {
                    currentTheme.icons = IconPack::getDefault();
                }
            }
        }
    }
    
    void saveTheme() {
        juce::File themeFile = getThemeFile();
        themeFile.replaceWithText(juce::JSON::toString(currentTheme.toVar(), true));
    }
    
private:
    Theme currentTheme = Theme::getDefault();
    
    ThemeManager() {
        loadTheme();
    }
    
    juce::File getThemeFile() const {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("OmegaStudio")
            .getChildFile("theme.json");
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeManager)
};

//==============================================================================
// Theme Settings Panel - Panel de configuración de tema
//==============================================================================
class ThemeSettingsPanel : public juce::Component {
public:
    ThemeSettingsPanel() {
        // Theme selector
        themeSelector.addItem("Dark (Default)", 1);
        themeSelector.addItem("Light", 2);
        themeSelector.addItem("Blue Night", 3);
        themeSelector.addItem("Purple Haze", 4);
        themeSelector.setSelectedId(1);
        themeSelector.onChange = [this] { applySelectedTheme(); };
        addAndMakeVisible(themeSelector);
        
        // Accent color button
        accentColorButton.setButtonText("Choose Accent Color");
        accentColorButton.onClick = [this] { chooseAccentColor(); };
        addAndMakeVisible(accentColorButton);
        
        // Font scale slider
        fontScaleSlider.setRange(0.75, 2.0, 0.25);
        fontScaleSlider.setValue(1.0);
        fontScaleSlider.setTextValueSuffix(" Font Size");
        fontScaleSlider.onValueChange = [this] {
            ThemeManager::getInstance().setFontScale((float)fontScaleSlider.getValue());
        };
        addAndMakeVisible(fontScaleSlider);
        
        // Icon pack selector
        iconPackSelector.addItem("🎨 Emoji Icons", 1);
        iconPackSelector.addItem("📐 Minimal Icons", 2);
        iconPackSelector.setSelectedId(1);
        addAndMakeVisible(iconPackSelector);
        
        // Preview area
        preview Label.setText("PREVIEW", juce::dontSendNotification);
        previewLabel.setJustificationType(juce::Justification::centred);
        previewLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff1a1a1a));
        addAndMakeVisible(previewLabel);
    }
    
    void paint(juce::Graphics& g) override {
        auto& colors = ThemeManager::getInstance().getColors();
        
        g.fillAll(colors.backgroundMedium);
        
        g.setColour(colors.textPrimary);
        g.setFont(ThemeManager::getInstance().getFonts().getFont(16.0f, true));
        g.drawText("THEME SETTINGS", getLocalBounds().removeFromTop(40), 
                  juce::Justification::centred);
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(16);
        bounds.removeFromTop(44);
        
        themeSelector.setBounds(bounds.removeFromTop(32));
        bounds.removeFromTop(8);
        
        accentColorButton.setBounds(bounds.removeFromTop(32));
        bounds.removeFromTop(8);
        
        fontScaleSlider.setBounds(bounds.removeFromTop(60));
        bounds.removeFromTop(8);
        
        iconPackSelector.setBounds(bounds.removeFromTop(32));
        bounds.removeFromTop(16);
        
        previewLabel.setBounds(bounds.removeFromTop(100));
    }
    
private:
    juce::ComboBox themeSelector;
    juce::TextButton accentColorButton;
    juce::Slider fontScaleSlider;
    juce::ComboBox iconPackSelector;
    juce::Label previewLabel;
    
    void applySelectedTheme() {
        Theme theme;
        
        switch (themeSelector.getSelectedId()) {
            case 1: theme = Theme::getDefault(); break;
            case 2: theme = Theme::getLight(); break;
            case 3: theme.colors = ColorScheme::getBlue(); break;
            case 4: theme.colors = ColorScheme::getPurple(); break;
        }
        
        ThemeManager::getInstance().setTheme(theme);
        repaint();
    }
    
    void chooseAccentColor() {
        auto colourSelector = std::make_unique<juce::ColourSelector>();
        colourSelector->setCurrentColour(ThemeManager::getInstance().getColors().accentPrimary);
        colourSelector->addChangeListener(this);
        
        // Show in dialog
        juce::DialogWindow::showDialog("Choose Accent Color", colourSelector.release(), 
                                      this, juce::Colours::darkgrey, true);
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeSettingsPanel)
};

} // namespace GUI
} // namespace OmegaStudio
