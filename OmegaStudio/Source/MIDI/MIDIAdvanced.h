#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>
#include <atomic>
#include <array>
#include <cstring>
#include "../Memory/LockFreeFIFO.h"

namespace OmegaStudio {
namespace MIDI {

/**
 * @brief MIDI Out - Send MIDI to External Devices (FL Studio)
 */
class MIDIOut
{
public:
    MIDIOut();
    ~MIDIOut();
    
    // Device management
    bool openDevice(const juce::String& deviceName);
    void closeDevice();
    bool isDeviceOpen() const { return deviceOpen; }
    juce::String getCurrentDevice() const { return currentDevice; }
    static juce::StringArray getAvailableDevices();
    
    // Send MIDI messages
    void sendNoteOn(int channel, int note, uint8 velocity);
    void sendNoteOff(int channel, int note, uint8 velocity = 0);
    void sendControlChange(int channel, int controller, uint8 value);
    void sendProgramChange(int channel, int program);
    void sendPitchBend(int channel, int value);
    void sendAftertouch(int channel, uint8 pressure);
    void sendPolyAftertouch(int channel, int note, uint8 pressure);
    
    // System messages
    void sendClock();
    void sendStart();
    void sendStop();
    void sendContinue();
    void sendSongPosition(int beats);
    
    // SysEx
    void sendSysEx(const std::vector<uint8>& data);
    
    // Panic (all notes off)
    void panic();

private:
    std::unique_ptr<juce::MidiOutput> midiOutput;
    bool deviceOpen = false;
    juce::String currentDevice;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIOut)
};

/**
 * @brief Fruity LSD - Hardware Link/Control Surface Driver (FL Studio)
 */
class FruityLSD
{
public:
    struct ControlMapping {
        int controllerId;
        int midiCC;
        int midiChannel;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        juce::String targetParameter;
    };
    
    FruityLSD();
    ~FruityLSD();
    
    // Device connection
    bool connectDevice(const juce::String& midiInputDevice, const juce::String& midiOutputDevice);
    void disconnectDevice();
    bool isConnected() const { return connected; }
    
    // Mapping
    void addMapping(const ControlMapping& mapping);
    void removeMapping(int controllerId);
    void clearMappings();
    std::vector<ControlMapping>& getMappings() { return mappings; }
    
    // Learn mode
    void enterLearnMode(const juce::String& targetParameter);
    void exitLearnMode();
    bool isInLearnMode() const { return learnMode; }
    
    // Preset management
    void savePreset(const juce::String& name);
    void loadPreset(const juce::String& name);
    std::vector<juce::String> getPresetNames() const;
    
    // Feedback to hardware
    void updateLED(int controllerId, bool state);
    void updateFader(int controllerId, float value);
    void updateDisplay(const juce::String& text);
    
    // Process incoming MIDI
    void processMidiMessage(const juce::MidiMessage& message);

private:
    std::unique_ptr<MIDIOut> midiOut;
    std::unique_ptr<juce::MidiInput> midiIn;
    bool connected = false;
    bool learnMode = false;
    juce::String learnTarget;
    std::vector<ControlMapping> mappings;
    std::map<juce::String, std::vector<ControlMapping>> presets;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FruityLSD)
};

/**
 * @brief Control Surface - Virtual MIDI Controller (FL Studio)
 */
class ControlSurface : public juce::Component
{
public:
    enum class ControlType {
        Knob,
        Slider,
        Button,
        XYPad,
        Keyboard
    };
    
    struct Control {
        ControlType type;
        int id;
        juce::Rectangle<int> bounds;
        float value = 0.0f;
        bool momentary = false;
        juce::String label;
        juce::Colour colour;
    };
    
    ControlSurface();
    ~ControlSurface() override;
    
    // Control management
    void addControl(const Control& control);
    void removeControl(int id);
    void clearControls();
    Control* getControl(int id);
    
    // Value callbacks
    std::function<void(int id, float value)> onControlChanged;
    
    // Layout
    void loadLayout(const juce::File& file);
    void saveLayout(const juce::File& file);
    
    // MIDI mapping
    void mapControlToMidi(int controlId, int midiCC, int channel = 1);
    void sendMidiForControl(int controlId);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    std::vector<Control> controls;
    int draggedControlId = -1;
    
    void drawKnob(juce::Graphics& g, const Control& control);
    void drawSlider(juce::Graphics& g, const Control& control);
    void drawButton(juce::Graphics& g, const Control& control);
    void drawXYPad(juce::Graphics& g, const Control& control);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlSurface)
};

/**
 * @brief Fruity Voltage Controller - CV/Gate Output (FL Studio)
 */
class FruityVoltageController
{
public:
    struct CVOutput {
        int channel;           // 1-8
        float voltage;         // -10V to +10V
        bool gate;            // On/Off
        float frequency;      // For pitch CV
    };
    
    FruityVoltageController();
    ~FruityVoltageController();
    
    // Hardware interface
    bool connectHardware(const juce::String& deviceName);
    void disconnectHardware();
    bool isConnected() const { return connected; }
    
    // CV output
    void setCVOutput(int channel, float voltage);
    void setGateOutput(int channel, bool state);
    void setPitchCV(int channel, int midiNote, float pitchBend = 0.0f);
    
    // Calibration
    void calibrateOutput(int channel);
    void loadCalibration(const juce::File& file);
    void saveCalibration(const juce::File& file);
    
    // Get current outputs
    CVOutput getOutput(int channel) const;
    static constexpr int NumChannels = 8;

private:
    bool connected = false;
    std::array<CVOutput, NumChannels> outputs;
    std::array<float, NumChannels> calibrationOffsets;
    
    float midiNoteToVoltage(int note, float bend = 0.0f);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FruityVoltageController)
};

/**
 * @brief MIDI Scripting Engine (FL Studio)
 */
class MIDIScriptEngine
{
public:
    MIDIScriptEngine();
    ~MIDIScriptEngine();
    
    // Script management
    bool loadScript(const juce::File& file);
    bool loadScriptFromString(const juce::String& script);
    void unloadScript();
    bool isScriptLoaded() const { return scriptLoaded; }
    
    // Execute script functions
    void onMidiInput(const juce::MidiMessage& message);
    void onTransportChanged(bool playing, double position);
    void onBPMChanged(double bpm);
    
    // Script API
    void registerFunction(const juce::String& name, std::function<void()> func);
    void setScriptVariable(const juce::String& name, float value);
    float getScriptVariable(const juce::String& name);
    
    // Error handling
    juce::String getLastError() const { return lastError; }

private:
    bool scriptLoaded = false;
    juce::String scriptContent;
    juce::String lastError;
    std::map<juce::String, float> variables;
    std::map<juce::String, std::function<void()>> functions;
    
    void executeScript(const juce::String& code);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIScriptEngine)
};

/**
 * @brief MIDI Manager - Central MIDI System
 */
class MIDIManager
{
public:
    MIDIManager();
    ~MIDIManager();

    // RT-safe MIDI event
    struct RTEvent {
        std::array<uint8_t, 64> bytes{}; // Fits common MIDI msgs; SysEx truncated if larger
        uint8_t size { 0 };
        int samplePosition { 0 }; // position within current block
        bool isSysEx { false };

        static RTEvent fromMessage(const juce::MidiMessage& msg, int samplePos = 0) {
            RTEvent ev;
            const int msgSize = juce::jmin<int>(static_cast<int>(msg.getRawDataSize()), static_cast<int>(ev.bytes.size()));
            std::memcpy(ev.bytes.data(), msg.getRawData(), static_cast<size_t>(msgSize));
            ev.size = static_cast<uint8_t>(msgSize);
            ev.samplePosition = samplePos;
            ev.isSysEx = msg.isSysEx();
            return ev;
        }

        juce::MidiMessage toMessage() const {
            return juce::MidiMessage(bytes.data(), static_cast<int>(size));
        }
    };

    // Lock-free queues (audio thread consumes)
    Memory::LockFreeFIFO<RTEvent, 1024>& getInputQueue() { return inputQueue; }
    Memory::LockFreeFIFO<RTEvent, 1024>& getOutputQueue() { return outputQueue; }
    
    // MIDI Out
    MIDIOut* getMIDIOut() { return midiOut.get(); }
    
    // Control surfaces
    FruityLSD* getLSD() { return lsd.get(); }
    ControlSurface* createControlSurface();
    std::vector<ControlSurface*> getControlSurfaces() { return controlSurfaces; }
    
    // Voltage controller
    FruityVoltageController* getVoltageController() { return voltageController.get(); }
    
    // Scripting
    MIDIScriptEngine* getScriptEngine() { return scriptEngine.get(); }
    
    // MIDI Learn
    void enableMIDILearn(const juce::String& targetParameter);
    void disableMIDILearn();
    bool isMIDILearnActive() const { return midiLearnActive; }
    
    // Global MIDI processing
    void processMidiMessage(const juce::MidiMessage& message);

private:
    std::unique_ptr<MIDIOut> midiOut;
    std::unique_ptr<FruityLSD> lsd;
    std::unique_ptr<FruityVoltageController> voltageController;
    std::unique_ptr<MIDIScriptEngine> scriptEngine;
    std::vector<ControlSurface*> controlSurfaces;
    
    bool midiLearnActive = false;
    juce::String midiLearnTarget;

    Memory::LockFreeFIFO<RTEvent, 1024> inputQueue;   // GUI → Audio (scripted)
    Memory::LockFreeFIFO<RTEvent, 1024> outputQueue;  // Audio → GUI / hardware send
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIManager)
};

} // namespace MIDI
} // namespace OmegaStudio
