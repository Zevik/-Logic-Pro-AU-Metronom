#pragma once
#include "../Animation.h"

// Layered ocean waves that swell on the beat, with droplets that make ripples on every hit.
class WavesAnim : public Animation
{
public:
    void onBeat (const FrameContext& c) override
    {
        Ripple r;
        r.x = c.area.getX() + c.area.getWidth() * (c.beat.downbeat() ? 0.5f : 0.15f + 0.7f * rng.nextFloat());
        r.big = c.beat.downbeat();
        ripples.push_back (r);
    }

    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.12f);
        const auto a = c.area;
        const float u = c.unit();
        const int layers = 6;
        const float amp = u * (0.025f + 0.07f * c.pulse * (0.3f + c.intensity) + 0.02f * c.accent * c.pulse);

        for (int L = 0; L < layers; ++L)
        {
            const float f = (float) L / (float) (layers - 1);
            const float baseY = a.getY() + a.getHeight() * (0.42f + 0.1f * L / layers * 2.f);
            juce::Path p;
            p.startNewSubPath (a.getX(), a.getBottom());
            for (float x = 0.f; x <= a.getWidth() + 8.f; x += 8.f)
            {
                const float k = x / a.getWidth();
                const float y = baseY
                                + std::sin (k * (5.f + L) + c.time * (0.8f + 0.25f * L) + L) * amp * (1.f - 0.1f * L)
                                + std::sin (k * 11.f - c.time * 1.3f + L * 2.f) * amp * 0.3f;
                p.lineTo (a.getX() + x, y);
            }
            p.lineTo (a.getRight(), a.getBottom());
            p.closeSubPath();

            juce::ColourGradient cg (c.grad (f).withAlpha (0.75f), 0.f, baseY - amp, c.pal->bg.withAlpha (0.9f), 0.f, a.getBottom(), false);
            c.g.setGradientFill (cg);
            c.g.fillPath (p);
            c.g.setColour (c.grad (f).brighter (0.4f).withAlpha (0.8f));
            c.g.strokePath (p, juce::PathStrokeType (1.5f));
        }

        // ripples + droplets
        const float surface = a.getY() + a.getHeight() * 0.45f;
        for (auto& r : ripples) r.age += c.dt;
        ripples.erase (std::remove_if (ripples.begin(), ripples.end(), [] (const Ripple& r) { return r.age > 1.6f; }), ripples.end());
        for (auto& r : ripples)
        {
            const float k = r.age / 1.6f;
            if (r.age < 0.35f)                                              // falling drop
            {
                const float dy = surface - a.getHeight() * 0.4f * (1.f - r.age / 0.35f) * (1.f - r.age / 0.35f);
                c.g.setColour (c.col (1));
                c.g.fillEllipse (r.x - 4.f, dy - 6.f, 8.f, 12.f);
            }
            else
            {
                const float kk = (r.age - 0.35f) / 1.25f;
                for (int j = 0; j < 3; ++j)
                {
                    const float rr = (kk * (r.big ? 0.5f : 0.3f) * u) * (1.f - j * 0.25f);
                    if (rr <= 0.f) continue;
                    c.g.setColour (c.col (j).withAlpha ((1.f - kk) * 0.8f));
                    c.g.drawEllipse (r.x - rr, surface - rr * 0.25f, rr * 2.f, rr * 0.5f, 2.5f);
                }
            }
            (void) k;
        }
    }

private:
    struct Ripple { float x = 0.f, age = 0.f; bool big = false; };
    juce::Random rng;
    std::vector<Ripple> ripples;
};
BC_REGISTER_ANIMATION (WavesAnim, "waves", "Ocean Waves", "Elements")
