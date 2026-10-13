#pragma once
#include "../ParticleSystem.h"

// Pulsing heart: thumps on every beat, mini hearts float up on the downbeat.
class HeartAnim : public Animation
{
public:
    void onBeat (const FrameContext& c) override
    {
        if (! c.beat.downbeat()) return;
        const auto ctr = c.centre();
        for (int i = 0; i < 9 + (int) (10 * c.intensity); ++i)
        {
            Particle p;
            p.pos = ctr + juce::Point<float> ((rng.nextFloat() - 0.5f) * c.unit() * 0.3f, 0.f);
            p.vel = { (rng.nextFloat() - 0.5f) * c.unit() * 0.25f, -c.unit() * (0.25f + rng.nextFloat() * 0.45f) };
            p.life = p.maxLife = 1.4f + rng.nextFloat();
            p.size = c.unit() * (0.012f + rng.nextFloat() * 0.018f);
            p.t = rng.nextFloat();
            p.spin = (rng.nextFloat() - 0.5f) * 2.f;
            minis.spawn (p);
        }
    }

    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.3f * c.intensity);
        minis.update (c.dt, 0.f, 0.4f);

        const auto ctr = c.centre();
        const float u = c.unit();
        const float s = u * (0.27f + (0.07f + 0.05f * c.accent) * c.pulse * (0.4f + c.intensity));
        const juce::Point<float> o (ctr.x, ctr.y + s * 0.15f);

        // mini hearts
        minis.forEach ([&] (Particle& p)
        {
            const float a = bc::clamp01 (p.life / p.maxLife);
            juce::Path hp = heartPath (p.size * 5.f);
            hp.applyTransform (juce::AffineTransform::rotation (std::sin (p.rot) * 0.4f).translated (p.pos));
            c.g.setColour (c.grad (p.t).withAlpha (a));
            c.g.fillPath (hp);
        });

        // halo rings that expand after each thump
        for (int k = 0; k < 3; ++k)
        {
            const float ph = bc::clamp01 (c.beat.phase * 1.2f - k * 0.12f);
            if (ph <= 0.f || ph >= 1.f) continue;
            c.g.setColour (c.col (k).withAlpha ((1.f - ph) * 0.55f));
            c.g.strokePath (heartPath (s * (1.f + ph * 0.9f)), juce::PathStrokeType (3.f * (1.f - ph) + 0.5f),
                            juce::AffineTransform::translation (o));
        }

        // glow + body
        c.glow (o, s * 1.8f, c.col (0), 0.35f + 0.4f * c.pulse);
        const auto body = heartPath (s);
        juce::ColourGradient cg (c.col (0).brighter (0.3f), o.x - s * 0.4f, o.y - s * 0.8f, c.col (1).darker (0.2f), o.x + s * 0.4f, o.y + s, false);
        c.g.setGradientFill (cg);
        c.g.fillPath (body, juce::AffineTransform::translation (o));

        // shine
        c.g.setColour (juce::Colours::white.withAlpha (0.35f));
        c.g.fillEllipse (o.x - s * 0.62f, o.y - s * 0.62f, s * 0.28f, s * 0.16f);
    }

private:
    static juce::Path heartPath (float s)
    {
        juce::Path p;
        for (int i = 0; i <= 96; ++i)
        {
            const float t = bc::TAU * (float) i / 96.f;
            const float x = 16.f * std::pow (std::sin (t), 3.f);
            const float y = -(13.f * std::cos (t) - 5.f * std::cos (2.f * t) - 2.f * std::cos (3.f * t) - std::cos (4.f * t));
            const juce::Point<float> pt (x / 17.f * s, (y - 2.5f) / 17.f * s);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        return p;
    }

    juce::Random rng;
    ParticleSystem minis { 200 };
};
BC_REGISTER_ANIMATION (HeartAnim, "heart", "Pulsing Heart", "Organic")
