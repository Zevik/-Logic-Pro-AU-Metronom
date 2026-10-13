#pragma once
#include "../Animation.h"

// A hypnotic eye: the pupil dilates on every beat, rings spiral outwards, and it blinks on the downbeat.
class HypnoEyeAnim : public Animation
{
public:
    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.2f);
        const auto ctr = c.centre();
        const float u = c.unit();

        // expanding hypnotic rings behind the eye
        const float maxR = juce::jmax (c.area.getWidth(), c.area.getHeight()) * 0.75f;
        const int rings = 14;
        for (int k = 0; k < rings; ++k)
        {
            const float f = ((float) k + c.beat.phase) / (float) rings;
            const float r = f * maxR;
            c.g.setColour (c.col (k).withAlpha ((1.f - f) * 0.35f * (0.4f + c.intensity)));
            c.g.drawEllipse (ctr.x - r, ctr.y - r, r * 2.f, r * 2.f, 2.f + 6.f * f);
        }

        // blink on the downbeat
        float blink = 0.f;
        if (c.beat.downbeat() && c.beat.phase < 0.35f) blink = std::sin (c.beat.phase / 0.35f * bc::PI);
        const float open = 1.f - 0.92f * blink;

        const float ew = u * 0.5f;
        const float eh = u * 0.27f * open * (1.f + 0.08f * c.pulse);

        juce::Path eye;
        eye.startNewSubPath (ctr.x - ew, ctr.y);
        eye.quadraticTo (ctr.x, ctr.y - eh * 2.f, ctr.x + ew, ctr.y);
        eye.quadraticTo (ctr.x, ctr.y + eh * 2.f, ctr.x - ew, ctr.y);
        eye.closeSubPath();

        c.glow (ctr, ew * 1.4f, c.col (0), 0.25f + 0.3f * c.pulse);

        c.g.saveState();
        c.g.reduceClipRegion (eye);
        c.g.setColour (juce::Colour (0xfff2eefc));
        c.g.fillPath (eye);

        // iris
        const float ir = u * 0.2f * (1.f + 0.06f * c.pulse);
        juce::ColourGradient cg (c.col (2), ctr.x, ctr.y, c.col (0).darker (0.4f), ctr.x + ir, ctr.y, true);
        cg.addColour (0.6, c.col (1));
        c.g.setGradientFill (cg);
        c.g.fillEllipse (ctr.x - ir, ctr.y - ir, ir * 2.f, ir * 2.f);

        // radial fibres, slowly turning
        c.g.setColour (juce::Colours::white.withAlpha (0.22f));
        for (int i = 0; i < 48; ++i)
        {
            const float a = bc::TAU * (float) i / 48.f + c.time * 0.25f;
            c.g.drawLine (ctr.x + std::cos (a) * ir * 0.4f, ctr.y + std::sin (a) * ir * 0.4f,
                          ctr.x + std::cos (a) * ir, ctr.y + std::sin (a) * ir, 1.2f);
        }

        // spiral dashes
        c.g.setColour (c.col (3).withAlpha (0.55f));
        for (int i = 0; i < 90; ++i)
        {
            const float t = (float) i / 90.f;
            const float a = t * bc::TAU * 3.f - c.time * 1.5f;
            const float r = ir * (0.35f + 0.65f * t);
            c.g.fillEllipse (ctr.x + std::cos (a) * r - 2.f, ctr.y + std::sin (a) * r - 2.f, 4.f, 4.f);
        }

        // pupil: dilates on the beat
        const float pr = ir * (0.28f + 0.32f * c.pulse * (0.5f + c.intensity * 0.5f) + 0.1f * c.accent * c.pulse);
        c.g.setColour (juce::Colours::black);
        c.g.fillEllipse (ctr.x - pr, ctr.y - pr, pr * 2.f, pr * 2.f);
        c.g.setColour (juce::Colours::white.withAlpha (0.8f));
        c.g.fillEllipse (ctr.x - ir * 0.38f, ctr.y - ir * 0.45f, ir * 0.2f, ir * 0.2f);
        c.g.restoreState();

        c.g.setColour (c.col (0));
        c.g.strokePath (eye, juce::PathStrokeType (4.f, juce::PathStrokeType::curved));
    }
};
BC_REGISTER_ANIMATION (HypnoEyeAnim, "hypnoeye", "Hypnotic Eye", "Psychedelic")
