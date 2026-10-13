#pragma once
#include "../Animation.h"

// A mirrored, ever-changing mandala; petals burst outwards on each beat, the downbeat adds more symmetry.
class KaleidoscopeAnim : public Animation
{
public:
    void onBeat (const FrameContext& c) override
    {
        hueShift += 0.07f;
        if (c.beat.downbeat()) segments = segments >= 16 ? 6 : segments + 2;
    }

    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.2f);
        const auto ctr = c.centre();
        const float R = c.unit() * 0.48f;
        const float step = bc::TAU / (float) segments;
        const int elements = 11;

        for (int w = 0; w < segments; ++w)
            for (int mirror = 0; mirror < 2; ++mirror)
                for (int j = 0; j < elements; ++j)
                {
                    const float f = (float) j / (float) elements;
                    const float r = R * (0.12f + 0.88f * f) * (1.f + 0.1f * c.pulse * std::sin (f * 9.f));
                    const float local = (0.5f + 0.5f * std::sin (c.time * (0.6f + 0.13f * (float) j) + (float) j * 1.3f)) * step * 0.5f;
                    const float a = (float) w * step + (mirror ? -local : local) + c.time * 0.1f;
                    const float x = ctr.x + std::cos (a) * r, y = ctr.y + std::sin (a) * r;
                    const float sz = R * (0.015f + 0.05f * (1.f - f)) * (1.f + 1.4f * c.pulse * c.intensity);
                    const auto col = c.grad (std::fmod (f + hueShift + 0.2f * (float) mirror, 1.f));

                    c.g.setColour (col.withAlpha (0.2f));
                    c.g.fillEllipse (x - sz * 2.f, y - sz * 2.f, sz * 4.f, sz * 4.f);
                    c.g.setColour (col.withAlpha (0.9f));
                    if (j % 3 == 0)                                            // petals: elongated ellipses along the radius
                    {
                        juce::Path petal;
                        petal.addEllipse (-sz * 2.2f, -sz * 0.7f, sz * 4.4f, sz * 1.4f);
                        c.g.fillPath (petal, juce::AffineTransform::rotation (a).translated (x, y));
                    }
                    else
                        c.g.fillEllipse (x - sz, y - sz, sz * 2.f, sz * 2.f);
                }

        c.glow (ctr, R * 0.25f, c.col (0), 0.4f + 0.5f * c.pulse);
    }

private:
    int segments = 8;
    float hueShift = 0.f;
};
BC_REGISTER_ANIMATION (KaleidoscopeAnim, "kaleido", "Kaleidoscope", "Psychedelic")
