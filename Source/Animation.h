#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>
#include "BeatClock.h"
#include "Palette.h"

namespace bc
{
constexpr float PI = juce::MathConstants<float>::pi;
constexpr float TAU = juce::MathConstants<float>::twoPi;
inline float lerp (float a, float b, float t) { return a + (b - a) * t; }
inline float clamp01 (float x) { return juce::jlimit (0.f, 1.f, x); }
inline float easeOutBack (float t) { const float c1 = 1.70158f, c3 = c1 + 1.f; t -= 1.f; return 1.f + c3 * t * t * t + c1 * t * t; }
inline float easeInOut (float t) { return t * t * (3.f - 2.f * t); }
}

// Everything an animation needs for one frame. Animations must treat it as read-only.
struct FrameContext
{
    explicit FrameContext (juce::Graphics& gr) : g (gr) {}

    juce::Graphics& g;
    juce::Rectangle<float> area;
    BeatInfo beat;                 // beat.phase can be > 1 while an outgoing animation fades out
    float pulse = 0.f;             // 1 on the beat -> 0, shaped by the "Style" parameter. Use this for punch.
    float accent = 0.f;            // 1 on the first beat of the bar, otherwise 0
    float time = 0.f;              // seconds, scaled by the Speed control. Use for free-running motion.
    float dt = 0.f;                // seconds since last frame, scaled by Speed. Use for particles/physics.
    float intensity = 0.7f;        // 0..1 Intensity control
    const Palette* pal = nullptr;

    juce::Point<float> centre() const { return area.getCentre(); }
    float unit() const { return juce::jmin (area.getWidth(), area.getHeight()); }
    juce::Colour col (int i) const { const auto& p = *pal; switch (((i % 4) + 4) % 4) { case 0: return p.a; case 1: return p.b; case 2: return p.c; default: return p.d; } }
    juce::Colour grad (float t) const { return pal->grad (t); }

    void background (float flash = 0.f) const
    {
        const auto c = centre();
        juce::ColourGradient cg (pal->bg.interpolatedWith (pal->a, bc::clamp01 (0.08f + flash)), c.x, c.y,
                                 pal->bg.darker (0.5f), area.getRight(), area.getBottom(), true);
        g.setGradientFill (cg);
        g.fillRect (area);
    }

    void glow (juce::Point<float> p, float radius, juce::Colour c, float alpha = 1.f) const
    {
        juce::ColourGradient cg (c.withAlpha (alpha), p.x, p.y, c.withAlpha (0.f), p.x + radius, p.y, true);
        g.setGradientFill (cg);
        g.fillEllipse (p.x - radius, p.y - radius, radius * 2.f, radius * 2.f);
    }
};

class Animation
{
public:
    virtual ~Animation() = default;
    // Called once at the start of every beat, right before paint(). ctx.beat tells you which beat it is.
    virtual void onBeat (const FrameContext&) {}
    virtual void paint (FrameContext& ctx) = 0;
};

class AnimationRegistry
{
public:
    struct Entry
    {
        juce::String id, name, category;
        std::function<std::unique_ptr<Animation>()> create;
    };

    static AnimationRegistry& get();
    void add (Entry e);
    const std::vector<Entry>& all();                 // sorted by category, then name
    const Entry* find (const juce::String& id);
    static juce::String defaultId (bool downbeat);

private:
    std::vector<Entry> entries;
    bool dirty = false;
};

struct AnimationRegistrar
{
    AnimationRegistrar (const char* id, const char* name, const char* category,
                        std::function<std::unique_ptr<Animation>()> f)
    {
        AnimationRegistry::get().add ({ id, name, category, std::move (f) });
    }
};

// Put this at the bottom of an animation header in Source/Animations/. That is all it takes to add one.
#define BC_REGISTER_ANIMATION(Class, id, name, category) \
    static AnimationRegistrar bcRegistrar_##Class { id, name, category, [] { return std::unique_ptr<Animation> (new Class()); } };
