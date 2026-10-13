#pragma once
#include "../Animation.h"

// A spiral galaxy that breathes with the beat; the downbeat sends a shockwave through the arms.
class GalaxyAnim : public Animation
{
public:
    GalaxyAnim()
    {
        juce::Random r (1234);
        for (int i = 0; i < 900; ++i)
        {
            Star s;
            s.r = std::pow (r.nextFloat(), 0.7f);
            s.arm = (float) (r.nextInt (3)) * bc::TAU / 3.f;
            s.scatter = (r.nextFloat() - 0.5f) * (0.15f + 0.5f * s.r);
            s.size = 0.6f + r.nextFloat() * 1.8f;
            s.tw = r.nextFloat() * bc::TAU;
            stars.push_back (s);
        }
    }

    void onBeat (const FrameContext& c) override { if (c.beat.downbeat()) wave = 1.f; }

    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.18f);
        wave = juce::jmax (0.f, wave - c.dt * 1.2f);

        const auto ctr = c.centre();
        const float R = c.unit() * 0.5f * (1.f + 0.07f * c.pulse * c.intensity);
        const float twist = 0.55f * c.pulse * c.intensity;
        const float shock = (1.f - wave);                       // 0 -> 1 as the wave travels outwards

        c.glow (ctr, R * 0.5f, c.col (0), 0.3f + 0.5f * c.pulse);

        for (auto& s : stars)
        {
            const float spin = c.time * 0.35f / (s.r + 0.25f);
            const float ang = s.arm + s.r * 4.2f + s.scatter + spin + twist * (1.f - s.r);
            float rr = s.r * R;
            const float dw = std::abs (s.r - shock);
            if (wave > 0.f && dw < 0.08f) rr += R * 0.05f * wave * (1.f - dw / 0.08f);
            const float x = ctr.x + std::cos (ang) * rr;
            const float y = ctr.y + std::sin (ang) * rr * 0.62f;       // tilt the disc
            const float tw = 0.65f + 0.35f * std::sin (c.time * 3.f + s.tw);
            const float boost = 1.f + 1.5f * c.pulse * (1.f - s.r) + ((wave > 0.f && dw < 0.08f) ? 1.f : 0.f);
            c.g.setColour (c.grad (s.r).withAlpha (juce::jmin (1.f, tw * 0.9f)));
            const float sz = s.size * boost;
            c.g.fillEllipse (x - sz * 0.5f, y - sz * 0.5f, sz, sz);
        }

        c.g.setColour (juce::Colours::white.withAlpha (0.6f + 0.4f * c.pulse));
        const float cr = R * (0.04f + 0.05f * c.pulse);
        c.g.fillEllipse (ctr.x - cr, ctr.y - cr, cr * 2.f, cr * 2.f);
    }

private:
    struct Star { float r, arm, scatter, size, tw; };
    std::vector<Star> stars;
    float wave = 0.f;
};
BC_REGISTER_ANIMATION (GalaxyAnim, "galaxy", "Spiral Galaxy", "Space")
