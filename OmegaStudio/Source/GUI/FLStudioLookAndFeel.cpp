//==============================================================================
// FLStudioLookAndFeel.cpp
// Professional FL Studio-inspired Look & Feel Implementation
//==============================================================================

#include "FLStudioLookAndFeel.h"

namespace Omega {
namespace GUI {

//==============================================================================
// FLColors Implementation - FL STUDIO PROFESSIONAL
//==============================================================================
const juce::Colour FLColors::Orange            = juce::Colour(0xFFFF6B00); // FL Studio Orange
const juce::Colour FLColors::OrangeHover       = juce::Colour(0xFFFF8533);
const juce::Colour FLColors::OrangeActive      = juce::Colour(0xFFFF5200);

const juce::Colour FLColors::DarkBg            = juce::Colour(0xFF404040); // Fondo playlist
const juce::Colour FLColors::MediumBg          = juce::Colour(0xFF4A4A4A); // Panels
const juce::Colour FLColors::LightBg           = juce::Colour(0xFF555555); // Hover
const juce::Colour FLColors::PanelBg           = juce::Colour(0xFF2C2C2C); // Toolbar/Sidebar

const juce::Colour FLColors::TextPrimary       = juce::Colour(0xFFEBEBEB); // Texto blanco
const juce::Colour FLColors::TextSecondary     = juce::Colour(0xFFAAAAAA); // Texto gris
const juce::Colour FLColors::TextDisabled      = juce::Colour(0xFF666666);

const juce::Colour FLColors::Success           = juce::Colour(0xFF7FA563); // Verde patterns
const juce::Colour FLColors::Warning           = juce::Colour(0xFFFFC107);
const juce::Colour FLColors::Danger            = juce::Colour(0xFFE74C3C);
const juce::Colour FLColors::Info              = juce::Colour(0xFF4A7BA7); // Azul patterns

const juce::Colour FLColors::Green             = juce::Colour(0xFF7FA563);
const juce::Colour FLColors::Blue              = juce::Colour(0xFF4A7BA7);
const juce::Colour FLColors::Cyan              = juce::Colour(0xFF5DADE2);
const juce::Colour FLColors::Purple            = juce::Colour(0xFF8E44AD);

const juce::Colour FLColors::Border            = juce::Colour(0xFF1E1E1E); // Bordes EXACTO imagen
const juce::Colour FLColors::BorderLight       = juce::Colour(0xFF606060);
const juce::Colour FLColors::Shadow            = juce::Colour(0x80000000);
const juce::Colour FLColors::Glow              = juce::Colour(0x60FF6B00);

juce::Colour FLColors::getChannelColor(int channelIndex) {
    static const juce::Colour colors[] = {
        juce::Colour(0xFFFF6B6B), // Red
        juce::Colour(0xFF4ECDC4), // Cyan
        juce::Colour(0xFFFFE66D), // Yellow
        juce::Colour(0xFF95E1D3), // Mint
        juce::Colour(0xFFC7CEEA), // Purple
        juce::Colour(0xFFFFA07A), // Salmon
        juce::Colour(0xFF98D8C8), // Teal
        juce::Colour(0xFFF7DC6F)  // Gold
    };
    return colors[channelIndex % 8];
}

juce::Colour FLColors::getPatternColor(int patternIndex) {
    static const juce::Colour colors[] = {
        juce::Colour(0xFFE74C3C), // Red
        juce::Colour(0xFF3498DB), // Blue
        juce::Colour(0xFF2ECC71), // Green
        juce::Colour(0xFFF39C12), // Orange
        juce::Colour(0xFF9B59B6), // Purple
        juce::Colour(0xFF1ABC9C), // Turquoise
        juce::Colour(0xFFE67E22), // Carrot
        juce::Colour(0xFF34495E)  // Wet Asphalt
    };
    return colors[patternIndex % 8];
}

//==============================================================================
// FLStudioLookAndFeel Implementation
//==============================================================================
FLStudioLookAndFeel::FLStudioLookAndFeel() {
    // Set default colors
    setColour(juce::ResizableWindow::backgroundColourId, FLColors::DarkBg);
    setColour(juce::TextButton::buttonColourId, FLColors::MediumBg);
    setColour(juce::TextButton::textColourOffId, FLColors::TextPrimary);
    setColour(juce::TextButton::buttonOnColourId, FLColors::Orange);
    setColour(juce::ComboBox::backgroundColourId, FLColors::MediumBg);
    setColour(juce::ComboBox::textColourId, FLColors::TextPrimary);
    setColour(juce::ComboBox::outlineColourId, FLColors::Border);
    setColour(juce::Label::textColourId, FLColors::TextPrimary);
    setColour(juce::Slider::thumbColourId, FLColors::Orange);
    setColour(juce::Slider::trackColourId, FLColors::LightBg);
    setColour(juce::Slider::backgroundColourId, FLColors::MediumBg);
    
    // Load custom font (if available)
    customFont = juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain);
}

//==============================================================================
// Button Drawing
//==============================================================================
void FLStudioLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                               juce::Button& button,
                                               const juce::Colour& backgroundColour,
                                               bool isMouseOverButton,
                                               bool isButtonDown) {
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    auto cornerSize = 4.0f;
    
    // Determine button color
    juce::Colour baseColour = backgroundColour;
    if (button.getToggleState()) {
        baseColour = FLColors::Orange;
    }
    
    if (isButtonDown) {
        baseColour = baseColour.darker(0.3f);
    } else if (isMouseOverButton) {
        baseColour = baseColour.brighter(0.2f);
    }
    
    // Draw shadow
    g.setColour(FLColors::Shadow);
    g.fillRoundedRectangle(bounds.translated(0, 2), cornerSize);
    
    // Draw main button with gradient
    juce::ColourGradient gradient(
        baseColour.brighter(0.1f), bounds.getX(), bounds.getY(),
        baseColour.darker(0.1f), bounds.getX(), bounds.getBottom(),
        false
    );
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerSize);
    
    // Draw border
    g.setColour(isMouseOverButton ? FLColors::BorderLight : FLColors::Border);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    
    // Draw glow effect on hover
    if (isMouseOverButton && button.getToggleState()) {
        drawGlowEffect(g, bounds, FLColors::Orange, 0.6f);
    }
}

void FLStudioLookAndFeel::drawButtonText(juce::Graphics& g,
                                        juce::TextButton& button,
                                        bool isMouseOverButton,
                                        bool isButtonDown) {
    auto font = getCustomFont(14.0f);
    g.setFont(font);
    
    juce::Colour textColour = FLColors::TextPrimary;
    if (button.getToggleState()) {
        textColour = juce::Colours::white;
    } else if (!button.isEnabled()) {
        textColour = FLColors::TextDisabled;
    }
    
    g.setColour(textColour);
    
    auto bounds = button.getLocalBounds().reduced(4);
    g.drawText(button.getButtonText(), bounds,
              juce::Justification::centred, true);
}

void FLStudioLookAndFeel::drawToggleButton(juce::Graphics& g,
                                          juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat();
    auto tickSize = juce::jmin(20.0f, bounds.getHeight() * 0.75f);
    
    // Draw switch-style toggle
    auto switchBounds = juce::Rectangle<float>(tickSize * 2.0f, tickSize)
                            .withCentre(juce::Point<float>(tickSize + 2, bounds.getCentreY()));
    
    // Track
    g.setColour(button.getToggleState() ? FLColors::Orange : FLColors::MediumBg);
    g.fillRoundedRectangle(switchBounds, tickSize * 0.5f);
    
    // Thumb
    auto thumbBounds = juce::Rectangle<float>(tickSize * 0.8f, tickSize * 0.8f);
    if (button.getToggleState()) {
        thumbBounds.setCentre(switchBounds.getRight() - tickSize * 0.5f, switchBounds.getCentreY());
    } else {
        thumbBounds.setCentre(switchBounds.getX() + tickSize * 0.5f, switchBounds.getCentreY());
    }
    
    g.setColour(juce::Colours::white);
    g.fillEllipse(thumbBounds);
    
    // Draw label
    if (button.getButtonText().isNotEmpty()) {
        auto textBounds = bounds.withLeft(switchBounds.getRight() + 8);
        g.setColour(FLColors::TextPrimary);
        g.setFont(getCustomFont(14.0f));
        g.drawText(button.getButtonText(), textBounds.toNearestInt(),
                  juce::Justification::centredLeft, true);
    }
}

//==============================================================================
// Slider Drawing (Knobs & Faders)
//==============================================================================
void FLStudioLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& slider) {
    auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(10);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    
    // Draw outer ring (track)
    g.setColour(FLColors::MediumBg);
    g.fillEllipse(bounds);
    
    // Draw inner circle
    auto innerBounds = bounds.reduced(4);
    g.setColour(FLColors::DarkBg);
    g.fillEllipse(innerBounds);
    
    // Draw value arc
    juce::Path arcPath;
    arcPath.addCentredArc(centreX, centreY, radius - 2, radius - 2,
                          0.0f, rotaryStartAngle, angle, true);
    
    g.setColour(FLColors::Orange);
    g.strokePath(arcPath, juce::PathStrokeType(3.0f));
    
    // Draw pointer
    juce::Path pointer;
    auto pointerLength = radius * 0.6f;
    auto pointerThickness = 2.0f;
    pointer.addRectangle(-pointerThickness * 0.5f, -radius + 6, pointerThickness, pointerLength);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    
    g.setColour(FLColors::Orange);
    g.fillPath(pointer);
    
    // Draw center dot
    g.setColour(FLColors::TextPrimary);
    g.fillEllipse(juce::Rectangle<float>(6, 6).withCentre({centreX, centreY}));
    
    // Draw glow effect
    if (slider.isMouseOverOrDragging()) {
        drawGlowEffect(g, bounds, FLColors::Orange, 0.4f);
    }
}

void FLStudioLookAndFeel::drawLinearSlider(juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPos,
                                          float minSliderPos,
                                          float maxSliderPos,
                                          const juce::Slider::SliderStyle style,
                                          juce::Slider& slider) {
    if (style == juce::Slider::LinearVertical || style == juce::Slider::LinearHorizontal) {
        auto isVertical = (style == juce::Slider::LinearVertical);
        auto trackWidth = isVertical ? width * 0.3f : height * 0.3f;
        
        // Draw track background
        juce::Rectangle<float> track;
        if (isVertical) {
            track = juce::Rectangle<float>(x + width * 0.5f - trackWidth * 0.5f,
                                          y, trackWidth, height);
        } else {
            track = juce::Rectangle<float>(x, y + height * 0.5f - trackWidth * 0.5f,
                                          width, trackWidth);
        }
        
        g.setColour(FLColors::MediumBg);
        g.fillRoundedRectangle(track, trackWidth * 0.5f);
        
        // Draw filled track
        juce::Rectangle<float> filledTrack;
        if (isVertical) {
            filledTrack = track.withTop(sliderPos);
        } else {
            filledTrack = track.withRight(sliderPos);
        }
        
        g.setColour(FLColors::Orange);
        g.fillRoundedRectangle(filledTrack, trackWidth * 0.5f);
        
        // Draw thumb
        auto thumbSize = isVertical ? width * 0.8f : height * 0.8f;
        juce::Rectangle<float> thumb;
        if (isVertical) {
            thumb = juce::Rectangle<float>(thumbSize, thumbSize * 0.5f)
                        .withCentre({track.getCentreX(), sliderPos});
        } else {
            thumb = juce::Rectangle<float>(thumbSize * 0.5f, thumbSize)
                        .withCentre({sliderPos, track.getCentreY()});
        }
        
        // Draw shadow
        g.setColour(FLColors::Shadow);
        g.fillRoundedRectangle(thumb.translated(0, 2), 2.0f);
        
        // Draw thumb
        g.setColour(slider.isMouseOverOrDragging() ? FLColors::OrangeHover : FLColors::Orange);
        g.fillRoundedRectangle(thumb, 2.0f);
        
        // Draw border
        g.setColour(FLColors::BorderLight);
        g.drawRoundedRectangle(thumb, 2.0f, 1.0f);
    }
}

//==============================================================================
// ComboBox Drawing
//==============================================================================
void FLStudioLookAndFeel::drawComboBox(juce::Graphics& g,
                                      int width, int height,
                                      bool isButtonDown,
                                      int buttonX, int buttonY,
                                      int buttonW, int buttonH,
                                      juce::ComboBox& box) {
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1);
    auto cornerSize = 4.0f;
    
    // Background
    g.setColour(isButtonDown ? FLColors::LightBg : FLColors::MediumBg);
    g.fillRoundedRectangle(bounds, cornerSize);
    
    // Border
    g.setColour(box.hasKeyboardFocus(true) ? FLColors::Orange : FLColors::Border);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    
    // Arrow button
    auto arrowZone = juce::Rectangle<int>(buttonX, buttonY, buttonW, buttonH).toFloat();
    
    juce::Path path;
    path.startNewSubPath(arrowZone.getX() + 3.0f, arrowZone.getCentreY() - 2.0f);
    path.lineTo(arrowZone.getCentreX(), arrowZone.getCentreY() + 2.0f);
    path.lineTo(arrowZone.getRight() - 3.0f, arrowZone.getCentreY() - 2.0f);
    
    g.setColour(FLColors::TextPrimary);
    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void FLStudioLookAndFeel::drawPopupMenuItem(juce::Graphics& g,
                                           const juce::Rectangle<int>& area,
                                           bool isSeparator,
                                           bool isActive,
                                           bool isHighlighted,
                                           bool isTicked,
                                           bool hasSubMenu,
                                           const juce::String& text,
                                           const juce::String& shortcutKeyText,
                                           const juce::Drawable* icon,
                                           const juce::Colour* textColourToUse) {
    if (isSeparator) {
        auto r = area.reduced(5, 0);
        r.removeFromTop(r.getHeight() / 2);
        
        g.setColour(FLColors::Border);
        g.fillRect(r.removeFromTop(1));
    } else {
        auto textColour = FLColors::TextPrimary;
        
        if (isHighlighted && isActive) {
            g.setColour(FLColors::Orange.withAlpha(0.2f));
            g.fillRect(area);
            textColour = FLColors::TextPrimary.brighter(0.2f);
        }
        
        if (!isActive)
            textColour = FLColors::TextDisabled;
        
        auto r = area.reduced(1);
        
        if (isTicked) {
            g.setColour(FLColors::Orange);
            auto tick = juce::Rectangle<float>(r.getX() + 4, r.getCentreY() - 6, 12, 12);
            g.fillEllipse(tick);
        }
        
        if (hasSubMenu) {
            auto arrowH = 0.6f * area.getHeight();
            auto x = area.getRight() - arrowH - 6;
            auto y = area.getCentreY() - arrowH * 0.5f;
            
            juce::Path p;
            p.addTriangle(x, y, x, y + arrowH, x + arrowH * 0.6f, y + arrowH * 0.5f);
            
            g.setColour(textColour);
            g.fillPath(p);
        }
        
        r.removeFromLeft(20);
        
        g.setColour(textColour);
        g.setFont(getCustomFont(14.0f));
        g.drawText(text, r, juce::Justification::centredLeft, true);
        
        if (shortcutKeyText.isNotEmpty()) {
            g.setFont(getCustomFont(12.0f));
            g.setColour(FLColors::TextSecondary);
            g.drawText(shortcutKeyText, r, juce::Justification::centredRight, true);
        }
    }
}

//==============================================================================
// Other UI Elements
//==============================================================================
void FLStudioLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label) {
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited()) {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
        g.setFont(getCustomFont(label.getFont().getHeight()));
        
        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
        g.drawText(label.getText(), textArea,
                  label.getJustificationType(),
                  true);
    }
}

void FLStudioLookAndFeel::drawProgressBar(juce::Graphics& g,
                                         juce::ProgressBar& progressBar,
                                         int width, int height,
                                         double progress,
                                         const juce::String& textToShow) {
    auto background = FLColors::MediumBg;
    g.setColour(background);
    g.fillRoundedRectangle(0, 0, (float)width, (float)height, 4.0f);
    
    if (progress >= 0.0f && progress <= 1.0f) {
        g.setColour(FLColors::Orange);
        g.fillRoundedRectangle(0, 0, (float)(width * progress), (float)height, 4.0f);
    }
    
    if (textToShow.isNotEmpty()) {
        g.setColour(FLColors::TextPrimary);
        g.setFont(getCustomFont(12.0f));
        g.drawText(textToShow, 0, 0, width, height,
                  juce::Justification::centred, false);
    }
}

void FLStudioLookAndFeel::drawScrollbar(juce::Graphics& g,
                                       juce::ScrollBar& scrollbar,
                                       int x, int y, int width, int height,
                                       bool isScrollbarVertical,
                                       int thumbStartPosition,
                                       int thumbSize,
                                       bool isMouseOver,
                                       bool isMouseDown) {
    g.setColour(FLColors::DarkBg);
    g.fillAll();
    
    juce::Rectangle<int> thumbBounds;
    if (isScrollbarVertical) {
        thumbBounds = juce::Rectangle<int>(x, thumbStartPosition, width, thumbSize);
    } else {
        thumbBounds = juce::Rectangle<int>(thumbStartPosition, y, thumbSize, height);
    }
    
    auto colour = FLColors::MediumBg;
    if (isMouseDown)
        colour = FLColors::Orange;
    else if (isMouseOver)
        colour = FLColors::LightBg;
    
    g.setColour(colour);
    g.fillRoundedRectangle(thumbBounds.reduced(2).toFloat(), 3.0f);
}

void FLStudioLookAndFeel::drawTabButton(juce::TabBarButton& button,
                      juce::Graphics& g,
                      bool isMouseOver,
                      bool isMouseDown) {
    auto bounds = button.getLocalBounds().toFloat();
    
    g.setColour(button.getToggleState() ? FLColors::Orange : FLColors::MediumBg);
    g.fillRect(bounds);
    
    if (isMouseOver && !button.getToggleState()) {
        g.setColour(FLColors::LightBg);
        g.fillRect(bounds);
    }
    
    g.setColour(button.getToggleState() ? juce::Colours::white : FLColors::TextPrimary);
    g.setFont(getCustomFont(14.0f));
    g.drawText(button.getButtonText(), bounds.toNearestInt(),
              juce::Justification::centred, true);
}

void FLStudioLookAndFeel::drawTooltip(juce::Graphics& g,
                    const juce::String& text,
                    int width, int height) {
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    
    g.setColour(FLColors::PanelBg);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    g.setColour(FLColors::Orange);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    g.setColour(FLColors::TextPrimary);
    g.setFont(getCustomFont(12.0f));
    g.drawText(text, bounds.reduced(4).toNearestInt(),
              juce::Justification::centred, true);
}

void FLStudioLookAndFeel::drawDocumentWindowTitleBar(juce::DocumentWindow& window,
                                   juce::Graphics& g,
                                   int w, int h,
                                   int titleSpaceX, int titleSpaceW,
                                   const juce::Image* icon,
                                   bool drawTitleTextOnLeft) {
    g.setColour(FLColors::PanelBg);
    g.fillAll();
    
    g.setColour(FLColors::Orange);
    g.setFont(getCustomFont(16.0f));
    
    auto textW = juce::jmin(titleSpaceW, g.getCurrentFont().getStringWidth(window.getName()) + 20);
    auto textX = drawTitleTextOnLeft ? titleSpaceX : juce::jmax(titleSpaceX, (w - textW) / 2);
    
    g.drawText(window.getName(), textX, 0, textW, h,
              juce::Justification::centredLeft, true);
}

void FLStudioLookAndFeel::drawResizableFrame(juce::Graphics& g,
                           int w, int h,
                           const juce::BorderSize<int>& border) {
    g.setColour(FLColors::Border);
    g.drawRect(0, 0, w, h, 1);
}

//==============================================================================
// Custom Methods
//==============================================================================
void FLStudioLookAndFeel::drawGlowEffect(juce::Graphics& g,
                                        juce::Rectangle<float> bounds,
                                        juce::Colour colour,
                                        float intensity) {
    auto glowRadius = 20.0f * intensity;
    
    for (int i = 0; i < 3; ++i) {
        auto alpha = (1.0f - i / 3.0f) * intensity;
        g.setColour(colour.withAlpha(alpha * 0.3f));
        g.drawRoundedRectangle(bounds.expanded(glowRadius * (i + 1)),
                              4.0f, 2.0f);
    }
}

void FLStudioLookAndFeel::drawNeuomorphicButton(juce::Graphics& g,
                                               juce::Rectangle<float> bounds,
                                               bool isPressed,
                                               bool isHighlighted) {
    auto cornerSize = 8.0f;
    
    if (!isPressed) {
        // Light shadow (top-left)
        g.setColour(FLColors::LightBg);
        g.fillRoundedRectangle(bounds.translated(-2, -2), cornerSize);
        
        // Dark shadow (bottom-right)
        g.setColour(FLColors::DarkBg);
        g.fillRoundedRectangle(bounds.translated(2, 2), cornerSize);
    }
    
    // Main surface
    g.setColour(FLColors::MediumBg);
    g.fillRoundedRectangle(bounds, cornerSize);
    
    if (isHighlighted) {
        g.setColour(FLColors::Orange.withAlpha(0.1f));
        g.fillRoundedRectangle(bounds, cornerSize);
    }
}

juce::Font FLStudioLookAndFeel::getCustomFont(float height) {
    return juce::Font(juce::Font::getDefaultSansSerifFontName(), height, juce::Font::plain);
}

} // namespace GUI
} // namespace Omega
