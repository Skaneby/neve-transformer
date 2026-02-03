#pragma once

#include <juce_core/juce_core.h>

/**
 * Simple preset storage and management
 */
struct Preset {
  juce::String name;
  float drive = 0.3f;
  float iron = 0.5f;
  float hfRoll = 0.7f;
  bool micMode = false;
  bool hiZLoad = true;

  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("drive", drive);
    obj->setProperty("iron", iron);
    obj->setProperty("hfRoll", hfRoll);
    obj->setProperty("micMode", micMode);
    obj->setProperty("hiZLoad", hiZLoad);
    return juce::var(obj);
  }

  static Preset fromVar(const juce::var &v) {
    Preset p;
    if (auto *obj = v.getDynamicObject()) {
      p.name = obj->getProperty("name").toString();
      p.drive = static_cast<float>(obj->getProperty("drive"));
      p.iron = static_cast<float>(obj->getProperty("iron"));
      p.hfRoll = static_cast<float>(obj->getProperty("hfRoll"));
      p.micMode = obj->getProperty("micMode");
      p.hiZLoad = obj->getProperty("hiZLoad");
    }
    return p;
  }
};

class PresetManager {
public:
  PresetManager() {
    // Create default factory presets
    createFactoryPresets();
    loadPresetsFromFile();
  }

  void addPreset(const Preset &preset) {
    userPresets.add(preset);
    savePresetsToFile();
  }

  void deletePreset(int index) {
    if (index >= 0 && index < userPresets.size()) {
      userPresets.remove(index);
      savePresetsToFile();
    }
  }

  juce::StringArray getPresetNames() const {
    juce::StringArray names;
    
    // Add factory presets
    names.add("--- FACTORY ---");
    for (const auto &preset : factoryPresets)
      names.add(preset.name);
    
    // Add user presets if any
    if (userPresets.size() > 0) {
      names.add("--- USER ---");
      for (const auto &preset : userPresets)
        names.add(preset.name);
    }
    
    return names;
  }

  Preset getPreset(int index) const {
    // Adjust index for separator
    if (index <= 0 || index > factoryPresets.size() + userPresets.size() + 1)
      return Preset();
    
    // Factory presets (skip separator at index 0)
    if (index <= factoryPresets.size()) {
      return factoryPresets[index - 1];
    }
    
    // User presets (skip user separator)
    int userIndex = index - factoryPresets.size() - 2;
    if (userIndex >= 0 && userIndex < userPresets.size()) {
      return userPresets[userIndex];
    }
    
    return Preset();
  }

  int getNumUserPresets() const { return userPresets.size(); }

private:
  juce::Array<Preset> factoryPresets;
  juce::Array<Preset> userPresets;

  void createFactoryPresets() {
    // Clean/Transparent
    factoryPresets.add({"Clean", 0.0f, 0.0f, 0.0f, false, false});
    
    // Subtle warmth
    factoryPresets.add({"Subtle Warmth", 0.15f, 0.3f, 0.4f, false, true});
    
    // Classic Neve
    factoryPresets.add({"Classic Neve", 0.5f, 0.5f, 0.7f, false, true});
    
    // Heavy saturation
    factoryPresets.add({"Heavy Drive", 0.85f, 0.6f, 0.8f, false, true});
    
    // Mic preamp mode
    factoryPresets.add({"Mic Preamp", 0.3f, 0.7f, 0.5f, true, false});
    
    // Hi-Z guitar/bass
    factoryPresets.add({"Hi-Z Guitar", 0.4f, 0.4f, 0.6f, false, true});
  }

  juce::File getPresetFile() const {
    auto appData = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory);
    auto presetDir = appData.getChildFile("NeveTransformer");
    presetDir.createDirectory();
    return presetDir.getChildFile("presets.json");
  }

  void savePresetsToFile() {
    juce::Array<juce::var> arr;
    
    for (const auto &preset : userPresets)
      arr.add(preset.toVar());
    
    juce::var presetsArray(arr);
    
    auto file = getPresetFile();
    if (auto output = file.createOutputStream()) {
      output->writeText(juce::JSON::toString(presetsArray, true), false, false,
                        nullptr);
    }
  }

  void loadPresetsFromFile() {
    auto file = getPresetFile();
    if (file.existsAsFile()) {
      auto json = juce::JSON::parse(file);
      if (auto *arr = json.getArray()) {
        for (const auto &v : *arr) {
          userPresets.add(Preset::fromVar(v));
        }
      }
    }
  }
};
