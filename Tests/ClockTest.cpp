// Standalone test for the sync engine (no JUCE needed):  c++ -std=c++17 -I Source Tests/ClockTest.cpp -o ClockTest && ./ClockTest
#include "BeatClock.h"
#include <cstdio>
#include <random>

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf ("FAIL %s:%d  %s  ", __FILE__, __LINE__, #cond); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

// Simulates the audio thread (blocks arriving with timing jitter) feeding a UI that draws at 60 fps.
struct Sim
{
    TransportMailbox mb;
    BeatClock clock;
    std::mt19937 rng { 42 };
    double blockMs = 512.0 / 48000.0 * 1000.0;
    double bpm = 120.0, P0 = 0.0, T0 = 0.0;        // truth: P(T) = P0 + (T - T0) * bpm / 60000
    double jitterMs = 4.0;
    long k = 0;
    double nextWall = 0.0;

    double truth (double T) const { return P0 + (T - T0) * bpm / 60000.0; }
    void setBpm (double nb, double atT) { P0 = truth (atT); T0 = atT; bpm = nb; }

    // advance to wall time `now`, delivering every block whose (jittered) call time has passed
    void runUntil (double now, bool running = true)
    {
        for (;;)
        {
            const double nominal = (double) k * blockMs;
            std::uniform_real_distribution<double> j (-jitterMs, jitterMs);
            if (k == 0) nextWall = nominal;
            if (nextWall > now) break;
            TransportSnapshot s;
            s.ppq = truth (nominal);  s.bpm = bpm;  s.wallMs = nextWall;  s.running = running;
            s.barStartPpq = 0.0;  s.num = 4;  s.den = 4;
            mb.write (s);
            ++k;
            nextWall = (double) k * blockMs + j (rng);
        }
        TransportSnapshot s;
        if (mb.read (s)) clock.feed (s);
    }
};

static void testNoDriftWithJitter()
{
    Sim sim;
    ClockSettings cs;
    double maxErrMs = 0.0, sumErr = 0.0;  long cnt = 0;
    const double total = 20.0 * 60.0 * 1000.0;     // 20 minutes
    for (double now = 0.0; now < total; now += 1000.0 / 60.0)
    {
        sim.runUntil (now);
        const auto b = sim.clock.evaluate (now, cs);
        if (now > 1000.0)
        {
            const double errBeats = b.beatPos - sim.truth (now);
            const double errMs = std::fabs (errBeats) * 60000.0 / sim.bpm;
            maxErrMs = std::fmax (maxErrMs, errMs);
            sumErr += errMs;  ++cnt;
        }
    }
    std::printf ("jitter test: max err %.2f ms, mean err %.2f ms over 20 min\n", maxErrMs, sumErr / (double) cnt);
    CHECK (maxErrMs < 6.0, "max err %.3f", maxErrMs);
    CHECK (sumErr / (double) cnt < 3.0, "mean err %.3f", sumErr / (double) cnt);
}

static void testTempoChangeAndSeek()
{
    Sim sim;
    ClockSettings cs;
    double now = 0.0;
    for (; now < 5000.0; now += 1000.0 / 60.0) sim.runUntil (now);

    sim.setBpm (90.0, now);                                     // tempo change
    for (; now < 8000.0; now += 1000.0 / 60.0) sim.runUntil (now);
    double err = std::fabs (sim.clock.evaluate (now, cs).beatPos - sim.truth (now)) * 60000.0 / 90.0;
    CHECK (err < 8.0, "after tempo change: %.2f ms", err);

    sim.P0 += 64.0;                                             // locate forward 16 bars
    sim.runUntil (now + 20.0);
    now += 20.0;
    auto b = sim.clock.evaluate (now, cs);
    CHECK (std::fabs (b.beatPos - sim.truth (now)) < 0.05, "after seek: beatPos %.3f vs %.3f", b.beatPos, sim.truth (now));
}

static void testBarAndBeatIndices()
{
    BeatClock c;
    ClockSettings cs;
    TransportSnapshot s;
    s.ppq = 8.0;  s.bpm = 120.0;  s.barStartPpq = 8.0;  s.wallMs = 1000.0;  s.running = true;  s.num = 4;  s.den = 4;  s.serial = 1;
    c.feed (s);
    auto b = c.evaluate (1000.0, cs);
    CHECK (b.beatIndex == 0 && b.downbeat(), "downbeat at bar start (idx %d)", b.beatIndex);
    b = c.evaluate (1000.0 + 500.0 * 3 + 100.0, cs);            // 3 beats + a bit at 120 bpm
    CHECK (b.beatIndex == 3, "idx %d", b.beatIndex);
    CHECK (std::fabs (b.phase - 0.2f) < 0.01f, "phase %.3f", b.phase);
    b = c.evaluate (1000.0 + 2000.0 + 1.0, cs);                 // one full bar later -> downbeat again
    CHECK (b.beatIndex == 0, "next bar idx %d", b.beatIndex);

    // 6/8 follows the host: 6 beats of an eighth note (0.5 quarter) each
    s.num = 6;  s.den = 8;  s.serial = 2;  s.wallMs = 5000.0;  s.ppq = 16.0;  s.barStartPpq = 16.0;
    c.feed (s);
    b = c.evaluate (5000.0 + 250.0 * 4 + 10.0, cs);             // 120 bpm quarter = 500ms -> eighth = 250ms
    CHECK (b.beatsPerBar == 6 && b.beatIndex == 4, "6/8 idx %d of %d", b.beatIndex, b.beatsPerBar);

    // manual meter override
    cs.followHostMeter = false;  cs.beatsPerBar = 3;  cs.beatUnit = 4;
    b = c.evaluate (5000.0, cs);
    CHECK (b.beatsPerBar == 3, "override beats %d", b.beatsPerBar);

    // negative positions (count-in / pre-roll) must still map to valid indices
    BeatClock pre;
    s.ppq = -2.0;  s.barStartPpq = -4.0;  s.num = 4;  s.den = 4;  s.serial = 1;  s.wallMs = 0.0;
    pre.feed (s);
    cs = ClockSettings();
    b = pre.evaluate (0.0, cs);
    CHECK (b.beatIndex == 2, "pre-roll idx %d", b.beatIndex);
}

static void testStoppedTransportFreezes()
{
    BeatClock c;
    ClockSettings cs;
    TransportSnapshot s;
    s.ppq = 3.5;  s.running = false;  s.wallMs = 0.0;  s.serial = 1;
    c.feed (s);
    const auto a = c.evaluate (0.0, cs), b = c.evaluate (10000.0, cs);
    CHECK (std::fabs (a.beatPos - b.beatPos) < 1e-12, "stopped transport must not advance");
}

int main()
{
    testNoDriftWithJitter();
    testTempoChangeAndSeek();
    testBarAndBeatIndices();
    testStoppedTransportFreezes();
    if (failures == 0) std::printf ("ALL OK\n");
    return failures == 0 ? 0 : 1;
}
