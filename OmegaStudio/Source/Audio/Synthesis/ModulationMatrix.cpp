//==============================================================================
// ModulationMatrix.cpp - Simplified Modulation Matrix Implementation
// FL Studio Killer - Professional DAW
//==============================================================================

#include "ModulationMatrix.h"

namespace OmegaStudio {

//==============================================================================
// ModulationMatrix Implementation (Simplified - matches header exactly)
//==============================================================================

ModulationMatrix::ModulationMatrix()
{
    for (auto& source : sourceValues_)
        source = 0.0f;
}

void ModulationMatrix::prepare(double sampleRate)
{
    sampleRate_ = sampleRate;
}

void ModulationMatrix::reset()
{
    for (auto& source : sourceValues_)
        source = 0.0f;
}

int ModulationMatrix::addConnection(ModSource source, ModDestination dest, float amount)
{
    ModConnection conn;
    conn.source = source;
    conn.destination = dest;
    conn.amount = amount;
    conn.bipolar = false;
    conn.enabled = true;
    conn.curvature = 0.0f;
    
    connections_.push_back(conn);
    return static_cast<int>(connections_.size()) - 1;
}

void ModulationMatrix::removeConnection(int index)
{
    if (index >= 0 && index < static_cast<int>(connections_.size()))
        connections_.erase(connections_.begin() + index);
}

void ModulationMatrix::clearAllConnections()
{
    connections_.clear();
}

void ModulationMatrix::setConnectionAmount(int index, float amount)
{
    if (index >= 0 && index < static_cast<int>(connections_.size()))
        connections_[index].amount = juce::jlimit(0.0f, 1.0f, amount);
}

void ModulationMatrix::setConnectionEnabled(int index, bool enabled)
{
    if (index >= 0 && index < static_cast<int>(connections_.size()))
        connections_[index].enabled = enabled;
}

void ModulationMatrix::setConnectionBipolar(int index, bool bipolar)
{
    if (index >= 0 && index < static_cast<int>(connections_.size()))
        connections_[index].bipolar = bipolar;
}

void ModulationMatrix::setConnectionCurvature(int index, float curvature)
{
    if (index >= 0 && index < static_cast<int>(connections_.size()))
        connections_[index].curvature = juce::jlimit(-1.0f, 1.0f, curvature);
}

void ModulationMatrix::setSourceValue(ModSource source, float value)
{
    size_t idx = static_cast<size_t>(source);
    if (idx < sourceValues_.size())
        sourceValues_[idx] = value;
}

float ModulationMatrix::getSourceValue(ModSource source) const
{
    size_t idx = static_cast<size_t>(source);
    return idx < sourceValues_.size() ? sourceValues_[idx] : 0.0f;
}

float ModulationMatrix::getModulationFor(ModDestination dest) const
{
    float total = 0.0f;
    
    for (const auto& conn : connections_)
    {
        if (!conn.enabled || conn.destination != dest)
            continue;
        
        float sourceVal = getSourceValue(conn.source);
        
        if (conn.curvature != 0.0f)
        {
            if (conn.curvature > 0.0f)
                sourceVal = std::pow(sourceVal, 1.0f + conn.curvature * 2.0f);
            else
                sourceVal = 1.0f - std::pow(1.0f - sourceVal, 1.0f - conn.curvature * 2.0f);
        }
        
        if (conn.bipolar)
            sourceVal = sourceVal * 2.0f - 1.0f;
        
        total += sourceVal * conn.amount;
    }
    
    return total;
}

std::map<ModulationMatrix::ModDestination, float> ModulationMatrix::getAllModulationValues() const
{
    std::map<ModDestination, float> result;
    
    for (const auto& conn : connections_)
    {
        if (!conn.enabled)
            continue;
        
        float sourceVal = getSourceValue(conn.source);
        
        if (conn.curvature != 0.0f)
        {
            if (conn.curvature > 0.0f)
                sourceVal = std::pow(sourceVal, 1.0f + conn.curvature * 2.0f);
            else
                sourceVal = 1.0f - std::pow(1.0f - sourceVal, 1.0f - conn.curvature * 2.0f);
        }
        
        if (conn.bipolar)
            sourceVal = sourceVal * 2.0f - 1.0f;
        
        result[conn.destination] += sourceVal * conn.amount;
    }
    
    return result;
}

void ModulationMatrix::loadPreset(const juce::String& presetName)
{
    clearAllConnections();
    
    if (presetName == "Basic Filter Sweep")
    {
        addConnection(ModSource::LFO1, ModDestination::FilterCutoff, 0.5f);
    }
    else if (presetName == "Vibrato")
    {
        auto idx = addConnection(ModSource::LFO1, ModDestination::Osc1Pitch, 0.05f);
        setConnectionBipolar(idx, true);
    }
}

juce::StringArray ModulationMatrix::getPresetList() const
{
    return { "Basic Filter Sweep", "Vibrato", "Tremolo" };
}

juce::ValueTree ModulationMatrix::toValueTree() const
{
    juce::ValueTree tree("ModulationMatrix");
    
    for (const auto& conn : connections_)
    {
        juce::ValueTree connTree("Connection");
        connTree.setProperty("source", static_cast<int>(conn.source), nullptr);
        connTree.setProperty("destination", static_cast<int>(conn.destination), nullptr);
        connTree.setProperty("amount", conn.amount, nullptr);
        connTree.setProperty("bipolar", conn.bipolar, nullptr);
        connTree.setProperty("enabled", conn.enabled, nullptr);
        connTree.setProperty("curvature", conn.curvature, nullptr);
        tree.appendChild(connTree, nullptr);
    }
    
    return tree;
}

void ModulationMatrix::fromValueTree(const juce::ValueTree& tree)
{
    if (!tree.hasType("ModulationMatrix"))
        return;
    
    clearAllConnections();
    
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        juce::ValueTree connTree = tree.getChild(i);
        
        ModConnection conn;
        conn.source = static_cast<ModSource>(static_cast<int>(connTree.getProperty("source", 0)));
        conn.destination = static_cast<ModDestination>(static_cast<int>(connTree.getProperty("destination", 0)));
        conn.amount = connTree.getProperty("amount", 0.5f);
        conn.bipolar = connTree.getProperty("bipolar", false);
        conn.enabled = connTree.getProperty("enabled", true);
        conn.curvature = connTree.getProperty("curvature", 0.0f);
        
        connections_.push_back(conn);
    }
}

float ModulationMatrix::applyCurve(float value, float curvature) const
{
    if (curvature == 0.0f)
        return value;
    
    if (curvature > 0.0f)
        return std::pow(value, 1.0f + curvature * 2.0f);
    else
        return 1.0f - std::pow(1.0f - value, 1.0f - curvature * 2.0f);
}

} // namespace OmegaStudio
