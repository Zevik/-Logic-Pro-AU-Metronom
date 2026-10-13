#pragma once
#include <algorithm>
#include <cmath>
#include "Animation.h"

// Small reusable particle pool for animations. Positions are in pixels.
struct Particle
{
    juce::Point<float> pos, vel;
    float life = 0.f, maxLife = 1.f;   // seconds
    float size = 3.f;
    float rot = 0.f, spin = 0.f;
    float t = 0.f;                     // 0..1 colour position (palette gradient)
    juce::Colour colour;
    bool useGrad = true;

    float age01() const { return maxLife > 0.f ? 1.f - life / maxLife : 1.f; }   // 0 newborn -> 1 dying
};

class ParticleSystem
{
public:
    explicit ParticleSystem (size_t cap = 1200) : capacity (cap) { items.reserve (cap); }

    void spawn (const Particle& p)
    {
        if (items.size() < capacity) items.push_back (p);
        else items[(next++) % capacity] = p;
    }

    void update (float dt, float gravity = 0.f, float drag = 0.f)
    {
        const float k = std::exp (-drag * dt);
        for (auto& p : items)
        {
            p.vel.y += gravity * dt;
            p.vel *= k;
            p.pos += p.vel * dt;
            p.rot += p.spin * dt;
            p.life -= dt;
        }
        items.erase (std::remove_if (items.begin(), items.end(), [] (const Particle& p) { return p.life <= 0.f; }), items.end());
    }

    template <typename Fn>
    void forEach (Fn&& fn) { for (auto& p : items) fn (p); }

    void clear() { items.clear(); }
    size_t size() const { return items.size(); }

    // soft dots: faint halo + solid core. Fades and shrinks with age.
    void drawDots (const FrameContext& c, float haloScale = 2.2f) const
    {
        for (auto& p : items)
        {
            const float a = bc::clamp01 (p.life / juce::jmax (0.001f, p.maxLife));
            const float s = p.size * (0.4f + 0.6f * a);
            const auto col = p.useGrad ? c.grad (p.t) : p.colour;
            c.g.setColour (col.withAlpha (0.18f * a));
            c.g.fillEllipse (p.pos.x - s * haloScale, p.pos.y - s * haloScale, s * haloScale * 2.f, s * haloScale * 2.f);
            c.g.setColour (col.withAlpha (a));
            c.g.fillEllipse (p.pos.x - s, p.pos.y - s, s * 2.f, s * 2.f);
        }
    }

private:
    std::vector<Particle> items;
    size_t capacity, next = 0;
};
