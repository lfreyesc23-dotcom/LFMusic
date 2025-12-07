cmake_minimum_required(VERSION 3.22)

# ============================================================================
# PROJECT CONFIGURATION
# ============================================================================
project(OmegaStudio VERSION 1.0.0 LANGUAGES C CXX)

# Require C++20 (modern features compatible with JUCE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# JUCE FRAMEWORK INTEGRATION
# ============================================================================
# Clone JUCE if not present: git clone https://github.com/juce-framework/JUCE.git
# Or install via package manager

# Disable building JUCE extras (demos, tools)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Build JUCE Extras" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "Build JUCE Examples" FORCE)

# Option 1: If JUCE is in a subdirectory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
    message(STATUS "Using JUCE from subdirectory")
else()
    # Option 2: Find JUCE if installed system-wide
    find_package(JUCE CONFIG REQUIRED)
    message(STATUS "Using system-installed JUCE")
endif()

# ============================================================================
# COMPILER FLAGS & OPTIMIZATIONS
# ============================================================================
if(MSVC)
    # Windows: Visual Studio optimizations
    add_compile_options(
        /W4                 # Warning level 4
        /WX                 # Treat warnings as errors
        /permissive-        # Standards conformance
        /Zc:__cplusplus     # Enable __cplusplus macro
        /arch:AVX2          # Enable AVX2 SIMD instructions
        /fp:fast            # Fast floating point
        /Oi                 # Enable intrinsic functions
        /MP                 # Multi-processor compilation
    )
    
    # Release optimizations
    add_compile_options($<$<CONFIG:Release>:/O2 /Ob2 /GL>)
    add_link_options($<$<CONFIG:Release>:/LTCG>)
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    # macOS: Clang optimizations
    add_compile_options(
        -Wall
        -Wno-unused-parameter
        -Wno-sign-conversion
        -Wno-shorten-64-to-32
        -Wno-unused-function
        -Wno-deprecated-declarations
    )
    
    # Detect Apple Silicon vs Intel
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
        message(STATUS "Building for Apple Silicon (ARM64) - Using NEON")
        add_compile_options(-mcpu=apple-m1)
    else()
        message(STATUS "Building for Intel - Using AVX2")
        add_compile_options(-mavx2 -mfma)
    endif()
    
    # Release optimizations
    add_compile_options($<$<CONFIG:Release>:-O3>)
    add_compile_options($<$<CONFIG:Release>:-flto>)
    add_link_options($<$<CONFIG:Release>:-flto>)
    
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Linux: GCC optimizations
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Werror
        -ffast-math
        -mavx2 -mfma
    )
    add_compile_options($<$<CONFIG:Release>:-O3>)
    add_compile_options($<$<CONFIG:Release>:-flto>)
    add_link_options($<$<CONFIG:Release>:-flto>)
endif()

# ============================================================================
# JUCE APPLICATION SETUP
# ============================================================================
juce_add_gui_app(OmegaStudio
    PRODUCT_NAME "Omega Studio"
    COMPANY_NAME "AudioTech"
    BUNDLE_ID "com.audiotech.omegastudio"
    
    # Formats
    FORMATS Standalone
    
    # Plugin characteristics (for future plugin hosting)
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    
    # Copy plugin to standard locations
    COPY_PLUGIN_AFTER_BUILD FALSE
    
    # Generate JuceHeader.h for legacy compatibility
    JUCE_GENERATE_JUCE_HEADER TRUE
)

# ============================================================================
# SOURCE FILES
# ============================================================================
target_sources(OmegaStudio PRIVATE
    # Core Application
    Source/Core/Main.cpp
    Source/Core/Application.h
    Source/Core/Application.cpp
    
    # Audio Engine
    Source/Audio/Engine/AudioEngine.h
    Source/Audio/Engine/AudioEngine.cpp
    Source/Audio/Engine/AudioCallback.h
    Source/Audio/Engine/AudioCallback.cpp
    
    # Audio Graph
    Source/Audio/Graph/AudioGraph.h
    Source/Audio/Graph/AudioGraph.cpp
    Source/Audio/Graph/AudioNode.h
    Source/Audio/Graph/AudioNode.cpp
    Source/Audio/Graph/ProcessorNodes.h
    Source/Audio/Graph/ProcessorNodes.cpp
    
    # DSP
    Source/Audio/DSP/SIMDProcessor.h
    Source/Audio/DSP/SIMDProcessor.cpp
    Source/Audio/DSP/PitchCorrection.h
    Source/Audio/DSP/PitchCorrection.cpp
    
    # NEW: Vocal Processing Bundle
    Source/Audio/DSP/VocalComping.h
    Source/Audio/DSP/VocalComping.cpp
    Source/Audio/DSP/VocalHarmonizer.h
    Source/Audio/DSP/VocalHarmonizer.cpp
    Source/Audio/DSP/DeEsser.h
    Source/Audio/DSP/DeEsser.cpp
    Source/Audio/DSP/BreathControl.h
    Source/Audio/DSP/BreathControl.cpp
    Source/Audio/DSP/VocalProcessingBundle.h
    Source/Audio/DSP/VocalProcessingBundle.cpp
    
    # NEW: Mastering Bundle
    Source/Audio/DSP/MasteringBundle.h
    Source/Audio/DSP/MasteringBundle.cpp
    
    # NEW: Content Library & Sample Slicer
    Source/Content/ContentLibrary.h
    Source/Content/ContentLibrary.cpp
    Source/Audio/SampleSlicer.h
    Source/Audio/SampleSlicer.cpp
    Source/Audio/AudioToMidi.h
    Source/Audio/AudioToMidi.cpp
    
    # Recording
    Source/Audio/Recording/AudioRecorder.h
    Source/Audio/Recording/AudioRecorder.cpp
    
    # Sample Library
    Source/Audio/Library/SampleManager.h
    Source/Audio/Library/SampleManager.cpp
    
    # AI Processing
    Source/Audio/AI/VocalEnhancer.h
    Source/Audio/AI/VocalEnhancer.cpp
    Source/Audio/AI/AIFeatures.h
    Source/Audio/AI/AIFeatures.cpp
    
    # Memory Management
    Source/Memory/MemoryPool.h
    Source/Memory/MemoryPool.cpp
    Source/Memory/LockFreeFIFO.h
    
    # GUI
    Source/GUI/MainWindow.h
    Source/GUI/MainWindow.cpp
    Source/GUI/MainComponent.h
    Source/GUI/MainComponent.cpp
    Source/GUI/ProcessorPanels.h
    Source/GUI/ProcessorPanels.cpp
    Source/GUI/PianoRollEditor.h
    Source/GUI/PianoRollEditor.cpp
    Source/GUI/MixerEditor.h
    Source/GUI/MixerEditor.cpp
    Source/GUI/PlaylistEditor.h
    Source/GUI/PlaylistEditor.cpp
    Source/GUI/PluginWindowManager.h
    Source/GUI/PluginWindowManager.cpp
    Source/GUI/TransportBar.h
    
    # NEW GUI Components - FL Studio Killer
    Source/GUI/ChannelRackEditor.h
    Source/GUI/BrowserComponent.h
    Source/GUI/AutomationEditor.h
    Source/GUI/WaveformRenderer.h
    Source/GUI/WavetableSynthUI.h
    Source/GUI/FMSynthUI.h
    Source/GUI/DrumMachineUI.h
    Source/GUI/EffectUIs.h
    Source/GUI/ThemeManager.h
    Source/GUI/AudioEditorWindow.h
    Source/GUI/PerformanceModeWindow.h
    Source/GUI/MacroPanelComponent.h
    Source/GUI/StemSeparatorUI.h
    Source/GUI/SmartMixingAssistantUI.h
    
    # FL Studio-style Look & Feel
    Source/GUI/FLStudioLookAndFeel.h
    Source/GUI/FLStudioLookAndFeel.cpp
    Source/GUI/ChannelRackUI.h
    Source/GUI/ChannelRackUI.cpp
    Source/GUI/FLStudioMainWindow.h
    Source/GUI/FLStudioMainWindow.cpp
    
    # NEW: FL STUDIO 2025 INTERFACE (EXACT REPLICA)
    Source/GUI/FLStudio2025Interface.h
    Source/GUI/FLStudio2025Interface.cpp
    
    # Utils
    Source/Utils/Constants.h
    Source/Utils/Atomic.h
    
    # Project Management
    Source/Project/ProjectManager.h
    Source/Project/ProjectManager.cpp
    
    # Plugin System
    Source/Audio/Plugins/PluginManager.h
    Source/Audio/Plugins/PluginManager.cpp
    
    # MIDI Sequencer
    Source/Sequencer/MIDI/MIDIEngine.h
    Source/Sequencer/MIDI/MIDIEngine.cpp
    
    # Timeline/Arrangement
    Source/Sequencer/Timeline/Timeline.h
    Source/Sequencer/Timeline/Timeline.cpp
    
    # Mixer
    Source/Mixer/MixerEngine.h
    Source/Mixer/MixerEngine.cpp
    Source/Mixer/ChannelStrip.h
    Source/Mixer/ChannelStrip.cpp
    
    # Professional Effects
    Source/Audio/DSP/ProfessionalEffects.h
    Source/Audio/DSP/ProfessionalEffects.cpp
    
    # Spectral Analysis
    Source/Audio/Analysis/SpectralAnalyzer.h
    Source/Audio/Analysis/SpectralAnalyzer.cpp
    
    # Automation System
    Source/Sequencer/Automation/AutomationSystem.h
    Source/Sequencer/Automation/AutomationSystem.cpp
    
    # Built-in Instruments
    Source/Audio/Instruments/Instruments.h
    Source/Audio/Instruments/Instruments.cpp
    
    # Advanced AI Features
    Source/Audio/AI/AdvancedAI.h
    Source/Audio/AI/AdvancedAI.cpp
    
    # New Professional Features (v2.0) - Compiled and working
    Source/Sequencer/StepSequencer.h
    Source/Sequencer/StepSequencer.cpp
    Source/Sequencer/PianoRoll.h
    Source/Sequencer/PianoRoll.cpp
    Source/Sequencer/MIDIFX.h
    Source/Sequencer/MIDIFX.cpp
    Source/Audio/DSP/TimeStretch.h
    Source/Audio/DSP/TimeStretch.cpp
    Source/Audio/DSP/SidechainCompression.h
    Source/Audio/DSP/SidechainCompression.cpp
    
    # Velocity Layers & Multi-sampling
    Source/Audio/VelocityLayers.h
    Source/Audio/VelocityLayers.cpp
    
    # Reference Matching & Analysis
    Source/Audio/Analysis/ReferenceMatching.h
    Source/Audio/Analysis/ReferenceMatching.cpp
    Source/Audio/Analysis/AdvancedVisualizers.h
    Source/Audio/Analysis/AdvancedVisualizers.cpp
    Source/Audio/Analysis/DynamicRangeAnalyzer.h
    Source/Audio/Analysis/DynamicRangeAnalyzer.cpp
    Source/Audio/Analysis/TonalBalance.h
    Source/Audio/Analysis/TonalBalance.cpp
    
    # Workflow Systems
    Source/Workflow/KeyboardShortcuts.h
    Source/Workflow/KeyboardShortcuts.cpp
    Source/Workflow/MacroSystem.h
    Source/Workflow/MacroSystem.cpp
    Source/Workflow/PresetBrowser.h
    Source/Workflow/PresetBrowser.cpp
    Source/Workflow/StemExporter.h
    Source/Workflow/StemExporter.cpp
    Source/Workflow/TrackFreezing.h
    Source/Workflow/TrackFreezing.cpp
    
    # Routing Matrix
    Source/Mixer/RoutingMatrix.h
    Source/Mixer/RoutingMatrix.cpp
    
    # Advanced AI Features
    Source/Audio/AI/StemSeparation.h
    Source/Audio/AI/StemSeparation.cpp
    Source/Audio/AI/AIVocalTuning.h
    Source/Audio/AI/AIVocalTuning.cpp
    
    # Content Management
    Source/Project/ContentManager.h
    Source/Project/ContentManager.cpp
    # Source/GUI/SampleBrowser.h  # Temporalmente deshabilitado
    # Source/GUI/SampleBrowser.cpp
    
    # NEW FL Studio-style Features
    Source/Sequencer/PlaylistEngine.h
    Source/Sequencer/PlaylistEngine.cpp
    Source/Sequencer/ChannelRack.h
    Source/Sequencer/ChannelRack.cpp
    Source/Sequencer/PianoRollAdvanced.h
    
    # NEW Advanced Instruments
    Source/Audio/Instruments/WavetableSynth.h
    Source/Audio/Instruments/FMSynth.h
    
    # NEW Creative Effects
    Source/Audio/DSP/CreativeEffects/ModulationEffects.h
    
    # NEW Unified Browser
    Source/Workflow/UnifiedBrowser.h
    
    # NEW Export & Performance
    Source/Workflow/ExportEngine.h
    
    # ========== FL STUDIO KILLER - GOD TIER IMPLEMENTATION ==========
    
    # Professional Mixer & DSP
    Source/Audio/DSP/ParametricEQ.h
    Source/Audio/DSP/ParametricEQ.cpp
    Source/Audio/DSP/MultibandCompressor.h
    Source/Audio/DSP/MultibandCompressor.cpp
    Source/Audio/DSP/LimiterMaximizer.h
    Source/Audio/DSP/LimiterMaximizer.cpp
    
    # Spectrum Analysis
    Source/Audio/Analysis/SpectrumAnalyzer.h
    Source/Audio/Analysis/SpectrumAnalyzer.cpp
    
    # Automation System
    Source/Workflow/AutomationClip.h
    Source/Workflow/AutomationClip.cpp
    
    # Advanced Drum Programming
    Source/Sequencer/DrumProgramming.h
    
    # Modulation & Synthesis
    Source/Audio/Synthesis/ModulationMatrix.h
    Source/Audio/Synthesis/ModulationMatrix.cpp
    
    # Workflow & UX Systems
    Source/Workflow/UndoRedoSystem.h
    
    # Visual Design Systems
    Source/GUI/VisualMeteringSystem.h
    Source/GUI/AdvancedComponents.h
    Source/GUI/AdvancedComponents.cpp
    
    # Project Management
    Source/Project/ProjectManagement.h
    
    # Advanced Plugin System
    Source/Audio/PluginSystemAdvanced.h
    
    # Composition Tools (100+ chord progressions, notation, etc)
    Source/Composition/CompositionTools.h
    Source/Composition/ChordProgressionSystem.cpp
    
    # Vocal Production Suite
    Source/Audio/VocalProductionSuite.h
    Source/Audio/VocalProductionSuite.cpp
    
    # Performance & Optimization
    Source/Performance/PerformanceSystem.h
    
    # ========== NEW: 4 PHASES FL STUDIO KILLER ==========
    
    # FASE 1: Professional Synthesizers
    Source/Audio/Synthesis/WavetableSynth.h
    Source/Audio/Synthesis/WavetableSynth.cpp
    Source/Audio/Synthesis/FMSynth.h
    Source/Audio/Synthesis/FMSynth.cpp
    Source/Audio/Synthesis/VirtualAnalogSynth.h
    Source/Audio/Synthesis/VirtualAnalogSynth.cpp
    Source/Audio/Synthesis/AdvancedSampler.h
    Source/Audio/Synthesis/AdvancedSampler.cpp
    
    # FASE 2: Workflow Visual
    Source/Content/SmartBrowser.h
    Source/Content/SmartBrowser.cpp
    
    # FASE 3: Creative Effects
    Source/Audio/Effects/CreativeEffects.h
    Source/Audio/Effects/CreativeEffects.cpp
    
    # FASE 4: Performance Manager
    Source/Performance/PerformanceManager.h
    Source/Performance/PerformanceManager.cpp
)

# ============================================================================
# JUCE MODULES
# ============================================================================
target_compile_definitions(OmegaStudio PRIVATE
    # JUCE Config
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:OmegaStudio,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:OmegaStudio,JUCE_VERSION>"
    
    # Audio optimizations
    JUCE_ALLOW_STATIC_NULL_VARIABLES=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    
    # Disable JUCE analytics
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    
    # VST3 SDK path (update this to your VST3 SDK location)
    # JUCE_VST3_PATH="/path/to/VST_SDK/VST3_SDK"
)

target_link_libraries(OmegaStudio PRIVATE
    # JUCE Core modules
    juce::juce_core
    juce::juce_events
    juce::juce_data_structures
    
    # JUCE Audio modules
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    
    # JUCE GUI modules
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    
    # JUCE DSP module (SIMD optimizations)
    juce::juce_dsp
    
    # Recommended compile flags
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Add Source directory to include path for JuceHeader.h
target_include_directories(OmegaStudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# ============================================================================
# PLATFORM-SPECIFIC CONFIGURATIONS
# ============================================================================
if(APPLE)
    # macOS specific settings
    set_target_properties(OmegaStudio PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.audiotech.omegastudio"
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    )
    
    # Required frameworks for macOS
    target_link_libraries(OmegaStudio PRIVATE
        "-framework Accelerate"    # Apple's optimized math library
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework AudioToolbox"
    )
    
    # Codesigning for Apple Silicon
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set_target_properties(OmegaStudio PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        )
    endif()
    
elseif(WIN32)
    # Windows specific settings
    target_link_libraries(OmegaStudio PRIVATE
        winmm      # Windows Multimedia API
        ole32      # COM
        ws2_32     # Winsock
    )
    
    # Enable Windows 11 features
    target_compile_definitions(OmegaStudio PRIVATE
        _WIN32_WINNT=0x0A00    # Windows 10/11
        WINVER=0x0A00
    )
endif()

# ============================================================================
# INSTALL CONFIGURATION
# ============================================================================
install(TARGETS OmegaStudio
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# ============================================================================
# CUSTOM TARGETS
# ============================================================================
# Print build configuration
message(STATUS "========================================")
message(STATUS "OmegaStudio Configuration")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "C++ Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Processor: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "========================================")
