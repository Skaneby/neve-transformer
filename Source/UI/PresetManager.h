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
  float mix = 1.0f;
  bool micMode = false;
  bool hiZLoad = true;

  juce::var toVar() const {
    auto *obj = new juce::DynamicObject();
    obj->setProperty("name", name);
    obj->setProperty("drive", drive);
    obj->setProperty("iron", iron);
    obj->setProperty("hfRoll", hfRoll);
    obj->setProperty("mix", mix);
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
      if (obj->hasProperty("mix"))
        p.mix = static_cast<float>(obj->getProperty("mix"));
      p.micMode = obj->getProperty("micMode");
      p.hiZLoad = obj->getProperty("hiZLoad");
    }
    return p;
  }
};

class PresetManager {
public:
  PresetManager() {
    createFactoryPresets();
    loadPresetsFromFile();
  }

  void savePresetAsText(const Preset &p) {
    auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto exportDir = docs.getChildFile("Neve Transformer").getChildFile("Presets_Text");
    exportDir.createDirectory();

    auto file = exportDir.getChildFile(p.name + ".txt");
    juce::String content;
    content << "=== Neve Transformer Preset: " << p.name << " ===\n";
    content << "Drive: " << juce::String(p.drive, 2) << "\n";
    content << "Iron: " << juce::String(p.iron, 2) << "\n";
    content << "HF Roll: " << juce::String(p.hfRoll, 2) << "\n";
    content << "Mix: " << juce::String(p.mix, 2) << "\n";
    content << "Mode: " << (p.micMode ? "MIC" : "LINE") << "\n";
    content << "Hi-Z Load: " << (p.hiZLoad ? "ON" : "OFF") << "\n";
    content << "Timestamp: " << juce::Time::getCurrentTime().toString(true, true) << "\n";

    file.replaceWithText(content);
  }

  void addPreset(const Preset &preset) {
    userPresets.add(preset);
    savePresetsToFile();
    savePresetAsText(preset);
  }

  void deletePreset(int index) {
    if (index >= 0 && index < userPresets.size()) {
      auto name = userPresets[index].name;
      userPresets.remove(index);
      savePresetsToFile();

      auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
      auto textFile = docs.getChildFile("Neve Transformer").getChildFile("Presets_Text").getChildFile(name + ".txt");
      if (textFile.existsAsFile())
          textFile.deleteFile();
    }
  }

  juce::StringArray getPresetNames() const {
    juce::StringArray names;

    names.add("--- FACTORY ---");
    for (const auto &preset : factoryPresets)
      names.add(preset.name);

    if (userPresets.size() > 0) {
      names.add("--- USER ---");
      for (const auto &preset : userPresets)
        names.add(preset.name);
    }

    return names;
  }

  Preset getPreset(int index) const {
    if (index <= 0 || index > factoryPresets.size() + userPresets.size() + 1)
      return Preset();

    if (index <= factoryPresets.size()) {
      return factoryPresets[index - 1];
    }

    int userIndex = index - factoryPresets.size() - 2;
    if (userIndex >= 0 && userIndex < userPresets.size()) {
      return userPresets[userIndex];
    }

    return Preset();
  }

  int getNumUserPresets() const { return userPresets.size(); }

  juce::File getPresetsFolder() const {
      auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
      auto dir = docs.getChildFile("Neve Transformer").getChildFile("Presets_Text");
      dir.createDirectory();
      return dir;
  }

private:
  juce::Array<Preset> factoryPresets;
  juce::Array<Preset> userPresets;

  void createFactoryPresets() {
    factoryPresets.add({"Clean", 0.0f, 0.0f, 0.0f, 1.0f, false, false});
    factoryPresets.add({"Subtle Warmth", 0.15f, 0.3f, 0.4f, 1.0f, false, true});
    factoryPresets.add({"Classic Neve", 0.5f, 0.5f, 0.7f, 1.0f, false, true});
    factoryPresets.add({"Heavy Drive", 0.85f, 0.6f, 0.8f, 1.0f, false, true});
    factoryPresets.add({"Mic Preamp", 0.3f, 0.7f, 0.5f, 1.0f, true, false});
    factoryPresets.add({"Hi-Z Guitar", 0.4f, 0.4f, 0.6f, 1.0f, false, true});
  }

  juce::File getPresetFile() const {
    auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto presetDir = docs.getChildFile("Neve Transformer");
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
