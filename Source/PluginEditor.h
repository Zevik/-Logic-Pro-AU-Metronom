#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class AnimationCanvas;
class ControlPanel;

class BeatCanvasEditor : public juce::AudioProcessorEditor
{
public:
    explicit BeatCanvasEditor (BeatCanvasProcessor&);
    ~BeatCanvasEditor() override;

    void resized() override;
    void paint (juce::Graphics&) override {}

private:
    BeatCanvasProcessor& proc;
    std::unique_ptr<juce::LookAndFeel_V4> laf;     // declared first -> destroyed last
    std::unique_ptr<AnimationCanvas> canvas;
    std::unique_ptr<ControlPanel> panel;
    juce::Viewport viewport;
    juce::TextButton gear { "..." };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BeatCanvasEditor)
};
