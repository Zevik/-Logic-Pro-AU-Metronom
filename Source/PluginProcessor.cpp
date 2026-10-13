#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Palette.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout BeatCanvasProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto pid = [] (const char* s) { return ParameterID { s, 1 }; };

    l.add (std::make_unique<AudioParameterChoice> (pid ("mode"), "Sync Mode", StringArray { "Host Sync", "Manual BPM" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (pid ("bpm"), "Manual BPM", NormalisableRange<float> (20.f, 300.f, 0.01f, 0.6f), 120.f));
    l.add (std::make_unique<AudioParameterBool> (pid ("idleRun"), "Run When Stopped", true));
    l.add (std::make_unique<AudioParameterBool> (pid ("followMeter"), "Follow Host Meter", true));
    l.add (std::make_unique<AudioParameterInt> (pid ("beats"), "Beats Per Bar", 1, 16, 4));
    l.add (std::make_unique<AudioParameterChoice> (pid ("unit"), "Beat Unit", StringArray { "1/2", "1/4", "1/8", "1/16" }, 1));

    l.add (std::make_unique<AudioParameterChoice> (pid ("palette"), "Palette", getPaletteNames(), 0));
    l.add (std::make_unique<AudioParameterChoice> (pid ("style"), "Style", StringArray { "Smooth", "Punchy", "Elastic", "Stepped" }, 0));
    l.add (std::make_unique<AudioParameterFloat> (pid ("intensity"), "Intensity", NormalisableRange<float> (0.f, 1.f, 0.001f), 0.7f));
    l.add (std::make_unique<AudioParameterFloat> (pid ("speed"), "Speed", NormalisableRange<float> (0.25f, 4.f, 0.01f, 0.5f), 1.f));
    l.add (std::make_unique<AudioParameterFloat> (pid ("offset"), "Visual Offset (ms)", NormalisableRange<float> (-250.f, 250.f, 1.f), 0.f));

    l.add (std::make_unique<AudioParameterBool> (pid ("click"), "Audible Click", false));
    l.add (std::make_unique<AudioParameterFloat> (pid ("clickVol"), "Click Volume", NormalisableRange<float> (0.f, 1.f, 0.001f), 0.5f));
    return l;
}

BeatCanvasProcessor::BeatCanvasProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BEATCANVAS", createLayout())
{
    pMode = apvts.getRawParameterValue ("mode");
    pBpm = apvts.getRawParameterValue ("bpm");
    pClick = apvts.getRawParameterValue ("click");
    pClickVol = apvts.getRawParameterValue ("clickVol");
    pFollow = apvts.getRawParameterValue ("followMeter");
    pBeats = apvts.getRawParameterValue ("beats");
    pUnit = apvts.getRawParameterValue ("unit");
    pIdle = apvts.getRawParameterValue ("idleRun");
}

bool BeatCanvasProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == out;
}

void BeatCanvasProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    clickActive = false;
    lastClickBeat = std::numeric_limits<int64_t>::min();
}

void BeatCanvasProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const double nowMs = juce::Time::getMillisecondCounterHiRes();

    // ---- what does the host say? ----
    bool hostPlaying = false;
    juce::Optional<double> hostPpq, hostBpm, hostBarStart;
    int hostNum = 0, hostDen = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            hostPlaying = pos->getIsPlaying();
            hostPpq = pos->getPpqPosition();
            hostBpm = pos->getBpm();
            hostBarStart = pos->getPpqPositionOfLastBarStart();
            if (auto ts = pos->getTimeSignature()) { hostNum = ts->numerator; hostDen = ts->denominator; }
        }

    // ---- effective transport (single source of truth for visuals AND click) ----
    const bool hostMode = pMode->load() < 0.5f;
    const double paramBpm = (double) pBpm->load();
    double bpm = paramBpm, ppq = freeRunPpq, barStart = 0.0;
    bool running = true;

    if (hostMode)
    {
        if (hostBpm.hasValue() && *hostBpm > 1.0) bpm = *hostBpm;
        if (hostPlaying && hostPpq.hasValue())
        {
            ppq = *hostPpq;
            barStart = hostBarStart.hasValue() ? *hostBarStart : 0.0;
        }
        else if (pIdle->load() < 0.5f)
        {
            running = false;
            if (hostPpq.hasValue()) ppq = *hostPpq;
            if (hostBarStart.hasValue()) barStart = *hostBarStart;
        }
        // else: free-run from the last position, at the host tempo
    }

    const double qps = bpm / 60.0 / sampleRate;            // quarter notes per sample
    if (running) freeRunPpq = ppq + n * qps;               // keeps continuity when the host starts/stops

    TransportSnapshot snap;
    snap.ppq = ppq;  snap.bpm = bpm;  snap.barStartPpq = barStart;  snap.wallMs = nowMs;  snap.running = running;
    snap.num = hostNum > 0 ? hostNum : 4;  snap.den = hostDen > 0 ? hostDen : 4;
    mailbox.write (snap);

    // ---- optional sample-accurate click ----
    const bool clickOn = pClick->load() > 0.5f && running && (! hostMode || hostPlaying);
    if (clickOn)
    {
        const bool follow = pFollow->load() > 0.5f;
        const int beatsPerBar = follow ? snap.num : juce::jmax (1, (int) pBeats->load());
        const int unitDen = follow ? snap.den : (1 << ((int) pUnit->load() + 1));
        const double beatLen = 4.0 / unitDen;
        const double origin = follow ? barStart : 0.0;

        const double beatPos0 = (ppq - origin) / beatLen;
        const double k = std::ceil (beatPos0 - 1e-9);
        const double offsetSamples = (k - beatPos0) * beatLen / qps;
        const auto counter = (int64_t) k;
        if (offsetSamples < n && counter != lastClickBeat)
        {
            lastClickBeat = counter;
            const int idx = (int) (((counter % beatsPerBar) + beatsPerBar) % beatsPerBar);
            clickActive = true;
            clickPos = 0;
            clickDelay = (int) juce::jmax (0.0, offsetSamples);
            clickFreq = idx == 0 ? 1600.f : 1000.f;
            clickGain = idx == 0 ? 1.f : 0.7f;
        }
    }
    else
    {
        lastClickBeat = std::numeric_limits<int64_t>::min();
    }

    if (clickActive)
    {
        const float vol = pClickVol->load();
        const int len = (int) (0.06 * sampleRate);
        const int chans = buffer.getNumChannels();
        const float decay = (float) (1.0 / (0.012 * sampleRate));
        for (int i = 0; i < n; ++i)
        {
            if (clickDelay > 0) { --clickDelay; continue; }    // wait for the exact sample
            const float s = std::sin (juce::MathConstants<float>::twoPi * clickFreq * (float) clickPos / (float) sampleRate)
                            * std::exp (-(float) clickPos * decay) * clickGain * vol;
            for (int c = 0; c < chans; ++c) buffer.getWritePointer (c)[i] += s;
            if (++clickPos >= len) { clickActive = false; break; }
        }
    }
    // audio otherwise passes through untouched
}

juce::AudioProcessorEditor* BeatCanvasProcessor::createEditor() { return new BeatCanvasEditor (*this); }

juce::String BeatCanvasProcessor::getAnimationId (bool firstBeat) const
{
    return apvts.state.getProperty (firstBeat ? "animFirst" : "animOther", firstBeat ? "supernova" : "heart").toString();
}

void BeatCanvasProcessor::setAnimationId (bool firstBeat, const juce::String& id)
{
    apvts.state.setProperty (firstBeat ? "animFirst" : "animOther", id, nullptr);
}

void BeatCanvasProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void BeatCanvasProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BeatCanvasProcessor(); }
