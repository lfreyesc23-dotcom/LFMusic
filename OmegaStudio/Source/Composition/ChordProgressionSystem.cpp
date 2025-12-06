//==============================================================================
// ChordProgressionSystem.cpp - 100+ Professional Chord Progressions
// FL Studio Killer - Professional DAW
//==============================================================================

#include "../Composition/CompositionTools.h"

namespace OmegaStudio {

//==============================================================================
ChordProgressionSystem::ChordProgressionSystem()
{
    initializeProgressions();
}

//==============================================================================
void ChordProgressionSystem::initializeProgressions()
{
    // ========== POP ==========
    addProgression("I-V-vi-IV (Classic Pop)", "Pop", {1, 5, 6, 4}, 4);
    addProgression("vi-IV-I-V (Sensitive)", "Pop", {6, 4, 1, 5}, 4);
    addProgression("I-vi-IV-V (50s Progression)", "Pop", {1, 6, 4, 5}, 4);
    addProgression("I-IV-vi-V (Pop Ballad)", "Pop", {1, 4, 6, 5}, 4);
    addProgression("I-V-vi-iii-IV-I-IV-V (Extended Pop)", "Pop", {1, 5, 6, 3, 4, 1, 4, 5}, 2);
    addProgression("vi-V-IV-V (Ascending Pop)", "Pop", {6, 5, 4, 5}, 4);
    addProgression("I-iii-IV-V (Happy Pop)", "Pop", {1, 3, 4, 5}, 4);
    addProgression("I-bVII-IV (Modern Pop)", "Pop", {1, 7, 4}, 4);
    addProgression("vi-I-V-IV (Alternative Pop)", "Pop", {6, 1, 5, 4}, 4);
    addProgression("I-vi-ii-V (Circle Pop)", "Pop", {1, 6, 2, 5}, 4);
    
    // ========== ROCK ==========
    addProgression("I-bVII-IV (Rock Anthem)", "Rock", {1, 7, 4}, 4);
    addProgression("I-IV-V (Basic Rock)", "Rock", {1, 4, 5}, 4);
    addProgression("i-bVII-bVI-bVII (Minor Rock)", "Rock", {1, 7, 6, 7}, 4);
    addProgression("I-V-IV (Grunge)", "Rock", {1, 5, 4}, 4);
    addProgression("i-bVI-bIII-bVII (Aeolian Rock)", "Rock", {1, 6, 3, 7}, 4);
    addProgression("I-bIII-IV (Power Chord)", "Rock", {1, 3, 4}, 4);
    addProgression("i-iv-v (Punk Rock)", "Rock", {1, 4, 5}, 2);
    addProgression("I-II-IV (Lydian Rock)", "Rock", {1, 2, 4}, 4);
    addProgression("i-bVI-iv-v (Dark Rock)", "Rock", {1, 6, 4, 5}, 4);
    addProgression("I-IV-bVII-IV (Classic Rock)", "Rock", {1, 4, 7, 4}, 4);
    
    // ========== R&B / SOUL ==========
    addProgression("ii-V-I (Jazz Influenced)", "R&B", {2, 5, 1}, 4);
    addProgression("I-iii-vi-ii (Smooth R&B)", "R&B", {1, 3, 6, 2}, 4);
    addProgression("vi-ii-V-I (Circle of Fifths)", "R&B", {6, 2, 5, 1}, 4);
    addProgression("I-vi-ii-iii-IV (Extended Soul)", "R&B", {1, 6, 2, 3, 4}, 4);
    addProgression("I-IV-ii-V (Gospel R&B)", "R&B", {1, 4, 2, 5}, 4);
    addProgression("I-bIII-bVII-IV (Soul Ballad)", "R&B", {1, 3, 7, 4}, 4);
    addProgression("ii-iii-IV-V (Ascending R&B)", "R&B", {2, 3, 4, 5}, 4);
    addProgression("I-V-vi-ii-IV-I (Full Circle)", "R&B", {1, 5, 6, 2, 4, 1}, 4);
    addProgression("iv-I-V-vi (Minor Soul)", "R&B", {4, 1, 5, 6}, 4);
    addProgression("I-IV-iii-vi (Neo-Soul)", "R&B", {1, 4, 3, 6}, 4);
    
    // ========== HIP HOP / TRAP ==========
    addProgression("i-bVI-bIII-bVII (Trap Dark)", "Hip Hop", {1, 6, 3, 7}, 8);
    addProgression("i-iv-v (Simple Trap)", "Hip Hop", {1, 4, 5}, 8);
    addProgression("i-bVII-bVI (Sad Trap)", "Hip Hop", {1, 7, 6}, 8);
    addProgression("i-v-bVI-bIII (Modern Hip Hop)", "Hip Hop", {1, 5, 6, 3}, 8);
    addProgression("i (Drone Hip Hop)", "Hip Hop", {1}, 16);
    addProgression("i-bVII (Minimal Trap)", "Hip Hop", {1, 7}, 8);
    addProgression("i-iv-bVII-bVI (Cloud Rap)", "Hip Hop", {1, 4, 7, 6}, 8);
    addProgression("i-v-i-bVI (Boom Bap)", "Hip Hop", {1, 5, 1, 6}, 4);
    addProgression("i-bVI-v-bVII (Melodic Trap)", "Hip Hop", {1, 6, 5, 7}, 8);
    addProgression("i-bIII-bVII-iv (Dark Hip Hop)", "Hip Hop", {1, 3, 7, 4}, 8);
    
    // ========== EDM / ELECTRONIC ==========
    addProgression("I-V-vi-IV (EDM Drop)", "Electronic", {1, 5, 6, 4}, 4);
    addProgression("i-bVI-bIII-bVII (Progressive House)", "Electronic", {1, 6, 3, 7}, 4);
    addProgression("I-vi-V-IV (Trance)", "Electronic", {1, 6, 5, 4}, 4);
    addProgression("i-v-bVI-bIII (Future Bass)", "Electronic", {1, 5, 6, 3}, 4);
    addProgression("I-bVII-vi-bVII (Big Room)", "Electronic", {1, 7, 6, 7}, 4);
    addProgression("i-iv-v-i (Techno)", "Electronic", {1, 4, 5, 1}, 2);
    addProgression("I-IV-I-V (House Classic)", "Electronic", {1, 4, 1, 5}, 4);
    addProgression("i-bVII-bVI-V (Melodic Dubstep)", "Electronic", {1, 7, 6, 5}, 4);
    addProgression("vi-V-IV-iii (Chill EDM)", "Electronic", {6, 5, 4, 3}, 4);
    addProgression("I-V-vi-iii (Euphoric Trance)", "Electronic", {1, 5, 6, 3}, 4);
    
    // ========== JAZZ ==========
    addProgression("ii-V-I-vi (Jazz Standard)", "Jazz", {2, 5, 1, 6}, 4);
    addProgression("I-vi-ii-V (Rhythm Changes)", "Jazz", {1, 6, 2, 5}, 4);
    addProgression("iii-vi-ii-V-I (Extended Jazz)", "Jazz", {3, 6, 2, 5, 1}, 4);
    addProgression("I-IV-iii-vi-ii-V-I (Jazz Circle)", "Jazz", {1, 4, 3, 6, 2, 5, 1}, 2);
    addProgression("ii-V-iii-vi (Turnaround)", "Jazz", {2, 5, 3, 6}, 4);
    addProgression("I-bIII-ii-V (Tritone Sub)", "Jazz", {1, 3, 2, 5}, 4);
    addProgression("I-vi-IV-ii-V (Jazz Ballad)", "Jazz", {1, 6, 4, 2, 5}, 4);
    addProgression("ii-V-I-IV (Modal Jazz)", "Jazz", {2, 5, 1, 4}, 4);
    addProgression("iii-VI-ii-V (Minor ii-V)", "Jazz", {3, 6, 2, 5}, 4);
    addProgression("I-bII-I (Jazz Chromaticism)", "Jazz", {1, 2, 1}, 4);
    
    // ========== COUNTRY ==========
    addProgression("I-IV-V-IV (Country Classic)", "Country", {1, 4, 5, 4}, 4);
    addProgression("I-V-vi-IV (Country Pop)", "Country", {1, 5, 6, 4}, 4);
    addProgression("I-IV-I-V (Traditional Country)", "Country", {1, 4, 1, 5}, 4);
    addProgression("I-vi-IV-V (Country Ballad)", "Country", {1, 6, 4, 5}, 4);
    addProgression("I-iii-IV-V (Happy Country)", "Country", {1, 3, 4, 5}, 4);
    addProgression("I-IV-ii-V (Nashville)", "Country", {1, 4, 2, 5}, 4);
    addProgression("I-V-IV-I (Bluegrass)", "Country", {1, 5, 4, 1}, 2);
    addProgression("I-bVII-IV-I (Modern Country)", "Country", {1, 7, 4, 1}, 4);
    
    // ========== METAL ==========
    addProgression("i-bVI-bVII (Doom Metal)", "Metal", {1, 6, 7}, 4);
    addProgression("i-v-bVI-iv (Death Metal)", "Metal", {1, 5, 6, 4}, 4);
    addProgression("i-bVII-bVI-V (Melodic Metal)", "Metal", {1, 7, 6, 5}, 4);
    addProgression("i-iv-i-v (Thrash Metal)", "Metal", {1, 4, 1, 5}, 2);
    addProgression("i-bII-bVII-i (Phrygian Metal)", "Metal", {1, 2, 7, 1}, 4);
    addProgression("i-bVI-v-bVII (Black Metal)", "Metal", {1, 6, 5, 7}, 4);
    addProgression("i-bIII-bVII-bVI (Power Metal)", "Metal", {1, 3, 7, 6}, 4);
    addProgression("i-iv-bVII (Progressive Metal)", "Metal", {1, 4, 7}, 4);
    
    // ========== INDIE / ALTERNATIVE ==========
    addProgression("vi-IV-I-V (Indie Classic)", "Indie", {6, 4, 1, 5}, 4);
    addProgression("I-iii-vi-IV (Indie Pop)", "Indie", {1, 3, 6, 4}, 4);
    addProgression("vi-V-IV-I (Indie Rock)", "Indie", {6, 5, 4, 1}, 4);
    addProgression("I-vi-iii-IV (Dream Pop)", "Indie", {1, 6, 3, 4}, 4);
    addProgression("ii-IV-I-V (Indie Folk)", "Indie", {2, 4, 1, 5}, 4);
    addProgression("I-V-vi-bVII (Alt Indie)", "Indie", {1, 5, 6, 7}, 4);
    addProgression("vi-iii-IV-I (Lo-Fi Indie)", "Indie", {6, 3, 4, 1}, 4);
    
    // ========== GOSPEL ==========
    addProgression("I-IV-V-IV-I (Traditional Gospel)", "Gospel", {1, 4, 5, 4, 1}, 4);
    addProgression("I-vi-ii-V-I (Gospel Turnaround)", "Gospel", {1, 6, 2, 5, 1}, 4);
    addProgression("IV-I-V-vi (Modern Gospel)", "Gospel", {4, 1, 5, 6}, 4);
    addProgression("I-iii-IV-V-vi (Extended Gospel)", "Gospel", {1, 3, 4, 5, 6}, 4);
    addProgression("ii-V-I-IV (Gospel Praise)", "Gospel", {2, 5, 1, 4}, 4);
    
    // ========== LATIN ==========
    addProgression("i-bVII-bVI-V (Flamenco)", "Latin", {1, 7, 6, 5}, 4);
    addProgression("i-iv-v (Salsa)", "Latin", {1, 4, 5}, 4);
    addProgression("I-bVII-IV (Reggaeton)", "Latin", {1, 7, 4}, 4);
    addProgression("i-bVI-bVII-i (Tango)", "Latin", {1, 6, 7, 1}, 4);
    addProgression("I-IV-V-IV (Bossa Nova)", "Latin", {1, 4, 5, 4}, 4);
    
    // ========== BLUES ==========
    addProgression("I-I-I-I-IV-IV-I-I-V-IV-I-V (12-Bar Blues)", "Blues", 
                  {1, 1, 1, 1, 4, 4, 1, 1, 5, 4, 1, 5}, 4);
    addProgression("I-IV-I-V (Quick Blues)", "Blues", {1, 4, 1, 5}, 4);
    addProgression("i-iv-i-v (Minor Blues)", "Blues", {1, 4, 1, 5}, 4);
    
    // ========== AMBIENT / CINEMATIC ==========
    addProgression("vi-IV-I-V (Emotional)", "Cinematic", {6, 4, 1, 5}, 8);
    addProgression("i-bVII-bVI-bVII (Dark Cinematic)", "Cinematic", {1, 7, 6, 7}, 8);
    addProgression("I-V-vi-iii-IV-I (Epic)", "Cinematic", {1, 5, 6, 3, 4, 1}, 4);
    addProgression("ii-IV-I (Ethereal)", "Cinematic", {2, 4, 1}, 8);
    addProgression("i-iv-bVII (Suspense)", "Cinematic", {1, 4, 7}, 8);
}

void ChordProgressionSystem::addProgression(const juce::String& name, 
                                           const juce::String& genre,
                                           const std::vector<int>& degrees,
                                           int beatsPerChord)
{
    Progression prog;
    prog.name = name;
    prog.genre = genre;
    prog.degrees = degrees;
    prog.beatsPerChord = beatsPerChord;
    
    progressions_.push_back(prog);
}

//==============================================================================
const ChordProgressionSystem::Progression& ChordProgressionSystem::getProgression(int index) const
{
    jassert(index >= 0 && index < progressions_.size());
    return progressions_[index];
}

std::vector<ChordProgressionSystem::Progression> 
ChordProgressionSystem::getProgressionsByGenre(const juce::String& genre) const
{
    std::vector<Progression> result;
    
    for (const auto& prog : progressions_)
    {
        if (prog.genre.equalsIgnoreCase(genre))
            result.push_back(prog);
    }
    
    return result;
}

juce::StringArray ChordProgressionSystem::getAvailableGenres() const
{
    juce::StringArray genres;
    
    for (const auto& prog : progressions_)
    {
        if (!genres.contains(prog.genre))
            genres.add(prog.genre);
    }
    
    return genres;
}

//==============================================================================
ChordProgressionSystem::Chord ChordProgressionSystem::createChord(
    int root, ChordQuality quality) const
{
    Chord chord;
    chord.root = root % 12;
    chord.quality = quality;
    
    // Build chord notes
    chord.notes.push_back(0);  // Root
    
    switch (quality)
    {
        case ChordQuality::Major:
            chord.notes.push_back(4);   // Major 3rd
            chord.notes.push_back(7);   // Perfect 5th
            chord.symbol = "maj";
            break;
            
        case ChordQuality::Minor:
            chord.notes.push_back(3);   // Minor 3rd
            chord.notes.push_back(7);   // Perfect 5th
            chord.symbol = "m";
            break;
            
        case ChordQuality::Diminished:
            chord.notes.push_back(3);   // Minor 3rd
            chord.notes.push_back(6);   // Diminished 5th
            chord.symbol = "dim";
            break;
            
        case ChordQuality::Augmented:
            chord.notes.push_back(4);   // Major 3rd
            chord.notes.push_back(8);   // Augmented 5th
            chord.symbol = "aug";
            break;
            
        case ChordQuality::Major7:
            chord.notes.push_back(4);   // Major 3rd
            chord.notes.push_back(7);   // Perfect 5th
            chord.notes.push_back(11);  // Major 7th
            chord.symbol = "maj7";
            break;
            
        case ChordQuality::Minor7:
            chord.notes.push_back(3);   // Minor 3rd
            chord.notes.push_back(7);   // Perfect 5th
            chord.notes.push_back(10);  // Minor 7th
            chord.symbol = "m7";
            break;
            
        case ChordQuality::Dominant7:
            chord.notes.push_back(4);   // Major 3rd
            chord.notes.push_back(7);   // Perfect 5th
            chord.notes.push_back(10);  // Minor 7th
            chord.symbol = "7";
            break;
            
        // Add more chord types as needed...
        default:
            break;
    }
    
    return chord;
}

std::vector<ChordProgressionSystem::Progression> 
ChordProgressionSystem::getPopularProgressions() const
{
    std::vector<Progression> popular;
    
    // Top 10 most used progressions
    if (progressions_.size() > 0) popular.push_back(progressions_[0]);   // I-V-vi-IV
    if (progressions_.size() > 1) popular.push_back(progressions_[1]);   // vi-IV-I-V
    if (progressions_.size() > 2) popular.push_back(progressions_[2]);   // I-vi-IV-V
    if (progressions_.size() > 10) popular.push_back(progressions_[10]); // I-bVII-IV
    if (progressions_.size() > 20) popular.push_back(progressions_[20]); // i-bVI-bIII-bVII
    if (progressions_.size() > 30) popular.push_back(progressions_[30]); // I-V-vi-IV (EDM)
    if (progressions_.size() > 40) popular.push_back(progressions_[40]); // ii-V-I
    if (progressions_.size() > 50) popular.push_back(progressions_[50]); // I-IV-V
    if (progressions_.size() > 60) popular.push_back(progressions_[60]); // vi-IV-I-V (Indie)
    if (progressions_.size() > 70) popular.push_back(progressions_[70]); // I-IV-V-IV
    
    return popular;
}

} // namespace OmegaStudio
