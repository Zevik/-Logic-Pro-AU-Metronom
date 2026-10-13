#pragma once
#include "../ParticleSystem.h"

// Confetti cannons: every beat fires a burst; the downbeat fires both cannons at once.
class ConfettiAnim : public Animation
{
public:
    void onBeat (const FrameContext& c) override
    {
        const float u = c.unit();
        auto fire = [&] (juce::Point<float> origin, float dirX, int n)
        {
            for (int i = 0; i < n; ++i)
            {
                const float ang = -bc::PI * 0.5f + dirX * (0.15f + rng.nextFloat() * 0.5f);
                const float sp = u * (0.8f + rng.nextFloat() * 0.9f);
                Particle p;
                p.pos = origin;
                p.vel = { std::cos (ang) * sp, std::sin (ang) * sp };
                p.life = p.maxLife = 1.8f + rng.nextFloat() * 1.2f;
                p.size = u * (0.008f + rng.nextFloat() * 0.01f);
                p.t = rng.nextFloat();
                p.rot = rng.nextFloat() * bc::TAU;
                p.spin = (rng.nextFloat() - 0.5f) * 14.f;
                items.spawn (p);
            }
        };
        const int n = (int) (35 * (0.4f + c.intensity));
        const float y = c.area.getBottom();
        const bool left = (c.beat.beatCounter & 1) == 0;
        if (c.beat.downbeat()) { fire ({ c.area.getX() + u * 0.1f, y }, 1.f, n * 2); fire ({ c.area.getRight() - u * 0.1f, y }, -1.f, n * 2); }
        else if (left)         fire ({ c.area.getX() + u * 0.1f, y }, 1.f, n);
        else                   fire ({ c.area.getRight() - u * 0.1f, y }, -1.f, n);
    }

    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.15f);
        items.update (c.dt, c.unit() * 1.1f, 1.3f);
        items.forEach ([&] (Particle& p)
        {
            const float a = bc::clamp01 (p.life / p.maxLife * 2.f);
            const float flip = std::abs (std::sin (p.rot * 1.3f));            // fake 3D flutter
            juce::Path r;
            r.addRectangle (-p.size, -p.size * 0.5f * flip - 0.5f, p.size * 2.f, p.size * flip + 1.f);
            c.g.setColour (c.grad (p.t).withAlpha (a));
            c.g.fillPath (r, juce::AffineTransform::rotation (p.rot).translated (p.pos));
        });
        // cannon mouths
        for (int s = 0; s < 2; ++s)
        {
            const float x = s == 0 ? c.area.getX() + c.unit() * 0.1f : c.area.getRight() - c.unit() * 0.1f;
            c.glow ({ x, c.area.getBottom() }, c.unit() * 0.12f, c.col (s), 0.3f + 0.5f * c.pulse);
        }
    }

private:
    juce::Random rng;
    ParticleSystem items { 1500 };
};
BC_REGISTER_ANIMATION (ConfettiAnim, "confetti", "Confetti Cannons", "Particles")
