#include "Animation.h"
#include <algorithm>

AnimationRegistry& AnimationRegistry::get()
{
    static AnimationRegistry r;
    return r;
}

void AnimationRegistry::add (Entry e)
{
    entries.push_back (std::move (e));
    dirty = true;
}

const std::vector<AnimationRegistry::Entry>& AnimationRegistry::all()
{
    if (dirty)
    {
        std::stable_sort (entries.begin(), entries.end(), [] (const Entry& a, const Entry& b)
        {
            const int c = a.category.compareNatural (b.category);
            return c != 0 ? c < 0 : a.name.compareNatural (b.name) < 0;
        });
        dirty = false;
    }
    return entries;
}

const AnimationRegistry::Entry* AnimationRegistry::find (const juce::String& id)
{
    for (auto& e : all())
        if (e.id == id) return &e;
    return nullptr;
}

juce::String AnimationRegistry::defaultId (bool downbeat)
{
    return downbeat ? "supernova" : "heart";
}

// CMake generates this file: it #includes every header in Source/Animations/ (drop-in animations).
// The registrars therefore live in this translation unit and can never be stripped by the linker.
#include "AnimationIndex.inc"
