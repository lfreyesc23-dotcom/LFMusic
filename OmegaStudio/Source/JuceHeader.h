#pragma once

/**
 * JuceHeader.h - Unified JUCE Header
 * 
 * This header includes all necessary JUCE modules for OmegaStudio.
 * Modern JUCE 8+ recommends including specific modules directly, but
 * this wrapper provides compatibility with legacy code structure.
 */

// Core JUCE modules
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_data_structures/juce_data_structures.h>

// Audio modules
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

// DSP module
#include <juce_dsp/juce_dsp.h>

// GUI modules
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

// Bring JUCE namespace into scope
using namespace juce;
