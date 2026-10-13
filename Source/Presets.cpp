#include "Presets.h"
#include "PluginProcessor.h"
#include "Palette.h"

namespace Presets
{
namespace
{
    struct Factory
    {
        const char* name; const char* first; const char* other;
        const char* palette; int style; float intensity, speed;
    };

    const Factory factory[] = {
        { "Heartbeat",        "supernova", "heart",     "Sunset",     0, 0.75f, 1.0f },
        { "Disco Robot",      "confetti",  "robot",     "Candy",      1, 0.80f, 1.0f },
        { "Deep Space",       "supernova", "galaxy",    "Neon Night", 0, 0.70f, 0.8f },
        { "Bonfire Night",    "fire",      "fire",      "Inferno",    0, 0.85f, 1.0f },
        { "Hypnotized",       "kaleido",   "hypnoeye",  "Toxic",      2, 0.80f, 1.2f },
        { "Warp Drive",       "tunnel",    "neonrings", "Neon Night", 1, 0.90f, 1.5f },
        { "Classic (Clean)",  "pendulum",  "pendulum",  "Mono",       1, 0.50f, 1.0f },
        { "Ocean Calm",       "waves",     "waves",     "Ocean",      0, 0.50f, 0.8f },
        { "Bouncy Party",     "blob",      "ball",      "Candy",      2, 0.90f, 1.0f },
    };

    void setParam (BeatCanvasProcessor& p, const char* id, float plainValue)
    {
        if (auto* prm = p.apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
    }

    juce::File fileFor (const juce::String& name)
    {
        return directory().getChildFile (juce::File::createLegalFileName (name) + ".bcpreset");
    }
}

juce::File directory()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("BeatCanvas").getChildFile ("Presets");
}

std::vector<Item> list()
{
    std::vector<Item> out;
    for (auto& f : factory) out.push_back ({ f.name, true });

    juce::StringArray user;
    for (auto& f : directory().findChildFiles (juce::File::findFiles, false, "*.bcpreset"))
        user.add (f.getFileNameWithoutExtension());
    user.sortNatural();
    for (auto& n : user) out.push_back ({ n, false });
    return out;
}

bool load (BeatCanvasProcessor& p, const Item& item)
{
    if (item.factory)
    {
        for (auto& f : factory)
            if (item.name == f.name)
            {
                p.setAnimationId (true, f.first);
                p.setAnimationId (false, f.other);
                setParam (p, "palette", (float) getPaletteNames().indexOf (f.palette));
                setParam (p, "style", (float) f.style);
                setParam (p, "intensity", f.intensity);
                setParam (p, "speed", f.speed);
                return true;
            }
        return false;
    }

    auto xml = juce::XmlDocument::parse (fileFor (item.name));
    if (xml == nullptr || ! xml->hasTagName (p.apvts.state.getType())) return false;

    const auto w = p.apvts.state.getProperty ("w"), h = p.apvts.state.getProperty ("h");   // keep the window size
    auto tree = juce::ValueTree::fromXml (*xml);
    tree.setProperty ("w", w, nullptr);
    tree.setProperty ("h", h, nullptr);
    p.apvts.replaceState (tree);
    return true;
}

bool save (BeatCanvasProcessor& p, const juce::String& name)
{
    if (name.trim().isEmpty()) return false;
    directory().createDirectory();
    auto tree = p.apvts.copyState();
    tree.removeProperty ("w", nullptr);
    tree.removeProperty ("h", nullptr);
    if (auto xml = tree.createXml()) return xml->writeTo (fileFor (name.trim()));
    return false;
}

bool remove (const juce::String& name)
{
    return fileFor (name).deleteFile();
}
}
