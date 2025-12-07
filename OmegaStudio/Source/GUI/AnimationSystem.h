//==============================================================================
// AnimationSystem.h
// FL Studio 2025 Professional Animation System
// Smooth transitions, bounce effects, glow, meters con peak hold
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <functional>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
// Easing Functions - Funciones de interpolación
//==============================================================================
class Easing {
public:
    static float linear(float t) {
        return t;
    }
    
    static float easeInQuad(float t) {
        return t * t;
    }
    
    static float easeOutQuad(float t) {
        return t * (2.0f - t);
    }
    
    static float easeInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
    
    static float easeInCubic(float t) {
        return t * t * t;
    }
    
    static float easeOutCubic(float t) {
        float f = t - 1.0f;
        return f * f * f + 1.0f;
    }
    
    static float easeInOutCubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
    }
    
    static float easeOutBounce(float t) {
        if (t < 1.0f / 2.75f) {
            return 7.5625f * t * t;
        } else if (t < 2.0f / 2.75f) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        } else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        } else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
    }
    
    static float easeOutElastic(float t) {
        if (t == 0.0f || t == 1.0f) return t;
        
        float p = 0.3f;
        float s = p / 4.0f;
        return std::pow(2.0f, -10.0f * t) * std::sin((t - s) * (2.0f * juce::MathConstants<float>::pi) / p) + 1.0f;
    }
};

//==============================================================================
// Animator - Motor de animación
//==============================================================================
class Animator : public juce::Timer {
public:
    using EasingFunction = std::function<float(float)>;
    
    Animator() = default;
    
    void animate(float from, float to, int durationMs, 
                EasingFunction easing = Easing::easeOutQuad) {
        startValue = from;
        endValue = to;
        currentValue = from;
        duration = durationMs;
        elapsed = 0;
        easingFunc = easing;
        isAnimating = true;
        
        startTimerHz(60); // 60 FPS
    }
    
    void stop() {
        isAnimating = false;
        stopTimer();
    }
    
    float getCurrentValue() const { return currentValue; }
    bool isRunning() const { return isAnimating; }
    
    std::function<void(float)> onUpdate;
    std::function<void()> onComplete;
    
private:
    float startValue = 0.0f;
    float endValue = 0.0f;
    float currentValue = 0.0f;
    int duration = 300;
    int elapsed = 0;
    bool isAnimating = false;
    EasingFunction easingFunc;
    
    void timerCallback() override {
        elapsed += getTimerInterval();
        
        if (elapsed >= duration) {
            currentValue = endValue;
            isAnimating = false;
            stopTimer();
            
            if (onUpdate) onUpdate(currentValue);
            if (onComplete) onComplete();
        } else {
            float t = (float)elapsed / (float)duration;
            float easedT = easingFunc ? easingFunc(t) : t;
            currentValue = startValue + (endValue - startValue) * easedT;
            
            if (onUpdate) onUpdate(currentValue);
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Animator)
};

//==============================================================================
// Animated Button - Botón con animaciones
//==============================================================================
class AnimatedButton : public juce::Button,
                       private juce::Timer {
public:
    AnimatedButton(const juce::String& name) : juce::Button(name) {
        startTimer(16); // ~60fps
    }
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                    bool shouldDrawButtonAsDown) override {
        auto bounds = getLocalBounds().toFloat().reduced(2);
        
        // Base color
        auto baseColour = juce::Colour(0xff3a3a3a);
        if (getToggleState()) {
            baseColour = juce::Colour(0xffff8736);
        }
        
        // Scale animation on hover
        if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown) {
            targetScale = 1.05f;
        } else {
            targetScale = 1.0f;
        }
        
        currentScale += (targetScale - currentScale) * 0.2f;
        
        auto scaledBounds = bounds.withSizeKeepingCentre(
            bounds.getWidth() * currentScale,
            bounds.getHeight() * currentScale
        );
        
        // Glow effect
        if (getToggleState() || shouldDrawButtonAsHighlighted) {
            glowIntensity += (1.0f - glowIntensity) * 0.15f;
            
            g.setColour(baseColour.withAlpha(glowIntensity * 0.3f));
            g.fillRoundedRectangle(scaledBounds.expanded(4), 6.0f);
        } else {
            glowIntensity *= 0.9f;
        }
        
        // Button background
        g.setColour(baseColour);
        g.fillRoundedRectangle(scaledBounds, 4.0f);
        
        // Shine effect
        if (shouldDrawButtonAsHighlighted) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            auto shineBounds = scaledBounds.removeFromTop(scaledBounds.getHeight() * 0.5f);
            g.fillRoundedRectangle(shineBounds, 4.0f);
        }
        
        // Border with pulse
        if (getToggleState()) {
            float pulse = 0.8f + 0.2f * std::sin(pulsePhase);
            g.setColour(juce::Colours::white.withAlpha(pulse * 0.5f));
            g.drawRoundedRectangle(scaledBounds, 4.0f, 2.0f);
        } else {
            g.setColour(baseColour.darker(0.4f));
            g.drawRoundedRectangle(scaledBounds, 4.0f, 1.0f);
        }
        
        // Text
        g.setColour(getToggleState() ? juce::Colours::white : juce::Colour(0xffdddddd));
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText(getButtonText(), scaledBounds, juce::Justification::centred);
    }
    
    void mouseDown(const juce::MouseEvent& event) override {
        juce::Button::mouseDown(event);
        
        // Bounce effect
        scaleAnimator.animate(1.0f, 0.9f, 100, Easing::easeOutCubic);
        scaleAnimator.onComplete = [this] {
            scaleAnimator.animate(0.9f, 1.0f, 150, Easing::easeOutBounce);
        };
    }
    
private:
    float currentScale = 1.0f;
    float targetScale = 1.0f;
    float glowIntensity = 0.0f;
    float pulsePhase = 0.0f;
    Animator scaleAnimator;
    
    void timerCallback() override {
        pulsePhase += 0.1f;
        if (pulsePhase > juce::MathConstants<float>::twoPi) {
            pulsePhase = 0.0f;
        }
        repaint();
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedButton)
};

//==============================================================================
// Animated Level Meter - Meter con peak hold y animación suave
//==============================================================================
class AnimatedLevelMeter : public juce::Component,
                          private juce::Timer {
public:
    AnimatedLevelMeter() {
        startTimer(16); // ~60fps
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds, 3.0f);
        
        // Level bar (animated smooth)
        float levelHeight = bounds.getHeight() * smoothLevel;
        auto levelBounds = bounds.removeFromBottom(levelHeight);
        
        // Gradient based on level
        juce::ColourGradient gradient;
        if (smoothLevel < 0.7f) {
            gradient = juce::ColourGradient(
                juce::Colour(0xff36ff8c), levelBounds.getX(), levelBounds.getBottom(),
                juce::Colour(0xff4aff90), levelBounds.getX(), levelBounds.getY(),
                false);
        } else if (smoothLevel < 0.85f) {
            gradient = juce::ColourGradient(
                juce::Colour(0xfffff036), levelBounds.getX(), levelBounds.getBottom(),
                juce::Colour(0xffffcc36), levelBounds.getX(), levelBounds.getY(),
                false);
        } else {
            gradient = juce::ColourGradient(
                juce::Colour(0xffff3636), levelBounds.getX(), levelBounds.getBottom(),
                juce::Colour(0xffff5050), levelBounds.getX(), levelBounds.getY(),
                false);
        }
        
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(levelBounds, 3.0f);
        
        // Glow effect
        g.setColour(gradient.getColour(1).withAlpha(0.3f));
        g.fillRoundedRectangle(levelBounds.expanded(2), 4.0f);
        
        // Peak hold indicator with glow
        if (smoothPeakLevel > 0.01f) {
            float peakY = bounds.getBottom() - (bounds.getHeight() * smoothPeakLevel);
            
            // Peak glow
            g.setColour(juce::Colour(0xffffffff).withAlpha(0.4f));
            g.fillRect(bounds.getX(), peakY - 2, bounds.getWidth(), 4.0f);
            
            // Peak line
            g.setColour(juce::Colours::white);
            g.fillRect(bounds.getX(), peakY - 1, bounds.getWidth(), 2.0f);
        }
        
        // Border
        g.setColour(juce::Colour(0xff0a0a0a));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
        
        // Scale marks
        g.setColour(juce::Colour(0xff3a3a3a));
        for (int i = 0; i <= 10; ++i) {
            float y = bounds.getY() + (bounds.getHeight() * i / 10.0f);
            g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
        }
    }
    
    void setLevel(float newLevel) {
        targetLevel = juce::jlimit(0.0f, 1.0f, newLevel);
        
        // Update peak
        if (targetLevel > peakLevel) {
            peakLevel = targetLevel;
            peakHoldCounter = 0;
        }
    }
    
    float getLevel() const { return smoothLevel; }
    
private:
    float targetLevel = 0.0f;
    float smoothLevel = 0.0f;
    float peakLevel = 0.0f;
    float smoothPeakLevel = 0.0f;
    int peakHoldCounter = 0;
    
    void timerCallback() override {
        // Smooth level animation (attack/release)
        float attack = 0.8f;  // Fast rise
        float release = 0.15f; // Slow fall
        
        if (targetLevel > smoothLevel) {
            smoothLevel += (targetLevel - smoothLevel) * attack;
        } else {
            smoothLevel += (targetLevel - smoothLevel) * release;
        }
        
        // Smooth peak level
        smoothPeakLevel += (peakLevel - smoothPeakLevel) * 0.3f;
        
        // Peak hold decay
        if (++peakHoldCounter > 120) { // 2 seconds at 60fps
            peakLevel *= 0.95f;
            if (peakLevel < 0.01f) peakLevel = 0.0f;
        }
        
        repaint();
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnimatedLevelMeter)
};

//==============================================================================
// Fade Transition - Transición de fade entre componentes
//==============================================================================
class FadeTransition : public juce::Component,
                      private juce::Timer {
public:
    FadeTransition() = default;
    
    void paint(juce::Graphics& g) override {
        if (fromComponent && toComponent) {
            // Draw from component (fading out)
            g.saveState();
            g.reduceClipRegion(getLocalBounds());
            g.setOpacity(1.0f - progress);
            fromComponent->paintEntireComponent(g, false);
            g.restoreState();
            
            // Draw to component (fading in)
            g.saveState();
            g.reduceClipRegion(getLocalBounds());
            g.setOpacity(progress);
            toComponent->paintEntireComponent(g, false);
            g.restoreState();
        }
    }
    
    void startTransition(juce::Component* from, juce::Component* to, int durationMs = 300) {
        fromComponent = from;
        toComponent = to;
        duration = durationMs;
        progress = 0.0f;
        elapsed = 0;
        
        if (from) addAndMakeVisible(from);
        if (to) addAndMakeVisible(to);
        
        startTimerHz(60);
    }
    
    std::function<void()> onTransitionComplete;
    
private:
    juce::Component* fromComponent = nullptr;
    juce::Component* toComponent = nullptr;
    float progress = 0.0f;
    int duration = 300;
    int elapsed = 0;
    
    void timerCallback() override {
        elapsed += getTimerInterval();
        progress = juce::jlimit(0.0f, 1.0f, (float)elapsed / (float)duration);
        
        if (progress >= 1.0f) {
            stopTimer();
            
            if (fromComponent) {
                removeChildComponent(fromComponent);
            }
            
            if (onTransitionComplete) {
                onTransitionComplete();
            }
        }
        
        repaint();
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FadeTransition)
};

//==============================================================================
// Pulse Effect - Efecto de pulso en recording
//==============================================================================
class PulseEffect : public juce::Component,
                   private juce::Timer {
public:
    PulseEffect() {
        startTimer(16);
    }
    
    void paint(juce::Graphics& g) override {
        if (!isActive) return;
        
        auto bounds = getLocalBounds().toFloat();
        auto centre = bounds.getCentre();
        
        // Multiple expanding circles
        for (int i = 0; i < 3; ++i) {
            float offset = (pulsePhase + i * juce::MathConstants<float>::twoPi / 3.0f);
            float radius = 20.0f + 40.0f * (std::sin(offset) * 0.5f + 0.5f);
            float alpha = 0.5f * (1.0f - (std::sin(offset) * 0.5f + 0.5f));
            
            g.setColour(juce::Colour(0xffff3636).withAlpha(alpha));
            g.drawEllipse(centre.x - radius, centre.y - radius, 
                         radius * 2, radius * 2, 3.0f);
        }
    }
    
    void setActive(bool active) {
        isActive = active;
        repaint();
    }
    
private:
    bool isActive = false;
    float pulsePhase = 0.0f;
    
    void timerCallback() override {
        if (isActive) {
            pulsePhase += 0.15f;
            if (pulsePhase > juce::MathConstants<float>::twoPi) {
                pulsePhase = 0.0f;
            }
            repaint();
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseEffect)
};

} // namespace GUI
} // namespace OmegaStudio
