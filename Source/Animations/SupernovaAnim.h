#pragma once
#include "../ParticleSystem.h"

// A star explodes on every beat. The downbeat is a full supernova in the middle of the screen.
class SupernovaAnim : public Animation
{
public:
    SupernovaAnim()
    {
        juce::Random r (99);
        for (int i = 0; i < 120; ++i) bg.push_back ({ r.nextFloat(), r.nextFloat(), 0.5f + r.nextFloat() * 1.3f, r.nextFloat() * bc::TAU });
    }

    void onBeat (const FrameContext& c) override
    {
        const bool big = c.beat.downbeat();
        const float u = c.unit();
        Burst b;
        b.centre = big ? c.centre()
                       : juce::Point<float> (c.area.getX() + c.area.getWidth() * (0.15f + 0.7f * rng.nextFloat()),
                                             c.area.getY() + c.area.getHeight() * (0.15f + 0.7f * rng.nextFloat()));
        b.maxAge = big ? 1.3f : 0.9f;
        b.size = u * (big ? 0.55f : 0.28f);
        b.t = rng.nextFloat();
        bursts.push_back (b);

        const int n = (int) ((big ? 170 : 60) * (0.4f + c.intensity));
        for (int i = 0; i < n; ++i)
        {
            const float a = rng.nextFloat() * bc::TAU;
            const float sp = b.size * (0.4f + rng.nextFloat() * 1.8f);
            Particle p;
            p.pos = b.centre;
            p.vel = { std::cos (a) * sp, std::sin (a) * sp };
            p.life = p.maxLife = 0.7f + rng.nextFloat() * (big ? 1.4f : 0.8f);
            p.size = u * (0.003f + rng.nextFloat() * 0.005f) * (big ? 1.4f : 1.f);
            p.t = juce::jlimit (0.f, 1.f, b.t + (rng.nextFloat() - 0.5f) * 0.5f);
            sparks.spawn (p);
        }
    }

    void paint (FrameContext& c) override
    {
        c.background (0.f);
        sparks.update (c.dt, c.unit() * 0.12f, 1.6f);

        for (auto& s : bg)
        {
            const float a = 0.3f + 0.7f * (0.5f + 0.5f * std::sin (c.time * 2.f + s.ph));
            c.g.setColour (juce::Colours::white.withAlpha (a * 0.7f));
            c.g.fillEllipse (c.area.getX() + s.x * c.area.getWidth(), c.area.getY() + s.y * c.area.getHeight(), s.sz, s.sz);
        }

        for (auto& b : bursts) b.age += c.dt;
        bursts.erase (std::remove_if (bursts.begin(), bursts.end(), [] (const Burst& b) { return b.age > b.maxAge; }), bursts.end());

        for (auto& b : bursts)
        {
            const float k = b.age / b.maxAge;
            const float shock = 1.f - std::pow (1.f - k, 3.f);
            c.glow (b.centre, b.size * (0.3f + 0.7f * (1.f - k)), c.grad (b.t), 0.9f * (1.f - k));
            c.g.setColour (c.grad (b.t + 0.2f).withAlpha ((1.f - k) * 0.8f));
            const float r = b.size * 1.3f * shock;
            c.g.drawEllipse (b.centre.x - r, b.centre.y - r, r * 2.f, r * 2.f, 4.f * (1.f - k) + 1.f);
            if (k < 0.15f)
            {
                c.g.setColour (juce::Colours::white.withAlpha (1.f - k / 0.15f));
                const float fr = b.size * 0.2f * (1.f - k / 0.15f);
                c.g.fillEllipse (b.centre.x - fr, b.centre.y - fr, fr * 2.f, fr * 2.f);
            }
        }

        sparks.forEach ([&] (Particle& p)           // streaks
        {
            const float a = bc::clamp01 (p.life / p.maxLife);
            c.g.setColour (c.grad (p.t).withAlpha (a));
            c.g.drawLine (p.pos.x, p.pos.y, p.pos.x - p.vel.x * 0.03f, p.pos.y - p.vel.y * 0.03f, p.size * 1.6f * (0.4f + a));
        });
        sparks.drawDots (c, 1.8f);
    }

private:
    struct Burst { juce::Point<float> centre; float age = 0.f, maxAge = 1.f, size = 100.f, t = 0.f; };
    struct BgStar { float x, y, sz, ph; };
    juce::Random rng;
    std::vector<Burst> bursts;
    std::vector<BgStar> bg;
    ParticleSystem sparks { 1500 };
};
BC_REGISTER_ANIMATION (SupernovaAnim, "supernova", "Exploding Stars", "Space")
