#include "SmartBrowser.h"
#include <algorithm>
#include <cmath>

namespace OmegaStudio {

//==============================================================================
SmartBrowser::SmartBrowser() {}
SmartBrowser::~SmartBrowser() {}

//==============================================================================
void SmartBrowser::scanDirectory(const juce::File& directory, bool recursive) {
    if (!directory.exists() || !directory.isDirectory()) {
        return;
    }
    
    juce::Array<juce::File> files;
    int flags = juce::File::findFiles;
    if (recursive) {
        directory.findChildFiles(files, flags, true);
    } else {
        directory.findChildFiles(files, flags, false);
    }
    
    for (auto& file : files) {
        ContentItem item;
        item.name = file.getFileNameWithoutExtension();
        item.path = file.getFullPathName();
        
        // Determine type
        juce::String ext = file.getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aiff" || ext == ".mp3" || ext == ".flac") {
            item.type = ContentType::Sample;
        } else if (ext == ".preset" || ext == ".fxp") {
            item.type = ContentType::Preset;
        } else if (ext == ".vst" || ext == ".vst3" || ext == ".au") {
            item.type = ContentType::Plugin;
        } else if (ext == ".mid" || ext == ".midi") {
            item.type = ContentType::MIDI;
        } else if (ext == ".omg" || ext == ".flp") {
            item.type = ContentType::Project;
        } else {
            continue; // Skip unknown types
        }
        
        // Auto-generate tags
        item.tags = autoGenerateTags(item);
        
        // Extract category from path
        juce::String relativePath = file.getRelativePathFrom(directory);
        int firstSlash = relativePath.indexOfChar('/');
        if (firstSlash > 0) {
            item.category = relativePath.substring(0, firstSlash);
        }
        
        addItem(item);
    }
}

void SmartBrowser::addItem(const ContentItem& item) {
    // Check if already exists
    for (auto& existing : database) {
        if (existing.path == item.path) {
            existing = item; // Update
            return;
        }
    }
    database.push_back(item);
}

void SmartBrowser::removeItem(const juce::String& path) {
    database.erase(std::remove_if(database.begin(), database.end(),
        [&path](const ContentItem& item) { return item.path == path; }),
        database.end());
}

void SmartBrowser::clearDatabase() {
    database.clear();
}

//==============================================================================
std::vector<SmartBrowser::ContentItem> SmartBrowser::search(const SearchQuery& query) {
    std::vector<ContentItem> results;
    
    for (auto& item : database) {
        // Type filter
        if (item.type != query.typeFilter) {
            continue;
        }
        
        // Text search (case-insensitive)
        if (!query.text.isEmpty()) {
            juce::String itemName = item.name.toLowerCase();
            juce::String searchText = query.text.toLowerCase();
            if (!itemName.contains(searchText)) {
                bool foundInTags = false;
                for (auto& tag : item.tags) {
                    if (tag.toLowerCase().contains(searchText)) {
                        foundInTags = true;
                        break;
                    }
                }
                if (!foundInTags) continue;
            }
        }
        
        // Category filter
        if (!query.categories.empty()) {
            bool matchCategory = false;
            for (auto& cat : query.categories) {
                if (item.category == cat) {
                    matchCategory = true;
                    break;
                }
            }
            if (!matchCategory) continue;
        }
        
        // Tag filter
        if (!query.tags.empty()) {
            bool matchTag = false;
            for (auto& tag : query.tags) {
                for (auto& itemTag : item.tags) {
                    if (itemTag == tag) {
                        matchTag = true;
                        break;
                    }
                }
                if (matchTag) break;
            }
            if (!matchTag) continue;
        }
        
        // BPM filter
        if (item.bpm > 0.0f) {
            if (item.bpm < query.bpmMin || item.bpm > query.bpmMax) {
                continue;
            }
        }
        
        // Key filter
        if (!query.keyFilter.isEmpty() && !item.key.isEmpty()) {
            if (item.key != query.keyFilter) {
                continue;
            }
        }
        
        // Rating filter
        if (item.rating < query.minRating) {
            continue;
        }
        
        // Favorites filter
        if (query.favoritesOnly && !item.isFavorite) {
            continue;
        }
        
        results.push_back(item);
    }
    
    // Sort results
    std::sort(results.begin(), results.end(), [&query](const ContentItem& a, const ContentItem& b) {
        switch (query.sortBy) {
            case SearchQuery::Name:
                return query.ascending ? a.name < b.name : a.name > b.name;
            case SearchQuery::BPM:
                return query.ascending ? a.bpm < b.bpm : a.bpm > b.bpm;
            case SearchQuery::Rating:
                return query.ascending ? a.rating < b.rating : a.rating > b.rating;
            case SearchQuery::TimesUsed:
                return query.ascending ? a.timesUsed < b.timesUsed : a.timesUsed > b.timesUsed;
            case SearchQuery::Date:
                return query.ascending ? a.lastUsed < b.lastUsed : a.lastUsed > b.lastUsed;
            default:
                return false;
        }
    });
    
    return results;
}

std::vector<SmartBrowser::ContentItem> SmartBrowser::fuzzySearch(const juce::String& query, float threshold) {
    std::vector<std::pair<ContentItem, float>> scoredResults;
    
    juce::String lowerQuery = query.toLowerCase();
    
    for (auto& item : database) {
        juce::String lowerName = item.name.toLowerCase();
        float similarity = calculateSimilarity(lowerQuery, lowerName);
        
        // Also check tags
        float maxTagSimilarity = 0.0f;
        for (auto& tag : item.tags) {
            float tagSim = calculateSimilarity(lowerQuery, tag.toLowerCase());
            maxTagSimilarity = std::max(maxTagSimilarity, tagSim);
        }
        
        float finalScore = std::max(similarity, maxTagSimilarity);
        
        if (finalScore >= threshold) {
            scoredResults.push_back({item, finalScore});
        }
    }
    
    // Sort by score (descending)
    std::sort(scoredResults.begin(), scoredResults.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<ContentItem> results;
    for (auto& pair : scoredResults) {
        results.push_back(pair.first);
    }
    
    return results;
}

//==============================================================================
void SmartBrowser::addToFavorites(const juce::String& path) {
    for (auto& item : database) {
        if (item.path == path) {
            item.isFavorite = true;
            break;
        }
    }
}

void SmartBrowser::removeFromFavorites(const juce::String& path) {
    for (auto& item : database) {
        if (item.path == path) {
            item.isFavorite = false;
            break;
        }
    }
}

std::vector<SmartBrowser::ContentItem> SmartBrowser::getFavorites() {
    std::vector<ContentItem> favorites;
    for (auto& item : database) {
        if (item.isFavorite) {
            favorites.push_back(item);
        }
    }
    return favorites;
}

//==============================================================================
std::vector<juce::String> SmartBrowser::getAllCategories() {
    std::vector<juce::String> categories;
    for (auto& item : database) {
        if (!item.category.isEmpty()) {
            bool found = false;
            for (auto& cat : categories) {
                if (cat == item.category) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                categories.push_back(item.category);
            }
        }
    }
    std::sort(categories.begin(), categories.end());
    return categories;
}

std::vector<juce::String> SmartBrowser::getAllTags() {
    std::vector<juce::String> allTags;
    for (auto& item : database) {
        for (auto& tag : item.tags) {
            bool found = false;
            for (auto& t : allTags) {
                if (t == tag) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                allTags.push_back(tag);
            }
        }
    }
    std::sort(allTags.begin(), allTags.end());
    return allTags;
}

void SmartBrowser::addTag(const juce::String& path, const juce::String& tag) {
    for (auto& item : database) {
        if (item.path == path) {
            item.tags.push_back(tag);
            break;
        }
    }
}

//==============================================================================
void SmartBrowser::saveDatabase(const juce::File& file) {
    juce::var jsonData = juce::var(juce::Array<juce::var>());
    
    for (auto& item : database) {
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("name", item.name);
        obj->setProperty("path", item.path);
        obj->setProperty("type", (int)item.type);
        obj->setProperty("category", item.category);
        obj->setProperty("bpm", item.bpm);
        obj->setProperty("key", item.key);
        obj->setProperty("rating", item.rating);
        obj->setProperty("isFavorite", item.isFavorite);
        obj->setProperty("timesUsed", item.timesUsed);
        
        juce::Array<juce::var> tagsArray;
        for (auto& tag : item.tags) {
            tagsArray.add(tag);
        }
        obj->setProperty("tags", tagsArray);
        
        jsonData.getArray()->add(juce::var(obj.get()));
    }
    
    juce::String jsonString = juce::JSON::toString(jsonData, true);
    file.replaceWithText(jsonString);
}

void SmartBrowser::loadDatabase(const juce::File& file) {
    if (!file.existsAsFile()) return;
    
    juce::String jsonString = file.loadFileAsString();
    juce::var jsonData = juce::JSON::parse(jsonString);
    
    if (!jsonData.isArray()) return;
    
    database.clear();
    
    for (auto& itemVar : *jsonData.getArray()) {
        if (auto* obj = itemVar.getDynamicObject()) {
            ContentItem item;
            item.name = obj->getProperty("name").toString();
            item.path = obj->getProperty("path").toString();
            item.type = (ContentType)(int)obj->getProperty("type");
            item.category = obj->getProperty("category").toString();
            item.bpm = (float)obj->getProperty("bpm");
            item.key = obj->getProperty("key").toString();
            item.rating = (int)obj->getProperty("rating");
            item.isFavorite = (bool)obj->getProperty("isFavorite");
            item.timesUsed = (int)obj->getProperty("timesUsed");
            
            if (auto* tagsArray = obj->getProperty("tags").getArray()) {
                for (auto& tagVar : *tagsArray) {
                    item.tags.push_back(tagVar.toString());
                }
            }
            
            database.push_back(item);
        }
    }
}

//==============================================================================
float SmartBrowser::calculateSimilarity(const juce::String& a, const juce::String& b) {
    // Levenshtein distance algorithm
    int len1 = a.length();
    int len2 = b.length();
    
    if (len1 == 0) return len2 == 0 ? 1.0f : 0.0f;
    if (len2 == 0) return 0.0f;
    
    // Quick exact match check
    if (a == b) return 1.0f;
    
    // Contains check (high score if one contains the other)
    if (b.contains(a)) return 0.9f;
    if (a.contains(b)) return 0.9f;
    
    // Create distance matrix
    std::vector<std::vector<int>> matrix(len1 + 1, std::vector<int>(len2 + 1));
    
    for (int i = 0; i <= len1; ++i) matrix[i][0] = i;
    for (int j = 0; j <= len2; ++j) matrix[0][j] = j;
    
    for (int i = 1; i <= len1; ++i) {
        for (int j = 1; j <= len2; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            matrix[i][j] = std::min({
                matrix[i - 1][j] + 1,      // deletion
                matrix[i][j - 1] + 1,      // insertion
                matrix[i - 1][j - 1] + cost // substitution
            });
        }
    }
    
    int distance = matrix[len1][len2];
    int maxLen = std::max(len1, len2);
    
    // Convert to similarity (0-1)
    return 1.0f - ((float)distance / (float)maxLen);
}

std::vector<juce::String> SmartBrowser::autoGenerateTags(const ContentItem& item) {
    std::vector<juce::String> tags;
    
    juce::String lowerName = item.name.toLowerCase();
    
    // Genre detection
    if (lowerName.contains("techno")) tags.push_back("Techno");
    if (lowerName.contains("house")) tags.push_back("House");
    if (lowerName.contains("trap")) tags.push_back("Trap");
    if (lowerName.contains("dubstep")) tags.push_back("Dubstep");
    if (lowerName.contains("ambient")) tags.push_back("Ambient");
    if (lowerName.contains("dnb") || lowerName.contains("drum")) tags.push_back("DnB");
    
    // Instrument detection
    if (lowerName.contains("kick")) tags.push_back("Kick");
    if (lowerName.contains("snare")) tags.push_back("Snare");
    if (lowerName.contains("hi") && lowerName.contains("hat")) tags.push_back("Hi-Hat");
    if (lowerName.contains("bass")) tags.push_back("Bass");
    if (lowerName.contains("lead")) tags.push_back("Lead");
    if (lowerName.contains("pad")) tags.push_back("Pad");
    if (lowerName.contains("pluck")) tags.push_back("Pluck");
    if (lowerName.contains("fx") || lowerName.contains("effect")) tags.push_back("FX");
    
    // Mood detection
    if (lowerName.contains("dark")) tags.push_back("Dark");
    if (lowerName.contains("bright")) tags.push_back("Bright");
    if (lowerName.contains("warm")) tags.push_back("Warm");
    if (lowerName.contains("cold")) tags.push_back("Cold");
    if (lowerName.contains("aggressive")) tags.push_back("Aggressive");
    if (lowerName.contains("soft")) tags.push_back("Soft");
    
    return tags;
}

} // namespace OmegaStudio
