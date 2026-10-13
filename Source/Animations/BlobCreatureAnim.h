#pragma once
#include "../Animation.h"

// A goofy googly-eyed blob that squashes and jumps to the beat; its mouth opens wide on the downbeat.
class BlobCreatureAnim : public Animation
{
public:
    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.15f);
        const auto a = c.area;
        const float u = c.unit();
        const float floorY = a.getY() + a.getHeight() * 0.8f;
        const float cx = a.getCentreX();

        const float hop = c.beat.downbeat() ? 0.16f : 0.07f;
        const float h = 4.f * c.beat.phase * (1.f - c.beat.phase);
        const float lift = u * hop * h;
        const float contact = bc::clamp01 (1.f - juce::jmin (c.beat.phase, 1.f - c.beat.phase) / 0.12f);
        const float sy = 1.f - 0.28f * contact + 0.12f * h;
        const float sx = 1.f + 0.25f * contact - 0.06f * h;
        const float R = u * 0.2f;

        // shadow
        c.g.setColour (juce::Colours::black.withAlpha (0.35f));
        const float sw = R * (1.2f - 0.4f * h);
        c.g.fillEllipse (cx - sw, floorY - 6.f, sw * 2.f, 12.f);

        // wobbly body outline
        juce::Path body;
        const int N = 72;
        const float cy = floorY - R * sy - lift;
        for (int i = 0; i <= N; ++i)
        {
            const float t = bc::TAU * (float) i / (float) N;
            const float wob = 1.f + 0.04f * std::sin (t * 5.f + c.time * 4.f) + 0.05f * c.pulse * std::sin (t * 3.f);
            float x = std::cos (t) * R * sx * wob;
            float y = std::sin (t) * R * sy * wob;
            if (y > 0.f) y *= 0.85f;                                   // flatter bottom
            const juce::Point<float> p (cx + x, cy + y);
            if (i == 0) body.startNewSubPath (p); else body.lineTo (p);
        }
        body.closeSubPath();

        c.glow ({ cx, cy }, R * 2.f, c.col (0), 0.25f);
        c.g.setGradientFill (juce::ColourGradient (c.col (0).brighter (0.3f), cx - R * 0.5f, cy - R * 0.8f, c.col (1).darker (0.3f), cx + R * 0.6f, cy + R, false));
        c.g.fillPath (body);
        c.g.setColour (juce::Colours::white.withAlpha (0.3f));
        c.g.fillEllipse (cx - R * 0.6f, cy - R * 0.75f * sy, R * 0.3f, R * 0.18f);

        // horns
        for (int s = -1; s <= 1; s += 2)
        {
            juce::Path horn;
            horn.addTriangle (cx + s * R * 0.35f * sx, cy - R * 0.85f * sy, cx + s * R * 0.7f * sx, cy - R * 0.6f * sy,
                              cx + s * R * 0.55f * sx, cy - R * (1.15f + 0.1f * c.pulse) * sy);
            c.g.setColour (c.col (2));
            c.g.fillPath (horn);
        }

        // googly eyes: pupils flop around under "gravity" and jump with the beat
        for (int s = -1; s <= 1; s += 2)
        {
            const float ex = cx + s * R * 0.38f * sx, ey = cy - R * 0.22f * sy;
            const float er = R * 0.27f;
            c.g.setColour (juce::Colours::white);
            c.g.fillEllipse (ex - er, ey - er, er * 2.f, er * 2.f);
            c.g.setColour (juce::Colours::black.withAlpha (0.6f));
            c.g.drawEllipse (ex - er, ey - er, er * 2.f, er * 2.f, 2.f);
            const float px = std::sin (c.time * 2.3f + s) * 0.35f * er;
            const float py = (0.25f + 0.35f * (h - 0.5f) * 2.f - 0.25f * contact) * er + std::cos (c.time * 1.7f) * 0.1f * er;
            const float pr = er * 0.45f;
            c.g.setColour (juce::Colours::black);
            c.g.fillEllipse (ex + px - pr, ey + py - pr, pr * 2.f, pr * 2.f);
        }

        // mouth
        const float mw = R * 0.7f * sx, my = cy + R * 0.32f * sy;
        const float open = (c.beat.downbeat() ? 0.5f : 0.22f) * c.pulse + 0.05f;
        juce::Path mouth;
        mouth.startNewSubPath (cx - mw, my);
        mouth.quadraticTo (cx, my + R * (0.35f + open * 1.4f), cx + mw, my);
        mouth.quadraticTo (cx, my + R * 0.05f, cx - mw, my);
        mouth.closeSubPath();
        c.g.setColour (juce::Colour (0xff3a0a1a));
        c.g.fillPath (mouth);
        if (open > 0.2f)                                               // tongue
        {
            c.g.setColour (juce::Colour (0xffff5c8a));
            c.g.fillEllipse (cx - mw * 0.4f, my + R * (0.2f + open * 0.5f), mw * 0.8f, R * 0.22f);
        }
    }
};
BC_REGISTER_ANIMATION (BlobCreatureAnim, "blob", "Goofy Blob", "Creatures")
