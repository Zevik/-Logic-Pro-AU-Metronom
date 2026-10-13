#pragma once
// Pure C++ (no JUCE) so it can be unit-tested anywhere.
//
// Audio thread  -> TransportMailbox (lock-free seqlock) -> UI thread -> BeatClock
//
// The audio thread publishes, once per block, "at wall-clock time W the musical position was P (quarter notes)
// and the tempo was T". The UI never counts beats on its own: every frame it extrapolates from the latest
// anchor and gently PLL-corrects towards each new anchor. That removes block-timing jitter and cannot drift,
// because the position always comes from the host (or from the processor's sample-accurate free-run counter).

#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>

struct TransportSnapshot
{
    double ppq = 0.0;          // quarter notes
    double bpm = 120.0;        // quarter notes per minute
    double barStartPpq = 0.0;
    double wallMs = 0.0;       // monotonic ms at which `ppq` was valid
    bool running = false;      // is ppq advancing?
    int num = 4, den = 4;
    uint32_t serial = 0;
};

class TransportMailbox
{
public:
    // single writer (audio thread)
    void write (const TransportSnapshot& s) noexcept
    {
        const auto v = version.load (std::memory_order_relaxed);
        version.store (v + 1, std::memory_order_relaxed);
        std::atomic_thread_fence (std::memory_order_release);
        ppq.store (s.ppq, std::memory_order_relaxed);
        bpm.store (s.bpm, std::memory_order_relaxed);
        barStart.store (s.barStartPpq, std::memory_order_relaxed);
        wall.store (s.wallMs, std::memory_order_relaxed);
        running.store (s.running ? 1 : 0, std::memory_order_relaxed);
        num.store (s.num, std::memory_order_relaxed);
        den.store (s.den, std::memory_order_relaxed);
        version.store (v + 2, std::memory_order_release);
    }

    // any reader thread; false if nothing published yet or the read kept racing
    bool read (TransportSnapshot& out) const noexcept
    {
        for (int i = 0; i < 4; ++i)
        {
            const auto v1 = version.load (std::memory_order_acquire);
            if ((v1 & 1u) != 0 || v1 == 0) continue;
            out.ppq = ppq.load (std::memory_order_relaxed);
            out.bpm = bpm.load (std::memory_order_relaxed);
            out.barStartPpq = barStart.load (std::memory_order_relaxed);
            out.wallMs = wall.load (std::memory_order_relaxed);
            out.running = running.load (std::memory_order_relaxed) != 0;
            out.num = num.load (std::memory_order_relaxed);
            out.den = den.load (std::memory_order_relaxed);
            std::atomic_thread_fence (std::memory_order_acquire);
            if (version.load (std::memory_order_relaxed) == v1) { out.serial = v1; return true; }
        }
        return false;
    }

private:
    std::atomic<uint32_t> version { 0 };
    std::atomic<double> ppq { 0.0 }, bpm { 120.0 }, barStart { 0.0 }, wall { 0.0 };
    std::atomic<int> running { 0 }, num { 4 }, den { 4 };
};

struct BeatInfo
{
    bool valid = false;        // false until the audio thread has published at least once
    bool running = false;
    double ppq = 0.0;
    double bpm = 120.0;
    double beatPos = 0.0;      // continuous beat counter (beats since bar origin)
    int64_t beatCounter = 0;   // floor (beatPos)
    int beatIndex = 0;         // 0-based position inside the bar; 0 == downbeat
    int beatsPerBar = 4;
    float phase = 0.f;         // 0..1 inside the current beat
    float barPhase = 0.f;      // 0..1 inside the current bar
    bool downbeat() const noexcept { return beatIndex == 0; }
};

struct ClockSettings
{
    double offsetMs = 0.0;     // + = show the future (visuals earlier), - = visuals later
    bool followHostMeter = true;
    int beatsPerBar = 4;       // used when !followHostMeter
    int beatUnit = 4;          // 2,4,8,16
};

class BeatClock
{
public:
    void feed (const TransportSnapshot& s) noexcept
    {
        if (have && s.serial == lastSerial) return;
        lastSerial = s.serial;

        if (! have || ! s.running || ! running)
        {
            anchorMs = s.wallMs;  anchorPpq = s.ppq;
        }
        else
        {
            const double predicted = anchorPpq + (s.wallMs - anchorMs) * bpm / 60000.0;
            const double err = s.ppq - predicted;
            if (std::fabs (err) > 0.5)                      // seek / loop / big tempo jump: snap
            {
                anchorMs = s.wallMs;  anchorPpq = s.ppq;
            }
            else                                            // jitter: pull gently towards the host
            {
                anchorMs = s.wallMs;  anchorPpq = predicted + 0.15 * err;
            }
        }
        bpm = s.bpm > 1.0 ? s.bpm : 120.0;
        barStart = s.barStartPpq;
        running = s.running;
        num = s.num > 0 ? s.num : 4;
        den = s.den > 0 ? s.den : 4;
        have = true;
    }

    BeatInfo evaluate (double nowMs, const ClockSettings& cs) const noexcept
    {
        BeatInfo b;
        if (! have) return b;

        const double t = nowMs + cs.offsetMs;
        const double ppqNow = running ? anchorPpq + (t - anchorMs) * bpm / 60000.0 : anchorPpq;

        const int n = cs.followHostMeter ? num : (cs.beatsPerBar > 0 ? cs.beatsPerBar : 4);
        const int d = cs.followHostMeter ? den : (cs.beatUnit > 0 ? cs.beatUnit : 4);
        const double beatLen = 4.0 / d;                     // in quarter notes
        const double origin = cs.followHostMeter ? barStart : 0.0;

        const double beatPos = (ppqNow - origin) / beatLen;
        const double fl = std::floor (beatPos);
        const auto counter = (int64_t) fl;
        const int idx = (int) (((counter % n) + n) % n);

        b.valid = true;
        b.running = running;
        b.ppq = ppqNow;
        b.bpm = bpm;
        b.beatPos = beatPos;
        b.beatCounter = counter;
        b.beatIndex = idx;
        b.beatsPerBar = n;
        b.phase = (float) (beatPos - fl);
        b.barPhase = (idx + b.phase) / (float) n;
        return b;
    }

private:
    bool have = false, running = false;
    uint32_t lastSerial = 0;
    double anchorMs = 0.0, anchorPpq = 0.0, bpm = 120.0, barStart = 0.0;
    int num = 4, den = 4;
};
