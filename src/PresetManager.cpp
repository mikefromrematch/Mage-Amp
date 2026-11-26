#include "PresetManager.h"

namespace mage
{
namespace
{
    const juce::Identifier presetNameId { "presetName" };
    const juce::String presetExtension { ".magepreset" };
}

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& s, juce::UndoManager& undo)
    : state(s), undoManager(undo)
{
    factory = {
        { "Granophyre Inspired", {} },
        { "Arcane Rhythm", { { "gain", 7.2f }, { "bass", 5.5f }, { "mid", 4.2f }, { "treble", 6.2f },
                             { "presence", 6.0f }, { "depth", 6.0f }, { "master", 5.5f }, { "gate", -50.0f },
                             { "driveEnabled", 1.0f }, { "drive", 1.0f }, { "driveTone", 6.0f },
                             { "driveLevel", 7.0f }, { "lowCut", 85.0f }, { "highCut", 9000.0f } } },
        { "Glass Spell", { { "gain", 1.2f }, { "bass", 5.5f }, { "treble", 6.5f }, { "presence", 5.5f },
                           { "depth", 4.5f }, { "master", 4.0f }, { "gateEnabled", 0.0f },
                           { "delayEnabled", 1.0f }, { "delayTime", 420.0f }, { "delayFeedback", 0.35f },
                           { "delayMix", 0.25f }, { "reverbEnabled", 1.0f }, { "reverbMix", 0.3f },
                           { "reverbSize", 0.7f }, { "reverbDecay", 0.7f }, { "output", 10.0f } } },
        { "Crunch Sigil", { { "gain", 3.8f }, { "mid", 6.5f }, { "treble", 5.5f }, { "presence", 5.5f },
                            { "depth", 4.5f }, { "master", 5.0f }, { "gate", -65.0f },
                            { "reverbEnabled", 1.0f }, { "reverbMix", 0.15f }, { "output", 9.0f } } },
        { "Leviathan Lead", { { "gain", 8.0f }, { "mid", 6.5f }, { "treble", 5.5f }, { "presence", 5.5f },
                              { "driveEnabled", 1.0f }, { "drive", 2.0f }, { "driveLevel", 5.0f },
                              { "delayEnabled", 1.0f }, { "delaySync", 1.0f }, { "delayDivision", 3.0f },
                              { "delayFeedback", 0.3f }, { "delayMix", 0.2f }, { "reverbEnabled", 1.0f },
                              { "reverbMix", 0.2f }, { "gate", -60.0f }, { "output", 7.5f } } },
        { "Abyss Drop", { { "gain", 7.8f }, { "bass", 6.0f }, { "mid", 3.8f }, { "treble", 6.0f },
                          { "depth", 6.5f }, { "gate", -45.0f },
                          { "gateDecay", 40.0f }, { "lowCut", 60.0f } } },

        // ---- Fae clean channel ----
        { "Moonlit Glass", { { "ampChannel", 1.0f }, { "gain", 4.0f }, { "bass", 5.0f }, { "mid", 5.0f },
                             { "treble", 6.0f }, { "presence", 6.0f }, { "master", 5.0f }, { "gateEnabled", 0.0f },
                             { "shimmerEnabled", 1.0f }, { "shimmerMix", 0.25f }, { "shimmerDecay", 0.55f },
                             { "shimmerAmount", 0.4f }, { "reverbEnabled", 1.0f }, { "reverbMix", 0.25f },
                             { "reverbSize", 0.65f }, { "output", 8.0f } } },
        { "Faerie Choir", { { "ampChannel", 1.0f }, { "gain", 3.5f }, { "treble", 6.5f }, { "presence", 6.0f },
                            { "gateEnabled", 0.0f }, { "shimmerEnabled", 1.0f }, { "shimmerMix", 0.55f },
                            { "shimmerDecay", 0.85f }, { "shimmerAmount", 0.75f }, { "shimmerTone", 0.7f },
                            { "delayEnabled", 1.0f }, { "delaySync", 1.0f }, { "delayDivision", 3.0f },
                            { "delayFeedback", 0.35f }, { "delayMix", 0.2f }, { "output", 8.5f } } },
        { "Will-o'-Wisp", { { "ampChannel", 1.0f }, { "gain", 5.0f }, { "bass", 4.5f }, { "mid", 5.5f },
                            { "treble", 5.5f }, { "gateEnabled", 0.0f }, { "delayEnabled", 1.0f },
                            { "delayTime", 520.0f }, { "delayFeedback", 0.45f }, { "delayMix", 0.3f },
                            { "reverbEnabled", 1.0f }, { "reverbMix", 0.35f }, { "reverbSize", 0.8f },
                            { "reverbDecay", 0.8f },
                            { "output", 8.0f } } },
        { "Thistle Breakup", { { "ampChannel", 1.0f }, { "gain", 8.5f }, { "bass", 5.0f }, { "mid", 6.5f },
                               { "treble", 5.5f }, { "presence", 5.5f }, { "master", 7.5f }, { "gate", -70.0f },
                               { "reverbEnabled", 1.0f }, { "reverbMix", 0.15f }, { "output", 7.0f } } },
        { "Frozen Grove", { { "ampChannel", 1.0f }, { "gain", 4.5f }, { "treble", 5.5f }, { "gateEnabled", 0.0f },
                            { "shimmerEnabled", 1.0f }, { "shimmerFifth", 1.0f }, { "shimmerMix", 0.4f },
                            { "shimmerDecay", 0.75f }, { "shimmerAmount", 0.6f }, { "chorusEnabled", 1.0f },
                            { "chorusRate", 0.4f }, { "chorusDepth", 0.6f }, { "chorusMix", 0.4f }, { "output", 8.5f } } },
    };

    if (!state.state.hasProperty(presetNameId))
        state.state.setProperty(presetNameId, factory.front().name, nullptr);
    loadedSnapshot = snapshot();
}

juce::File PresetManager::getUserPresetFolder()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("MAGE").getChildFile("Presets");
}

juce::Array<juce::File> PresetManager::getUserPresetFiles() const
{
    auto files = getUserPresetFolder().findChildFiles(juce::File::findFiles, false, "*" + presetExtension);
    files.sort();
    return files;
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : factory)
        names.add(f.name);
    for (const auto& file : getUserPresetFiles())
        names.addIfNotAlreadyThere(file.getFileNameWithoutExtension());
    return names;
}

int PresetManager::getNumFactoryPresets() const { return static_cast<int>(factory.size()); }

bool PresetManager::isFactory(const juce::String& name) const
{
    for (const auto& f : factory)
        if (f.name == name)
            return true;
    return false;
}

juce::String PresetManager::getCurrentName() const { return state.state.getProperty(presetNameId).toString(); }

int PresetManager::getCurrentIndex() const { return getPresetNames().indexOf(getCurrentName()); }

void PresetManager::setCurrentName(const juce::String& name)
{
    state.state.setProperty(presetNameId, name, nullptr);
}

juce::String PresetManager::snapshot() const
{
    juce::String s;
    for (auto* param : state.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
            s << ranged->paramID << '=' << juce::String(ranged->getValue(), 4) << ';';
    return s;
}

bool PresetManager::isModified() const { return snapshot() != loadedSnapshot; }

void PresetManager::applyValues(const std::function<float(juce::RangedAudioParameter&)>& valueFor)
{
    undoManager.beginNewTransaction("Load preset");
    for (auto* param : state.processor.getParameters())
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            const float normalised = ranged->convertTo0to1(valueFor(*ranged));
            ranged->beginChangeGesture();
            ranged->setValueNotifyingHost(normalised);
            ranged->endChangeGesture();
        }
    }
    undoManager.beginNewTransaction();
}

void PresetManager::loadPreset(int index)
{
    const auto names = getPresetNames();
    if (!juce::isPositiveAndBelow(index, names.size()))
        return;

    if (index < getNumFactoryPresets())
    {
        const auto& preset = factory[static_cast<size_t>(index)];
        applyValues([&preset](juce::RangedAudioParameter& param)
        {
            for (const auto& [id, value] : preset.values)
                if (id == param.paramID)
                    return value;
            return param.convertFrom0to1(param.getDefaultValue());
        });
    }
    else
    {
        const auto file = getUserPresetFolder().getChildFile(names[index] + presetExtension);
        const auto xml = juce::XmlDocument::parse(file);
        if (xml == nullptr || !xml->hasTagName("MAGEPreset"))
            return;
        applyValues([&xml](juce::RangedAudioParameter& param)
        {
            if (auto* e = xml->getChildByAttribute("id", param.paramID))
                return static_cast<float>(e->getDoubleAttribute("value"));
            return param.convertFrom0to1(param.getDefaultValue());
        });
    }

    setCurrentName(names[index]);
    loadedSnapshot = snapshot();
}

void PresetManager::step(int delta)
{
    const auto count = getPresetNames().size();
    if (count == 0)
        return;
    const int current = juce::jmax(0, getCurrentIndex());
    loadPreset((current + delta + count) % count);
}

bool PresetManager::saveUserPreset(const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName(rawName.trim());
    if (name.isEmpty() || isFactory(name))
        return false;

    juce::XmlElement xml("MAGEPreset");
    xml.setAttribute("version", 1);
    for (auto* param : state.processor.getParameters())
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
        {
            auto* e = xml.createNewChildElement("Param");
            e->setAttribute("id", ranged->paramID);
            e->setAttribute("value", ranged->convertFrom0to1(ranged->getValue()));
        }
    }

    const auto folder = getUserPresetFolder();
    if (!folder.createDirectory())
        return false;
    if (!xml.writeTo(folder.getChildFile(name + presetExtension)))
        return false;

    setCurrentName(name);
    loadedSnapshot = snapshot();
    return true;
}

bool PresetManager::deleteUserPreset(const juce::String& name)
{
    if (isFactory(name))
        return false;
    const auto file = getUserPresetFolder().getChildFile(name + presetExtension);
    const bool ok = file.existsAsFile() && file.deleteFile();
    if (ok && getCurrentName() == name)
        loadPreset(0);
    return ok;
}
} // namespace mage
