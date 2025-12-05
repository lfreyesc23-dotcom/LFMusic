#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>

namespace OmegaStudio {
namespace GUI {

/**
 * @class LevelMeter
 * @brief Medidor de nivel VU/PPM
 */
class LevelMeter : public juce::Component,
                   private juce::Timer {
public:
    enum class Type { VU, PPM };
    
    LevelMeter(Type type = Type::PPM) : type_(type) {
        startTimer(30);  // 33 fps
    }
    
    void setLevel(float left, float right) {
        leftLevel_.store(left);
        rightLevel_.store(right);
        
        // Update peak hold
        if (left > leftPeak_.load()) {
            leftPeak_.store(left);
            leftPeakTime_ = 0;
        }
        if (right > rightPeak_.load()) {
            rightPeak_.store(right);
            rightPeakTime_ = 0;
        }
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillRect(bounds);
        
        float channelWidth = bounds.getWidth() / 2.0f;
        
        // Draw left channel
        drawChannel(g, bounds.withWidth(channelWidth), leftLevel_.load(), leftPeak_.load());
        
        // Draw right channel
        drawChannel(g, bounds.withX(bounds.getX() + channelWidth).withWidth(channelWidth),
                   rightLevel_.load(), rightPeak_.load());
        
        // Border
        g.setColour(juce::Colours::darkgrey);
        g.drawRect(bounds, 1.0f);
    }
    
private:
    void timerCallback() override {
        // Decay peaks
        leftPeakTime_++;
        rightPeakTime_++;
        
        if (leftPeakTime_ > 60) {  // 2 seconds
            leftPeak_.store(leftPeak_.load() * 0.95f);
        }
        if (rightPeakTime_ > 60) {
            rightPeak_.store(rightPeak_.load() * 0.95f);
        }
        
        // Smooth decay
        float decay = (type_ == Type::VU) ? 0.95f : 0.90f;
        leftLevel_.store(leftLevel_.load() * decay);
        rightLevel_.store(rightLevel_.load() * decay);
        
        repaint();
    }
    
    void drawChannel(juce::Graphics& g, juce::Rectangle<float> bounds, float level, float peak) {
        float height = bounds.getHeight();
        
        // Convert to dB
        float levelDb = juce::Decibels::gainToDecibels(level, -60.0f);
        float normLevel = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, 1.0f);
        
        float barHeight = height * normLevel;
        
        // Gradient colors (green -> yellow -> red)
        juce::ColourGradient gradient(
            juce::Colours::green, 0.0f, height,
            juce::Colours::red, 0.0f, 0.0f,
            false
        );
        gradient.addColour(0.7, juce::Colours::yellow);
        
        g.setGradientFill(gradient);
        g.fillRect(bounds.withTop(bounds.getBottom() - barHeight));
        
        // Peak indicator
        if (peak > 0.01f) {
            float peakDb = juce::Decibels::gainToDecibels(peak, -60.0f);
            float normPeak = juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, 1.0f);
            float peakY = height * (1.0f - normPeak);
            
            g.setColour(juce::Colours::white);
            g.drawHorizontalLine(peakY, bounds.getX(), bounds.getRight());
        }
        
        // Clipping indicator
        if (level > 0.995f) {
            g.setColour(juce::Colours::red);
            g.fillRect(bounds.removeFromTop(10));
        }
        
        // Scale marks
        g.setColour(juce::Colours::darkgrey);
        g.setFont(8.0f);
        
        std::vector<float> marks = { 0.0f, -3.0f, -6.0f, -12.0f, -24.0f, -48.0f };
        for (float db : marks) {
            float y = height * (1.0f - juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f));
            g.drawHorizontalLine(y, bounds.getX(), bounds.getX() + 3.0f);
        }
    }
    
    Type type_;
    std::atomic<float> leftLevel_ { 0.0f };
    std::atomic<float> rightLevel_ { 0.0f };
    std::atomic<float> leftPeak_ { 0.0f };
    std::atomic<float> rightPeak_ { 0.0f };
    int leftPeakTime_ { 0 };
    int rightPeakTime_ { 0 };
};

/**
 * @class ChannelStrip
 * @brief Tira de canal con fader, pan, mute, solo, meter
 */
class ChannelStrip : public juce::Component {
public:
    ChannelStrip(const juce::String& name) : channelName_(name) {
        // Fader
        addAndMakeVisible(fader_);
        fader_.setSliderStyle(juce::Slider::LinearVertical);
        fader_.setRange(-60.0, 6.0, 0.1);  // dB
        fader_.setValue(0.0);
        fader_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        fader_.onValueChange = [this] {
            if (onGainChanged) onGainChanged(fader_.getValue());
        };
        
        // Pan knob
        addAndMakeVisible(panKnob_);
        panKnob_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panKnob_.setRange(-1.0, 1.0, 0.01);
        panKnob_.setValue(0.0);
        panKnob_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panKnob_.onValueChange = [this] {
            if (onPanChanged) onPanChanged(panKnob_.getValue());
        };
        
        // Mute button
        addAndMakeVisible(muteButton_);
        muteButton_.setButtonText("M");
        muteButton_.setToggleState(false, juce::dontSendNotification);
        muteButton_.onClick = [this] {
            if (onMuteChanged) onMuteChanged(muteButton_.getToggleState());
        };
        
        // Solo button
        addAndMakeVisible(soloButton_);
        soloButton_.setButtonText("S");
        soloButton_.setToggleState(false, juce::dontSendNotification);
        soloButton_.onClick = [this] {
            if (onSoloChanged) onSoloChanged(soloButton_.getToggleState());
        };
        
        // Record arm button
        addAndMakeVisible(recButton_);
        recButton_.setButtonText("R");
        recButton_.setToggleState(false, juce::dontSendNotification);
        
        // Level meter
        addAndMakeVisible(meter_);
        
        // Name label
        addAndMakeVisible(nameLabel_);
        nameLabel_.setText(channelName_, juce::dontSendNotification);
        nameLabel_.setJustificationType(juce::Justification::centred);
        nameLabel_.setEditable(true);
        
        // Send knobs (4 sends)
        for (int i = 0; i < 4; ++i) {
            auto knob = std::make_unique<juce::Slider>();
            knob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            knob->setRange(0.0, 1.0, 0.01);
            knob->setValue(0.0);
            knob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            addAndMakeVisible(*knob);
            sendKnobs_.push_back(std::move(knob));
        }
        
        // Insert slots (visual indicators)
        for (int i = 0; i < 6; ++i) {
            insertSlots_.push_back(false);
        }
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff2a2a2a));
        
        g.setColour(juce::Colour(0xff404040));
        g.drawRect(getLocalBounds(), 2);
        
        // Draw insert slots
        auto insertArea = juce::Rectangle<int>(10, 50, getWidth() - 20, 120);
        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillRect(insertArea);
        
        g.setColour(juce::Colours::grey);
        g.setFont(10.0f);
        
        for (int i = 0; i < 6; ++i) {
            int y = insertArea.getY() + i * 20;
            juce::Rectangle<int> slotRect(insertArea.getX() + 5, y + 2, 
                                         insertArea.getWidth() - 10, 16);
            
            if (insertSlots_[i]) {
                g.setColour(juce::Colours::cyan);
                g.fillRect(slotRect);
                g.setColour(juce::Colours::black);
            } else {
                g.setColour(juce::Colour(0xff3a3a3a));
                g.fillRect(slotRect);
                g.setColour(juce::Colours::grey);
            }
            
            g.drawRect(slotRect);
            g.drawText(juce::String(i + 1), slotRect, juce::Justification::centred);
        }
        
        // Pan label
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        g.drawText("PAN", panKnob_.getBounds().translated(0, -15), 
                  juce::Justification::centred);
        
        // Send labels
        for (int i = 0; i < 4; ++i) {
            g.drawText("S" + juce::String(i + 1), 
                      sendKnobs_[i]->getBounds().translated(0, -15),
                      juce::Justification::centred);
        }
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(5);
        
        // Name at top
        nameLabel_.setBounds(bounds.removeFromTop(20));
        
        bounds.removeFromTop(5);
        
        // Insert slots (drawn in paint)
        bounds.removeFromTop(125);
        
        // Pan knob
        panKnob_.setBounds(bounds.removeFromTop(50).reduced(10));
        
        // Send knobs
        for (auto& knob : sendKnobs_) {
            knob->setBounds(bounds.removeFromTop(40).reduced(10));
        }
        
        bounds.removeFromTop(10);
        
        // Buttons row
        auto buttonRow = bounds.removeFromTop(25);
        int buttonWidth = buttonRow.getWidth() / 3;
        muteButton_.setBounds(buttonRow.removeFromLeft(buttonWidth).reduced(2));
        soloButton_.setBounds(buttonRow.removeFromLeft(buttonWidth).reduced(2));
        recButton_.setBounds(buttonRow.reduced(2));
        
        bounds.removeFromTop(5);
        
        // Meter
        meter_.setBounds(bounds.removeFromTop(150).reduced(10, 0));
        
        bounds.removeFromTop(5);
        
        // Fader
        fader_.setBounds(bounds.reduced(10, 0));
    }
    
    void setLevel(float left, float right) {
        meter_.setLevel(left, right);
    }
    
    void setInsertEnabled(int index, bool enabled) {
        if (index >= 0 && index < 6) {
            insertSlots_[index] = enabled;
            repaint();
        }
    }
    
    std::function<void(double)> onGainChanged;
    std::function<void(double)> onPanChanged;
    std::function<void(bool)> onMuteChanged;
    std::function<void(bool)> onSoloChanged;
    
private:
    juce::String channelName_;
    
    juce::Slider fader_;
    juce::Slider panKnob_;
    juce::TextButton muteButton_;
    juce::TextButton soloButton_;
    juce::TextButton recButton_;
    
    LevelMeter meter_;
    juce::Label nameLabel_;
    
    std::vector<std::unique_ptr<juce::Slider>> sendKnobs_;
    std::vector<bool> insertSlots_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
};

/**
 * @class MixerView
 * @brief Vista de mezclador completa
 */
class MixerView : public juce::Component,
                  public juce::ScrollBar::Listener {
public:
    MixerView() {
        // Create 16 channel strips
        for (int i = 0; i < 16; ++i) {
            auto strip = std::make_unique<ChannelStrip>("Track " + juce::String(i + 1));
            addAndMakeVisible(*strip);
            channelStrips_.push_back(std::move(strip));
        }
        
        // Master channel
        masterStrip_ = std::make_unique<ChannelStrip>("Master");
        addAndMakeVisible(*masterStrip_);
        
        // Scrollbar
        addAndMakeVisible(scrollBar_);
        scrollBar_.setRangeLimits(0.0, 16.0);
        scrollBar_.setCurrentRange(0.0, 8.0);  // Show 8 channels at a time
        scrollBar_.setAutoHide(false);
        scrollBar_.addListener(this);
        
        // Group buttons
        addAndMakeVisible(groupButton_);
        groupButton_.setButtonText("Group Selected");
        
        addAndMakeVisible(hideButton_);
        hideButton_.setButtonText("Hide Selected");
    }
    
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(0xff1e1e1e));
    }
    
    void resized() override {
        auto bounds = getLocalBounds();
        
        // Top toolbar
        auto toolbar = bounds.removeFromTop(40);
        groupButton_.setBounds(toolbar.removeFromLeft(120).reduced(5));
        hideButton_.setBounds(toolbar.removeFromLeft(120).reduced(5));
        
        // Master strip on the right
        auto masterArea = bounds.removeFromRight(100);
        masterStrip_->setBounds(masterArea);
        
        // Scrollbar
        auto scrollArea = bounds.removeFromRight(20);
        scrollBar_.setBounds(scrollArea);
        
        // Channel strips
        int stripWidth = 80;
        int visibleStrips = bounds.getWidth() / stripWidth;
        int startIndex = (int)scrollBar_.getCurrentRangeStart();
        
        for (int i = 0; i < visibleStrips && startIndex + i < (int)channelStrips_.size(); ++i) {
            int x = i * stripWidth;
            channelStrips_[startIndex + i]->setBounds(x, 0, stripWidth, bounds.getHeight());
        }
    }
    
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override {
        resized();
    }
    
    ChannelStrip* getChannelStrip(int index) {
        return (index >= 0 && index < (int)channelStrips_.size()) 
            ? channelStrips_[index].get() : nullptr;
    }
    
    ChannelStrip* getMasterStrip() { return masterStrip_.get(); }
    
    void updateMeters(const std::vector<std::pair<float, float>>& levels) {
        for (size_t i = 0; i < levels.size() && i < channelStrips_.size(); ++i) {
            channelStrips_[i]->setLevel(levels[i].first, levels[i].second);
        }
    }
    
private:
    std::vector<std::unique_ptr<ChannelStrip>> channelStrips_;
    std::unique_ptr<ChannelStrip> masterStrip_;
    
    juce::ScrollBar scrollBar_ { false };  // Vertical
    juce::TextButton groupButton_;
    juce::TextButton hideButton_;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerView)
};

} // namespace GUI
} // namespace OmegaStudio
