#pragma once
#include <juce_graphics/juce_graphics.h>
#include <vector>

// a..d form a gradient (hot -> cold / primary -> accent). Animations use grad(t) or col(i).
struct Palette
{
    juce::String name;
    juce::Colour bg, a, b, c, d;

    juce::Colour grad (float t) const
    {
        t = juce::jlimit (0.f, 1.f, t) * 3.f;
        if (t < 1.f) return a.interpolatedWith (b, t);
        if (t < 2.f) return b.interpolatedWith (c, t - 1.f);
        return c.interpolatedWith (d, t - 2.f);
    }
};

inline const std::vector<Palette>& getPalettes()
{
    static const std::vector<Palette> p {
        { "Neon Night", juce::Colour (0xff0a0618), juce::Colour (0xffff2bd6), juce::Colour (0xff00e5ff), juce::Colour (0xffb6ff3b), juce::Colour (0xffffd400) },
        { "Sunset",     juce::Colour (0xff1a0b1f), juce::Colour (0xffff4d6d), juce::Colour (0xffff9e00), juce::Colour (0xffffd60a), juce::Colour (0xff9d4edd) },
        { "Ocean",      juce::Colour (0xff021526), juce::Colour (0xff00b4d8), juce::Colour (0xff48cae4), juce::Colour (0xff90e0ef), juce::Colour (0xff7b2ff7) },
        { "Toxic",      juce::Colour (0xff08120a), juce::Colour (0xff39ff14), juce::Colour (0xffe6ff00), juce::Colour (0xff00ffa3), juce::Colour (0xffff00aa) },
        { "Candy",      juce::Colour (0xff1b1030), juce::Colour (0xffff7ac6), juce::Colour (0xff8be9ff), juce::Colour (0xfffff59e), juce::Colour (0xffb28dff) },
        { "Inferno",    juce::Colour (0xff0c0300), juce::Colour (0xfffff2b0), juce::Colour (0xffffa200), juce::Colour (0xffff3c00), juce::Colour (0xff5a0a00) },
        { "Ice",        juce::Colour (0xff04101c), juce::Colour (0xffffffff), juce::Colour (0xffaee6ff), juce::Colour (0xff4aa8ff), juce::Colour (0xff7a5cff) },
        { "Mono",       juce::Colour (0xff080808), juce::Colour (0xffffffff), juce::Colour (0xffcfcfcf), juce::Colour (0xff8a8a8a), juce::Colour (0xff4a4a4a) },
    };
    return p;
}

inline juce::StringArray getPaletteNames()
{
    juce::StringArray s;
    for (auto& p : getPalettes()) s.add (p.name);
    return s;
}
