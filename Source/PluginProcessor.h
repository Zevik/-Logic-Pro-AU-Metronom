#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "BeatClock.h"

class BeatCanvasProcessor : public juce::AudioProcessor
{
public:
    BeatCanvasProcessor();
    ~BeatCanvasProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BeatCanvas"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Animation choice is stored by string id (not index) so presets survive adding new animations.
    juce::String getAnimationId (bool firstBeat) const;
    void setAnimationId (bool firstBeat, const juce::String& id);

    juce::AudioProcessorValueTreeState apvts;
    TransportMailbox mailbox;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float> *pMode, *pBpm, *pClick, *pClickVol, *pFollow, *pBeats, *pUnit, *pIdle;

    double sampleRate = 44100.0;
    double freeRunPpq = 0.0;
    int64_t lastClickBeat = std::numeric_limits<int64_t>::min();

    // click voice
    bool clickActive = false;
    int clickPos = 0, clickDelay = 0;
    float clickFreq = 1000.f, clickGain = 1.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatCanvasProcessor)
};
