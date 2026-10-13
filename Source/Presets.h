#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class BeatCanvasProcessor;

// Presets = the full plugin state (parameters + the two animation ids), stored as small XML files:
//   <Documents>/BeatCanvas/Presets/<name>.bcpreset      (copy files between machines to share them)
// plus a few built-in "factory" presets.
namespace Presets
{
    struct Item { juce::String name; bool factory; };

    juce::File directory();
    std::vector<Item> list();                                   // factory first, then user presets A-Z
    bool load (BeatCanvasProcessor&, const Item&);
    bool save (BeatCanvasProcessor&, const juce::String& name); // overwrites a user preset with the same name
    bool remove (const juce::String& name);                     // user presets only
}
