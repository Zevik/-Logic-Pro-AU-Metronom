#pragma once
#include "../Animation.h"

// Classic metronome pendulum, but with a glow trail and a flash on every extreme (the click).
class PendulumAnim : public Animation
{
public:
    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.22f);
        const auto a = c.area;
        const float u = c.unit();
        const float baseY = a.getBottom() - u * 0.08f;
        const float pivotY = baseY - u * 0.06f;
        const float cx = a.getCentreX();
        const float len = u * 0.68f;
        const float maxAng = 0.62f;

        // base
        juce::Path body;
        body.addTriangle (cx - u * 0.2f, baseY, cx + u * 0.2f, baseY, cx, a.getY() + a.getHeight() * 0.12f);
        c.g.setColour (c.col (1).withAlpha (0.12f));
        c.g.fillPath (body);
        c.g.setColour (c.col (1).withAlpha (0.7f));
        c.g.strokePath (body, juce::PathStrokeType (2.f));

        auto angleAt = [&] (double beatPos) { return maxAng * std::cos ((float) (bc::PI * beatPos)); };

        // trail
        for (int k = 6; k >= 1; --k)
        {
            const float ang = angleAt (c.beat.beatPos - 0.035 * k);
            const juce::Point<float> tip (cx + std::sin (ang) * len, pivotY - std::cos (ang) * len);
            c.g.setColour (c.col (0).withAlpha (0.05f * (7 - k) * c.intensity));
            c.g.fillEllipse (tip.x - u * 0.04f, tip.y - u * 0.04f, u * 0.08f, u * 0.08f);
        }

        const float ang = angleAt (c.beat.beatPos);
        const juce::Point<float> tip (cx + std::sin (ang) * len, pivotY - std::cos (ang) * len);
        c.g.setColour (juce::Colours::white.withAlpha (0.85f));
        c.g.drawLine (cx, pivotY, tip.x, tip.y, 3.f);

        // weight slides on the rod
        const float wt = 0.45f;
        const juce::Point<float> wp (cx + std::sin (ang) * len * wt, pivotY - std::cos (ang) * len * wt);
        c.g.setColour (c.col (2));
        c.g.fillRoundedRectangle (wp.x - u * 0.03f, wp.y - u * 0.02f, u * 0.06f, u * 0.04f, 4.f);

        // bob with flash at the extremes
        c.glow (tip, u * (0.1f + 0.14f * c.pulse), c.col (c.beat.downbeat() ? 0 : 1), 0.4f + 0.6f * c.pulse);
        c.g.setColour (c.col (c.beat.downbeat() ? 0 : 1));
        c.g.fillEllipse (tip.x - u * 0.035f, tip.y - u * 0.035f, u * 0.07f, u * 0.07f);
        c.g.setColour (juce::Colours::white);
        c.g.fillEllipse (tip.x - u * 0.012f, tip.y - u * 0.012f, u * 0.024f, u * 0.024f);

        c.g.setColour (juce::Colours::white);
        c.g.fillEllipse (cx - 6.f, pivotY - 6.f, 12.f, 12.f);
    }
};
BC_REGISTER_ANIMATION (PendulumAnim, "pendulum", "Glow Pendulum", "Classic")
