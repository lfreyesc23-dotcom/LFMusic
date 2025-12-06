#pragma once

#include <JuceHeader.h>

namespace OmegaStudio {
namespace GUI {

//==============================================================================
/**
 * Automation Point - Single point in automation curve
 */
struct AutomationPoint
{
    double time;      // In beats
    float value;      // Normalized 0.0-1.0
    float curve;      // Bezier curve amount (-1.0 to 1.0)
    
    enum class Type { Linear, Bezier, Step };
    Type type = Type::Linear;
    
    AutomationPoint(double t = 0.0, float v = 0.5f, float c = 0.0f)
        : time(t), value(v), curve(c) {}
};

//==============================================================================
/**
 * Automation Clip - Contains automation data for a parameter
 */
class AutomationClip
{
public:
    AutomationClip(const juce::String& paramName)
        : parameterName(paramName)
    {
        // Add default points
        points.push_back(AutomationPoint(0.0, 0.5f));
        points.push_back(AutomationPoint(4.0, 0.5f));
    }

    void addPoint(double time, float value, float curve = 0.0f)
    {
        points.push_back(AutomationPoint(time, value, curve));
        sortPoints();
    }

    void removePoint(int index)
    {
        if (index >= 0 && index < static_cast<int>(points.size()))
        {
            points.erase(points.begin() + index);
        }
    }

    void movePoint(int index, double newTime, float newValue)
    {
        if (index >= 0 && index < static_cast<int>(points.size()))
        {
            points[index].time = newTime;
            points[index].value = juce::jlimit(0.0f, 1.0f, newValue);
            sortPoints();
        }
    }

    float getValueAtTime(double time) const
    {
        if (points.empty())
            return 0.5f;
        
        if (time <= points.front().time)
            return points.front().value;
        
        if (time >= points.back().time)
            return points.back().value;
        
        // Find surrounding points
        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            if (time >= points[i].time && time <= points[i + 1].time)
            {
                auto& p1 = points[i];
                auto& p2 = points[i + 1];
                
                if (p1.type == AutomationPoint::Type::Step)
                    return p1.value;
                
                float t = static_cast<float>((time - p1.time) / (p2.time - p1.time));
                
                if (p1.type == AutomationPoint::Type::Bezier)
                {
                    // Simple bezier interpolation
                    float curve = p1.curve;
                    t = t * t * (3.0f - 2.0f * t); // Smoothstep
                    t = juce::jmap(t, 0.0f, 1.0f, -curve, 1.0f + curve);
                    t = juce::jlimit(0.0f, 1.0f, t);
                }
                
                return juce::jmap(t, p1.value, p2.value);
            }
        }
        
        return 0.5f;
    }

    const std::vector<AutomationPoint>& getPoints() const { return points; }
    std::vector<AutomationPoint>& getPoints() { return points; }
    
    const juce::String& getParameterName() const { return parameterName; }
    void setParameterName(const juce::String& name) { parameterName = name; }

private:
    void sortPoints()
    {
        std::sort(points.begin(), points.end(),
                 [](const AutomationPoint& a, const AutomationPoint& b) {
                     return a.time < b.time;
                 });
    }

    juce::String parameterName;
    std::vector<AutomationPoint> points;
};

//==============================================================================
/**
 * Automation Lane Component - Visual editor for automation curves
 */
class AutomationLaneComponent : public juce::Component
{
public:
    AutomationLaneComponent(AutomationClip& clip)
        : automationClip(clip)
    {
        setSize(800, 100);
        setInterceptsMouseClicks(true, true);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(bounds);
        
        // Grid lines
        g.setColour(juce::Colours::grey.withAlpha(0.2f));
        int numLines = 8;
        for (int i = 0; i <= numLines; ++i)
        {
            float y = bounds.getHeight() * (i / static_cast<float>(numLines));
            g.drawLine(0.0f, y, bounds.getWidth(), y, 1.0f);
        }
        
        // Draw automation curve
        drawAutomationCurve(g, bounds);
        
        // Draw points
        drawPoints(g, bounds);
        
        // Border
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(bounds, 1.0f);
        
        // Parameter name
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);
        g.drawText(automationClip.getParameterName(), 
                  bounds.reduced(5.0f), 
                  juce::Justification::topLeft);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            // Remove point
            int pointIndex = findPointAtPosition(e.position);
            if (pointIndex >= 0)
            {
                automationClip.removePoint(pointIndex);
                repaint();
                return;
            }
        }
        else if (e.mods.isLeftButtonDown())
        {
            // Check if clicking existing point
            draggingPointIndex = findPointAtPosition(e.position);
            
            if (draggingPointIndex < 0)
            {
                // Add new point
                auto time = pixelToTime(e.position.x);
                auto value = pixelToValue(e.position.y);
                automationClip.addPoint(time, value);
                draggingPointIndex = findPointAtPosition(e.position);
            }
            
            repaint();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (draggingPointIndex >= 0)
        {
            auto time = pixelToTime(e.position.x);
            auto value = pixelToValue(e.position.y);
            automationClip.movePoint(draggingPointIndex, time, value);
            repaint();
        }
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggingPointIndex = -1;
    }

    void setPixelsPerBeat(float ppb)
    {
        pixelsPerBeat = ppb;
        repaint();
    }

    void setViewRange(double startBeat, double endBeat)
    {
        viewStartBeat = startBeat;
        viewEndBeat = endBeat;
        repaint();
    }

private:
    void drawAutomationCurve(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        juce::Path curvePath;
        bool first = true;
        
        int numSamples = static_cast<int>(bounds.getWidth());
        for (int x = 0; x < numSamples; ++x)
        {
            double time = pixelToTime(static_cast<float>(x));
            float value = automationClip.getValueAtTime(time);
            float y = valueToPixel(value);
            
            if (first)
            {
                curvePath.startNewSubPath(static_cast<float>(x), y);
                first = false;
            }
            else
            {
                curvePath.lineTo(static_cast<float>(x), y);
            }
        }
        
        g.setColour(juce::Colours::orange);
        g.strokePath(curvePath, juce::PathStrokeType(2.0f));
    }

    void drawPoints(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        const auto& points = automationClip.getPoints();
        
        for (size_t i = 0; i < points.size(); ++i)
        {
            float x = timeToPixel(points[i].time);
            float y = valueToPixel(points[i].value);
            
            // Point circle
            juce::Rectangle<float> pointRect(x - 5.0f, y - 5.0f, 10.0f, 10.0f);
            
            if (static_cast<int>(i) == draggingPointIndex)
            {
                g.setColour(juce::Colours::yellow);
                g.fillEllipse(pointRect);
            }
            else
            {
                g.setColour(juce::Colours::white);
                g.fillEllipse(pointRect);
                g.setColour(juce::Colours::orange);
                g.drawEllipse(pointRect, 2.0f);
            }
        }
    }

    int findPointAtPosition(juce::Point<float> pos)
    {
        const auto& points = automationClip.getPoints();
        
        for (size_t i = 0; i < points.size(); ++i)
        {
            float x = timeToPixel(points[i].time);
            float y = valueToPixel(points[i].value);
            
            if (pos.getDistanceFrom(juce::Point<float>(x, y)) < 10.0f)
                return static_cast<int>(i);
        }
        
        return -1;
    }

    float timeToPixel(double time) const
    {
        return static_cast<float>((time - viewStartBeat) * pixelsPerBeat);
    }

    double pixelToTime(float pixel) const
    {
        return viewStartBeat + (pixel / pixelsPerBeat);
    }

    float valueToPixel(float value) const
    {
        return getHeight() * (1.0f - value);
    }

    float pixelToValue(float pixel) const
    {
        return 1.0f - (pixel / getHeight());
    }

    AutomationClip& automationClip;
    int draggingPointIndex = -1;
    
    float pixelsPerBeat = 100.0f;
    double viewStartBeat = 0.0;
    double viewEndBeat = 16.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLaneComponent)
};

//==============================================================================
/**
 * Automation Editor Window
 */
class AutomationEditor : public juce::Component
{
public:
    AutomationEditor()
    {
        setupToolbar();
        
        // Create viewport for lanes
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&laneContainer, false);
        viewport.setScrollBarsShown(true, false);
        
        // Add test automation clip
        addAutomationLane("Volume");
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        
        // Toolbar
        auto toolbarBounds = bounds.removeFromTop(40);
        
        addLaneButton.setBounds(toolbarBounds.removeFromLeft(100).reduced(5));
        toolbarBounds.removeFromLeft(10);
        
        modeLabel.setBounds(toolbarBounds.removeFromLeft(50).reduced(5));
        modeSelector.setBounds(toolbarBounds.removeFromLeft(120).reduced(5));
        
        // Viewport
        viewport.setBounds(bounds);
        updateLaneLayout();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff2b2b2b));
    }

    void addAutomationLane(const juce::String& parameterName)
    {
        auto clip = std::make_unique<AutomationClip>(parameterName);
        auto lane = std::make_unique<AutomationLaneComponent>(*clip);
        
        laneContainer.addAndMakeVisible(lane.get());
        
        automationClips.push_back(std::move(clip));
        lanes.push_back(std::move(lane));
        
        updateLaneLayout();
    }

private:
    void setupToolbar()
    {
        addAndMakeVisible(addLaneButton);
        addLaneButton.setButtonText("+ Add Lane");
        addLaneButton.onClick = [this]() {
            addAutomationLane("Parameter " + juce::String(lanes.size() + 1));
        };
        
        addAndMakeVisible(modeLabel);
        modeLabel.setText("Mode:", juce::dontSendNotification);
        
        addAndMakeVisible(modeSelector);
        modeSelector.addItem("Linear", 1);
        modeSelector.addItem("Bezier", 2);
        modeSelector.addItem("Step", 3);
        modeSelector.setSelectedId(1);
    }

    void updateLaneLayout()
    {
        int totalHeight = lanes.size() * 120;
        laneContainer.setSize(viewport.getWidth() - viewport.getScrollBarThickness(), totalHeight);
        
        int y = 0;
        for (auto& lane : lanes)
        {
            lane->setBounds(0, y, laneContainer.getWidth(), 100);
            lane->setPixelsPerBeat(50.0f);
            y += 120;
        }
    }

    juce::Viewport viewport;
    juce::Component laneContainer;
    
    juce::TextButton addLaneButton;
    juce::Label modeLabel;
    juce::ComboBox modeSelector;
    
    std::vector<std::unique_ptr<AutomationClip>> automationClips;
    std::vector<std::unique_ptr<AutomationLaneComponent>> lanes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationEditor)
};

//==============================================================================
/**
 * Automation Window
 */
class AutomationWindow : public juce::DocumentWindow
{
public:
    AutomationWindow()
        : DocumentWindow("Automation Editor",
                        juce::Colours::darkgrey,
                        DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(&editor, true);
        setResizable(true, false);
        centreWithSize(1000, 600);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    AutomationEditor editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationWindow)
};

} // namespace GUI
} // namespace OmegaStudio
