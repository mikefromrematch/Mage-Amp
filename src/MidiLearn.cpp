#include "MidiLearn.h"
#include "PresetManager.h"

namespace mage
{
MidiLearn::MidiLearn(juce::AudioProcessor& processor, juce::String mapFileName)
    : mapFile(std::move(mapFileName))
{
    for (auto* p : processor.getParameters())
    {
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
        {
            params.add(ranged);
            const bool twoWay = dynamic_cast<juce::AudioParameterBool*>(ranged) != nullptr
                || (dynamic_cast<juce::AudioParameterChoice*>(ranged) != nullptr && ranged->getNumSteps() == 2);
            isSwitch.add(twoWay);
        }
    }
    for (auto& slot : ccToParam)
        slot.store(-1);
}

void MidiLearn::process(const juce::MidiBuffer& midi) noexcept
{
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (!msg.isController())
            continue;
        const int cc = msg.getControllerNumber();
        if (cc < 0 || cc > 127)
            continue;

        if (const int target = learning.exchange(-1); target >= 0)
        {
            // One CC per parameter: drop any older CC that pointed at this one.
            for (auto& slot : ccToParam)
            {
                int expected = target;
                slot.compare_exchange_strong(expected, -1);
            }
            ccToParam[(size_t)cc].store(target);
            lastLearnedParam.store(target);
            changes.fetch_add(1);
        }

        const int index = ccToParam[(size_t)cc].load();
        if (index >= 0)
            apply(index, msg.getControllerValue());
    }
}

void MidiLearn::apply(int index, int value) noexcept
{
    if (index >= params.size())
        return;
    auto* param = params.getUnchecked(index);
    const float normalised = isSwitch[index] ? (value >= 64 ? 1.0f : 0.0f) : (float)value / 127.0f;
    if (std::abs(param->getValue() - normalised) > 1.0e-6f)
        param->setValueNotifyingHost(normalised);
}

int MidiLearn::indexOf(const juce::String& paramID) const
{
    for (int i = 0; i < params.size(); ++i)
        if (params.getUnchecked(i)->getParameterID() == paramID)
            return i;
    return -1;
}

juce::String MidiLearn::getParamID(int index) const
{
    return juce::isPositiveAndBelow(index, params.size()) ? params.getUnchecked(index)->getParameterID() : juce::String();
}

void MidiLearn::startLearning(const juce::String& paramID) { learning.store(indexOf(paramID)); }

juce::String MidiLearn::getLearningParam() const { return getParamID(learning.load()); }

int MidiLearn::getCCFor(const juce::String& paramID) const
{
    const int index = indexOf(paramID);
    if (index < 0)
        return -1;
    for (int cc = 0; cc < 128; ++cc)
        if (ccToParam[(size_t)cc].load() == index)
            return cc;
    return -1;
}

void MidiLearn::clear(const juce::String& paramID)
{
    const int index = indexOf(paramID);
    if (index < 0)
        return;
    for (auto& slot : ccToParam)
    {
        int expected = index;
        slot.compare_exchange_strong(expected, -1);
    }
    changes.fetch_add(1);
}

void MidiLearn::clearAll()
{
    for (auto& slot : ccToParam)
        slot.store(-1);
    changes.fetch_add(1);
}

bool MidiLearn::hasAnyMapping() const
{
    for (auto& slot : ccToParam)
        if (slot.load() >= 0)
            return true;
    return false;
}

juce::ValueTree MidiLearn::toValueTree() const
{
    juce::ValueTree tree(treeType);
    for (int cc = 0; cc < 128; ++cc)
    {
        const auto id = getParamID(ccToParam[(size_t)cc].load());
        if (id.isNotEmpty())
            tree.appendChild(juce::ValueTree("Map", { { "cc", cc }, { "param", id } }), nullptr);
    }
    return tree;
}

void MidiLearn::fromValueTree(const juce::ValueTree& tree)
{
    if (!tree.hasType(treeType))
        return;
    for (auto& slot : ccToParam)
        slot.store(-1);
    for (const auto child : tree)
    {
        const int cc = child.getProperty("cc", -1);
        const int index = indexOf(child.getProperty("param").toString());
        if (juce::isPositiveAndBelow(cc, 128) && index >= 0)
            ccToParam[(size_t)cc].store(index);
    }
    changes.fetch_add(1);
}

static juce::File defaultMapFile(const juce::String& name)
{
    return PresetManager::getUserPresetFolder().getParentDirectory().getChildFile(name);
}

void MidiLearn::saveAsDefault() const
{
    const auto file = defaultMapFile(mapFile);
    file.getParentDirectory().createDirectory();
    if (auto xml = toValueTree().createXml())
        xml->writeTo(file);
}

void MidiLearn::loadDefault()
{
    if (auto xml = juce::XmlDocument::parse(defaultMapFile(mapFile)))
        fromValueTree(juce::ValueTree::fromXml(*xml));
}
} // namespace mage
