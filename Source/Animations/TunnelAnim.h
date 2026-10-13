#pragma once
#include "../Animation.h"

// Fly through a tunnel of neon polygons. One ring passes per beat; the downbeat flips the shape.
class TunnelAnim : public Animation
{
public:
    void onBeat (const FrameContext& c) override
    {
        if (c.beat.downbeat()) sides = 3 + (sides - 2) % 6;        // 3..8
    }

    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.25f);
        const auto ctr = c.centre();
        const float u = c.unit();
        const int N = 16;
        for (int k = N; k >= 0; --k)
        {
            const float depth = (float) k + 1.f - c.beat.phase;            // ring moves one step per beat
            const float r = u * 0.03f * std::pow (1.32f, depth) * (1.f + 0.12f * c.pulse);
            const float f = depth / (float) N;
            juce::Path p;
            const float rot = c.time * 0.35f + depth * 0.08f * (1.f + c.pulse);
            for (int i = 0; i < sides; ++i)
            {
                const float a = rot + bc::TAU * (float) i / (float) sides;
                const juce::Point<float> pt (ctr.x + std::cos (a) * r, ctr.y + std::sin (a) * r);
                if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            p.closeSubPath();
            const auto col = c.grad (std::fmod (f * 0.8f + c.time * 0.05f, 1.f));
            const float alpha = bc::clamp01 (1.2f - f) * (k == 0 ? 1.f - c.beat.phase : 1.f);
            c.g.setColour (col.withAlpha (0.18f * alpha));
            c.g.strokePath (p, juce::PathStrokeType (2.f + r * 0.05f + 6.f * c.pulse));
            c.g.setColour (col.withAlpha (alpha));
            c.g.strokePath (p, juce::PathStrokeType (1.5f + r * 0.012f));
        }
        c.glow (ctr, u * 0.15f, juce::Colours::white, 0.25f + 0.4f * c.pulse);
    }

private:
    int sides = 6;
};
BC_REGISTER_ANIMATION (TunnelAnim, "tunnel", "Neon Tunnel", "Geometry")
