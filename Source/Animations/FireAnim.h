#pragma once
#include "../ParticleSystem.h"

// A bonfire that flares up on every beat; the downbeat throws embers into the air.
class FireAnim : public Animation
{
public:
    void onBeat (const FrameContext& c) override
    {
        if (! c.beat.downbeat()) return;
        const float u = c.unit();
        for (int i = 0; i < (int) (30 * (0.3f + c.intensity)); ++i)
        {
            Particle p;
            p.pos = { c.area.getCentreX() + (rng.nextFloat() - 0.5f) * u * 0.3f, c.area.getBottom() - u * 0.15f };
            p.vel = { (rng.nextFloat() - 0.5f) * u * 0.7f, -u * (0.5f + rng.nextFloat() * 0.9f) };
            p.life = p.maxLife = 1.5f + rng.nextFloat() * 1.5f;
            p.size = u * (0.004f + rng.nextFloat() * 0.004f);
            p.t = 0.f;
            embers.spawn (p);
        }
    }

    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.22f * c.intensity);
        const float u = c.unit();
        const float baseY = c.area.getBottom() - u * 0.12f;
        const float width = c.area.getWidth() * (0.22f + 0.1f * c.pulse);

        // emit
        emit += (60.f + 380.f * c.pulse * (0.4f + c.intensity)) * c.dt;
        while (emit >= 1.f)
        {
            emit -= 1.f;
            Particle p;
            const float spread = (rng.nextFloat() + rng.nextFloat() - 1.f);     // triangular: dense in the middle
            p.pos = { c.area.getCentreX() + spread * width, baseY };
            p.vel = { -spread * u * 0.12f + (rng.nextFloat() - 0.5f) * u * 0.05f,
                      -u * (0.35f + rng.nextFloat() * 0.45f) * (1.f + 0.7f * c.pulse) };
            p.life = p.maxLife = 0.7f + rng.nextFloat() * 0.8f;
            p.size = u * (0.03f + rng.nextFloat() * 0.03f) * (1.f - std::abs (spread) * 0.5f);
            flames.spawn (p);
        }
        flames.update (c.dt, 0.f, 0.3f);
        embers.update (c.dt, u * 0.25f, 0.5f);

        c.glow ({ c.area.getCentreX(), baseY }, u * 0.6f, c.col (2), 0.25f + 0.3f * c.pulse);

        // logs
        c.g.setColour (juce::Colour (0xff2a1608));
        const float lw = u * 0.34f, lh = u * 0.035f;
        for (int s = -1; s <= 1; s += 2)
        {
            juce::Path log;
            log.addRoundedRectangle (-lw * 0.5f, -lh * 0.5f, lw, lh, lh * 0.5f);
            c.g.fillPath (log, juce::AffineTransform::rotation (s * 0.18f).translated (c.area.getCentreX(), baseY + lh));
        }

        // flames: draw back-to-front by age, colour goes hot -> cold over a particle's life
        flames.forEach ([&] (Particle& p)
        {
            const float age = p.age01();
            const float a = bc::clamp01 (p.life / p.maxLife);
            const float s = p.size * (1.f - 0.6f * age);
            c.g.setColour (c.grad (age).withAlpha (0.12f * a));
            c.g.fillEllipse (p.pos.x - s * 1.8f, p.pos.y - s * 1.8f, s * 3.6f, s * 3.6f);
            c.g.setColour (c.grad (age).withAlpha (0.55f * a));
            c.g.fillEllipse (p.pos.x - s, p.pos.y - s * 1.3f, s * 2.f, s * 2.6f);
        });
        embers.drawDots (c, 2.f);
    }

private:
    juce::Random rng;
    float emit = 0.f;
    ParticleSystem flames { 1000 }, embers { 300 };
};
BC_REGISTER_ANIMATION (FireAnim, "fire", "Bonfire", "Elements")
