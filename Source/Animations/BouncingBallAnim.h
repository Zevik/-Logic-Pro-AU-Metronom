#pragma once
#include "../Animation.h"

// A ball ping-pongs between the sides and hits the floor exactly on every beat (squash & stretch included).
class BouncingBallAnim : public Animation
{
public:
    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.15f);
        const auto a = c.area;
        const float u = c.unit();
        const float floorY = a.getY() + a.getHeight() * 0.78f;
        const float p = c.beat.phase;

        // lines on the floor
        c.g.setColour (c.col (1).withAlpha (0.8f));
        c.g.fillRect (a.getX(), floorY, a.getWidth(), 3.f);
        c.glow ({ a.getCentreX(), floorY }, a.getWidth() * 0.5f, c.col (1), 0.12f);

        const float r = u * (c.beat.downbeat() ? 0.075f : 0.06f);
        const float peak = a.getHeight() * (c.beat.downbeat() ? 0.58f : 0.42f) * (0.7f + 0.3f * c.intensity);
        auto sideX = [&] (int64_t n) { return a.getCentreX() + ((n & 1) ? 1.f : -1.f) * a.getWidth() * 0.25f; };

        auto drawBall = [&] (float ph, float alpha, bool main)
        {
            const float fx = bc::lerp (sideX (c.beat.beatCounter), sideX (c.beat.beatCounter + 1), ph);
            const float h = 4.f * ph * (1.f - ph);                 // 0 at the beats, 1 at the apex
            const float cy = floorY - r - peak * h;
            const float contact = bc::clamp01 (1.f - juce::jmin (ph, 1.f - ph) / 0.1f);
            const float stretch = std::abs (1.f - 2.f * ph) * (1.f - contact);
            const float sx = 1.f + 0.35f * contact - 0.12f * stretch;
            const float sy = 1.f - 0.4f * contact + 0.2f * stretch;
            const auto col = c.col (c.beat.downbeat() ? 0 : 2).withAlpha (alpha);
            const float w = r * 2.f * sx, hh = r * 2.f * sy;
            const float bottom = floorY;                           // keep the squashed ball glued to the floor
            const float y = juce::jmin (cy + hh * 0.5f, bottom) - hh * 0.5f;
            if (main)
            {
                // shadow
                const float sw = r * (2.4f - 1.4f * h);
                c.g.setColour (juce::Colours::black.withAlpha (0.35f));
                c.g.fillEllipse (fx - sw, floorY - 4.f, sw * 2.f, 8.f);
                c.glow ({ fx, y + hh * 0.5f }, r * 3.f, col, 0.35f);
            }
            juce::ColourGradient cg (col.brighter (0.6f), fx - w * 0.25f, y + hh * 0.2f, col.darker (0.4f), fx + w * 0.4f, y + hh, true);
            c.g.setGradientFill (cg);
            c.g.fillEllipse (fx - w * 0.5f, y, w, hh);
            if (main)
            {
                c.g.setColour (juce::Colours::white.withAlpha (0.5f));
                c.g.fillEllipse (fx - w * 0.28f, y + hh * 0.14f, w * 0.22f, hh * 0.14f);
            }
        };

        for (int k = 5; k >= 1; --k)                               // motion trail
        {
            const float ph = p - 0.025f * (float) k;
            if (ph >= 0.f) drawBall (ph, 0.08f * (6 - k) * c.intensity, false);
        }
        drawBall (p, 1.f, true);

        // impact ripple
        if (p < 0.5f)
        {
            const float rp = p / 0.5f;
            const float rx = sideX (c.beat.beatCounter);
            c.g.setColour (c.col (0).withAlpha ((1.f - rp) * 0.8f));
            c.g.drawEllipse (rx - u * 0.3f * rp, floorY - u * 0.05f * rp, u * 0.6f * rp, u * 0.1f * rp, 3.f);
        }
    }
};
BC_REGISTER_ANIMATION (BouncingBallAnim, "ball", "Bouncing Ball", "Classic")
