#pragma once
#include "../Animation.h"

// Glowing concentric neon rings with orbiting arcs; every beat fires a shockwave ring.
class NeonRingsAnim : public Animation
{
public:
    void onBeat (const FrameContext& c) override
    {
        waves.push_back ({ 0.f, c.beat.downbeat() });
    }

    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.2f);
        const auto ctr = c.centre();
        const float u = c.unit();

        for (auto& w : waves) w.age += c.dt;
        waves.erase (std::remove_if (waves.begin(), waves.end(), [] (const Wave& w) { return w.age > 1.4f; }), waves.end());
        for (auto& w : waves)
        {
            const float k = w.age / 1.4f;
            const float r = u * (w.big ? 0.75f : 0.5f) * (1.f - std::pow (1.f - k, 3.f));
            glowRing (c, ctr, r, c.col (w.big ? 0 : 1), (1.f - k), 4.f * (1.f - k) + 1.f);
        }

        const int rings = 5;
        for (int k = 0; k < rings; ++k)
        {
            const float r = u * (0.08f + 0.065f * (float) k) * (1.f + 0.12f * c.pulse * (1.f + 0.3f * (float) k) * c.intensity);
            const auto col = c.col (k);
            const int arcs = 2 + k;
            const float dir = (k & 1) ? -1.f : 1.f;
            const float rot = c.time * (0.6f + 0.25f * (float) k) * dir + c.pulse * 0.4f * dir;
            for (int i = 0; i < arcs; ++i)
            {
                const float a0 = rot + bc::TAU * (float) i / (float) arcs;
                const float len = bc::TAU / (float) arcs * (0.55f + 0.2f * c.pulse);
                juce::Path p;
                p.addCentredArc (ctr.x, ctr.y, r, r, 0.f, a0 + bc::PI * 0.5f, a0 + len + bc::PI * 0.5f, true);
                for (int g = 3; g >= 1; --g)
                {
                    c.g.setColour (col.withAlpha (0.1f + 0.06f * c.pulse));
                    c.g.strokePath (p, juce::PathStrokeType (3.f + 5.f * (float) g, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                }
                c.g.setColour (col.brighter (0.5f));
                c.g.strokePath (p, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }
        c.glow (ctr, u * 0.08f, c.col (0), 0.6f + 0.4f * c.pulse);
    }

private:
    struct Wave { float age; bool big; };

    static void glowRing (FrameContext& c, juce::Point<float> ctr, float r, juce::Colour col, float alpha, float w)
    {
        for (int g = 3; g >= 1; --g)
        {
            c.g.setColour (col.withAlpha (0.08f * alpha));
            c.g.drawEllipse (ctr.x - r, ctr.y - r, r * 2.f, r * 2.f, w + 6.f * (float) g);
        }
        c.g.setColour (col.brighter (0.4f).withAlpha (alpha));
        c.g.drawEllipse (ctr.x - r, ctr.y - r, r * 2.f, r * 2.f, w);
    }

    std::vector<Wave> waves;
};
BC_REGISTER_ANIMATION (NeonRingsAnim, "neonrings", "Neon Rings", "Geometry")
