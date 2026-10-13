#pragma once
#include "../Animation.h"

// A dancing robot: arms alternate on each beat, the LED mouth is an equalizer, and it jumps on the downbeat.
class RobotAnim : public Animation
{
public:
    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.15f);
        const auto a = c.area;
        const float u = c.unit();
        const float cx = a.getCentreX();
        const float floorY = a.getY() + a.getHeight() * 0.85f;

        // scrolling disco floor lines
        c.g.setColour (c.col (1).withAlpha (0.35f));
        for (int i = 0; i < 8; ++i)
        {
            const float f = std::fmod ((float) i / 8.f + c.beat.phase / 8.f, 1.f);
            const float y = floorY + (a.getBottom() - floorY) * f * f;
            c.g.drawLine (a.getX(), y, a.getRight(), y, 1.f + f * 2.f);
        }

        const float jump = c.beat.downbeat() ? std::sin (bc::clamp01 (c.beat.phase * 1.4f) * bc::PI) * u * 0.14f : 0.f;
        const float squash = 1.f - 0.08f * c.pulse;
        const float bodyW = u * 0.26f, bodyH = u * 0.27f * squash;
        const float legH = u * 0.12f;
        const float bodyBottom = floorY - legH - jump;
        const float bodyTop = bodyBottom - bodyH;

        // shadow
        c.g.setColour (juce::Colours::black.withAlpha (0.4f));
        const float shw = bodyW * (0.9f - jump / u);
        c.g.fillEllipse (cx - shw, floorY - 5.f, shw * 2.f, 10.f);

        const bool leftUp = (c.beat.beatCounter & 1) == 0;
        const auto metal = c.col (2).interpolatedWith (juce::Colours::grey, 0.55f);
        const auto dark = metal.darker (0.6f);

        // legs
        c.g.setColour (dark);
        c.g.fillRoundedRectangle (cx - bodyW * 0.35f - u * 0.02f, bodyBottom, u * 0.045f, legH, 3.f);
        c.g.fillRoundedRectangle (cx + bodyW * 0.35f - u * 0.025f, bodyBottom, u * 0.045f, legH, 3.f);
        c.g.fillRoundedRectangle (cx - bodyW * 0.35f - u * 0.05f, bodyBottom + legH - 6.f, u * 0.09f, 8.f, 3.f);
        c.g.fillRoundedRectangle (cx + bodyW * 0.35f - u * 0.04f, bodyBottom + legH - 6.f, u * 0.09f, 8.f, 3.f);

        // arms: the active arm punches up on the beat
        auto arm = [&] (int side, bool up)
        {
            const float sx = cx + side * bodyW * 0.5f, sy = bodyTop + bodyH * 0.2f;
            const float raise = up ? (0.4f + 0.6f * c.pulse) : 0.05f;
            const float ang = bc::lerp (1.1f, -1.9f, raise);          // angle from straight down
            const float len = u * 0.2f;
            const float ex = sx + side * std::sin (std::abs (ang)) * len * 0.6f + side * u * 0.02f;
            const float ey = sy + std::cos (ang) * len;
            c.g.setColour (dark);
            c.g.drawLine (sx, sy, ex, ey, u * 0.03f);
            c.g.setColour (c.col (0));
            c.g.fillEllipse (ex - u * 0.03f, ey - u * 0.03f, u * 0.06f, u * 0.06f);
        };
        arm (-1, leftUp);
        arm (1, ! leftUp);

        // body + chest lights
        juce::ColourGradient bg (metal.brighter (0.3f), cx - bodyW * 0.5f, bodyTop, dark, cx + bodyW * 0.5f, bodyBottom, false);
        c.g.setGradientFill (bg);
        c.g.fillRoundedRectangle (cx - bodyW * 0.5f, bodyTop, bodyW, bodyH, u * 0.03f);
        for (int i = 0; i < 4; ++i)
        {
            c.g.setColour (c.col (i).withAlpha (i == c.beat.beatIndex % 4 ? 1.f : 0.25f));
            c.g.fillEllipse (cx - bodyW * 0.36f + i * bodyW * 0.24f, bodyTop + bodyH * 0.6f, u * 0.035f, u * 0.035f);
        }

        // head
        const float headW = u * 0.22f, headH = u * 0.17f;
        const float hy = bodyTop - headH - u * 0.015f - u * 0.03f * c.pulse;
        const float tilt = std::sin ((float) c.beat.beatPos * bc::PI) * 0.12f;
        c.g.saveState();
        c.g.addTransform (juce::AffineTransform::rotation (tilt, cx, hy + headH));

        c.g.setColour (dark);                                         // antenna
        c.g.drawLine (cx, hy, cx, hy - u * 0.06f, 3.f);
        c.glow ({ cx, hy - u * 0.07f }, u * 0.07f, c.col (0), 0.4f + 0.6f * c.pulse);
        c.g.setColour (c.col (0));
        c.g.fillEllipse (cx - u * 0.02f, hy - u * 0.09f, u * 0.04f, u * 0.04f);

        c.g.setGradientFill (juce::ColourGradient (metal.brighter (0.4f), cx - headW * 0.5f, hy, dark, cx + headW * 0.5f, hy + headH, false));
        c.g.fillRoundedRectangle (cx - headW * 0.5f, hy, headW, headH, u * 0.035f);

        // eyes: round LEDs, stars on the downbeat
        for (int s = -1; s <= 1; s += 2)
        {
            const float ex = cx + s * headW * 0.22f, ey = hy + headH * 0.36f;
            const float er = u * (0.03f + 0.012f * c.pulse);
            c.glow ({ ex, ey }, er * 2.5f, c.col (1), 0.6f);
            c.g.setColour (c.col (1).brighter (0.4f));
            if (c.beat.downbeat())
            {
                juce::Path star;
                star.addStar ({ ex, ey }, 5, er * 0.5f, er * 1.5f, c.time * 2.f);
                c.g.fillPath (star);
            }
            else
                c.g.fillEllipse (ex - er, ey - er, er * 2.f, er * 2.f);
        }

        // mouth: 5 equalizer bars
        for (int i = 0; i < 5; ++i)
        {
            const float h = headH * (0.05f + 0.2f * c.pulse * (0.4f + 0.6f * std::abs (std::sin (c.time * 7.f + i * 1.7f))));
            const float bx = cx - headW * 0.25f + i * headW * 0.125f;
            c.g.setColour (c.col (3));
            c.g.fillRect (bx - 2.5f, hy + headH * 0.82f - h, 5.f, h * 2.f);
        }
        c.g.restoreState();
    }
};
BC_REGISTER_ANIMATION (RobotAnim, "robot", "Dancing Robot", "Creatures")
