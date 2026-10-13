#include "PluginEditor.h"
#include "Animation.h"
#include "Presets.h"
#include <cmath>

//==============================================================================
// Look & feel
//==============================================================================
namespace
{
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel()
    {
        using juce::Colour;
        setColour (juce::ComboBox::backgroundColourId, Colour (0xff1b1530));
        setColour (juce::ComboBox::textColourId, juce::Colours::white);
        setColour (juce::ComboBox::outlineColourId, Colour (0x44ffffff));
        setColour (juce::ComboBox::arrowColourId, Colour (0xffff2bd6));
        setColour (juce::PopupMenu::backgroundColourId, Colour (0xf0140f24));
        setColour (juce::PopupMenu::textColourId, juce::Colours::white);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Colour (0xffff2bd6));
        setColour (juce::PopupMenu::headerTextColourId, Colour (0xff00e5ff));
        setColour (juce::Slider::trackColourId, Colour (0xffff2bd6));
        setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, Colour (0xffd8d0f0));
        setColour (juce::TextButton::buttonColourId, Colour (0xff3a1f66));
        setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        setColour (juce::ToggleButton::textColourId, Colour (0xffd8d0f0));
        setColour (juce::ToggleButton::tickColourId, Colour (0xff00e5ff));
        setColour (juce::ToggleButton::tickDisabledColourId, Colour (0x55ffffff));
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        const float cy = (float) y + (float) h * 0.5f, fx = (float) x, fw = (float) w;
        const float alpha = s.isEnabled() ? 1.f : 0.35f;
        g.setColour (juce::Colours::white.withAlpha (0.14f * alpha));
        g.fillRoundedRectangle (fx, cy - 2.f, fw, 4.f, 2.f);
        const auto col = s.findColour (juce::Slider::trackColourId).withAlpha (alpha);
        g.setColour (col);
        g.fillRoundedRectangle (fx, cy - 2.f, juce::jmax (0.f, pos - fx), 4.f, 2.f);
        g.setColour (col.withAlpha (0.25f * alpha));
        g.fillEllipse (pos - 9.f, cy - 9.f, 18.f, 18.f);
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.fillEllipse (pos - 5.f, cy - 5.f, 10.f, 10.f);
    }
};

float shapePulse (float phase, int style)
{
    const float p = bc::clamp01 (phase);
    switch (style)
    {
        case 1:  return std::pow (1.f - p, 5.f);                                       // punchy
        case 2:  return std::exp (-3.2f * p) * (0.5f + 0.5f * std::cos (p * 12.f));    // elastic
        case 3:  { const float q = std::floor (p * 4.f) / 4.f; return (1.f - q) * (1.f - q); }   // stepped
        default: return std::pow (1.f - p, 2.2f);                                      // smooth
    }
}
} // namespace

//==============================================================================
// Canvas: owns the clock + active animations, repaints on every display refresh
//==============================================================================
class AnimationCanvas : public juce::Component
{
public:
    explicit AnimationCanvas (BeatCanvasProcessor& p)
        : proc (p), vblank (this, [this] { tick(); })
    {
        setOpaque (true);
        setInterceptsMouseClicks (false, false);
        auto& a = proc.apvts;
        pIntensity = a.getRawParameterValue ("intensity");
        pSpeed = a.getRawParameterValue ("speed");
        pStyle = a.getRawParameterValue ("style");
        pPalette = a.getRawParameterValue ("palette");
        pOffset = a.getRawParameterValue ("offset");
        pFollow = a.getRawParameterValue ("followMeter");
        pBeats = a.getRawParameterValue ("beats");
        pUnit = a.getRawParameterValue ("unit");
        lastMs = juce::Time::getMillisecondCounterHiRes();
        syncAnimations();
    }

    void paint (juce::Graphics& g) override
    {
        const auto area = getLocalBounds().toFloat();
        g.fillAll (juce::Colours::black);
        if (animFirst == nullptr || animOther == nullptr) return;

        const auto& pals = getPalettes();
        const int pi = juce::jlimit (0, (int) pals.size() - 1, (int) pPalette->load());

        FrameContext ctx (g);
        ctx.area = area;
        ctx.pal = &pals[(size_t) pi];
        ctx.intensity = pIntensity->load();
        ctx.time = (float) std::fmod (animTime, 6000.0);
        ctx.dt = animDt;
        const int style = (int) pStyle->load();

        auto fill = [&] (const BeatInfo& b, float extraPhase)
        {
            ctx.beat = b;
            ctx.beat.phase = b.phase + extraPhase;
            ctx.pulse = shapePulse (ctx.beat.phase, style);
            ctx.accent = b.downbeat() ? 1.f : 0.f;
        };

        Animation* cur = pick (beat.downbeat());

        if (beat.beatCounter != lastBeatCounter)
        {
            prevAnim = (lastBeatCounter == std::numeric_limits<int64_t>::min()) ? nullptr : lastAnim;
            prevBeat = lastBeat;
            lastBeatCounter = beat.beatCounter;
            fill (beat, 0.f);
            cur->onBeat (ctx);
        }
        lastAnim = cur;
        lastBeat = beat;

        // short cross-fade from the previous beat's animation when the first-beat animation differs
        const float fade = bc::clamp01 (beat.phase / 0.3f);
        if (prevAnim != nullptr && prevAnim != cur && fade < 1.f)
        {
            fill (prevBeat, 1.f);
            prevAnim->paint (ctx);
            g.beginTransparencyLayer (fade);
            fill (beat, 0.f);
            cur->paint (ctx);
            g.endTransparencyLayer();
        }
        else
        {
            fill (beat, 0.f);
            cur->paint (ctx);
        }

        paintBeatDots (g, area, *ctx.pal);
    }

    void syncAnimations()
    {
        auto& reg = AnimationRegistry::get();
        auto ensure = [&] (bool first, juce::String& cachedId, std::unique_ptr<Animation>& slot)
        {
            auto id = proc.getAnimationId (first);
            if (reg.find (id) == nullptr) id = AnimationRegistry::defaultId (first);
            if (id != cachedId || slot == nullptr)
            {
                if (auto* e = reg.find (id)) slot = e->create();
                else if (! reg.all().empty()) slot = reg.all().front().create();
                cachedId = id;
                prevAnim = lastAnim = nullptr;
                lastBeatCounter = std::numeric_limits<int64_t>::min();
            }
        };
        ensure (true, idFirst, animFirst);
        ensure (false, idOther, animOther);
    }

private:
    Animation* pick (bool downbeat) { return (downbeat && idFirst != idOther) ? animFirst.get() : animOther.get(); }

    void tick()
    {
        const double now = juce::Time::getMillisecondCounterHiRes();
        const float rawDt = (float) juce::jlimit (0.0, 0.1, (now - lastMs) / 1000.0);
        lastMs = now;

        TransportSnapshot s;
        if (proc.mailbox.read (s)) clock.feed (s);

        ClockSettings cs;
        cs.offsetMs = pOffset->load();
        cs.followHostMeter = pFollow->load() > 0.5f;
        cs.beatsPerBar = (int) pBeats->load();
        cs.beatUnit = 1 << ((int) pUnit->load() + 1);
        beat = clock.evaluate (now, cs);

        animDt = rawDt * pSpeed->load();
        animTime += animDt;

        syncAnimations();
        repaint();
    }

    void paintBeatDots (juce::Graphics& g, juce::Rectangle<float> area, const Palette& pal) const
    {
        const int n = beat.beatsPerBar;
        const float r = 5.f, gap = 18.f;
        const float x0 = area.getCentreX() - (float) (n - 1) * gap * 0.5f, y = area.getBottom() - 20.f;
        for (int i = 0; i < n; ++i)
        {
            const bool on = beat.valid && i == beat.beatIndex;
            const auto c = i == 0 ? pal.a : pal.b;
            const float cx = x0 + (float) i * gap;
            g.setColour (c.withAlpha (on ? 1.f : 0.3f));
            if (on) g.fillEllipse (cx - r * 1.4f, y - r * 1.4f, r * 2.8f, r * 2.8f);
            else    g.drawEllipse (cx - r, y - r, r * 2.f, r * 2.f, 1.5f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.setFont (juce::FontOptions (12.f));
        g.drawText (juce::String (beat.bpm, 1) + " BPM", area.withTrimmedLeft (12.f).withHeight (area.getHeight() - 10.f).toNearestInt(),
                    juce::Justification::bottomLeft);
    }

    BeatCanvasProcessor& proc;
    BeatClock clock;
    BeatInfo beat, lastBeat, prevBeat;
    std::unique_ptr<Animation> animFirst, animOther;
    juce::String idFirst, idOther;
    Animation *lastAnim = nullptr, *prevAnim = nullptr;
    int64_t lastBeatCounter = std::numeric_limits<int64_t>::min();
    double lastMs = 0.0, animTime = 0.0;
    float animDt = 0.f;
    std::atomic<float> *pIntensity, *pSpeed, *pStyle, *pPalette, *pOffset, *pFollow, *pBeats, *pUnit;
    juce::VBlankAttachment vblank;                 // last member: stops before the rest is destroyed
};

//==============================================================================
// Control panel
//==============================================================================
class ControlPanel : public juce::Component, private juce::ValueTree::Listener
{
public:
    explicit ControlPanel (BeatCanvasProcessor& p) : proc (p)
    {
        auto& a = proc.apvts;

        addRow ("Preset", presetBox);
        addRow ("", saveBtn);
        addRow ("", deleteBtn);
        saveBtn.setButtonText ("Save preset...");
        deleteBtn.setButtonText ("Delete preset");
        presetBox.setTextWhenNothingSelected ("Choose a preset");
        presetBox.onChange = [this] { loadSelectedPreset(); };
        saveBtn.onClick = [this] { askSavePreset(); };
        deleteBtn.onClick = [this] { deleteSelectedPreset(); };
        refreshPresets();

        addRow ("Sync", modeBox);        fillChoice (modeBox, "mode");
        addRow ("BPM", bpmSlider);       setupSlider (bpmSlider);
        addRow ("Run when stopped", idleToggle);
        addRow ("First beat", firstBox); fillAnimCombo (firstBox);
        addRow ("Other beats", otherBox); fillAnimCombo (otherBox);
        addRow ("", randomBtn);
        addRow ("Palette", paletteBox);  fillChoice (paletteBox, "palette");
        addRow ("Style", styleBox);      fillChoice (styleBox, "style");
        addRow ("Intensity", intensitySlider); setupSlider (intensitySlider);
        addRow ("Speed", speedSlider);   setupSlider (speedSlider);
        addRow ("Visual offset", offsetSlider); setupSlider (offsetSlider);
        addRow ("Follow host meter", followToggle);
        addRow ("Beats per bar", beatsSlider); setupSlider (beatsSlider);
        addRow ("Beat unit", unitBox);   fillChoice (unitBox, "unit");
        addRow ("Audible click", clickToggle);
        addRow ("Click volume", clickVolSlider); setupSlider (clickVolSlider);

        idleToggle.setButtonText ("");  followToggle.setButtonText ("");  clickToggle.setButtonText ("");
        randomBtn.setButtonText ("Surprise me");
        randomBtn.onClick = [this] { randomise(); };

        aMode = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, "mode", modeBox);
        aBpm = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "bpm", bpmSlider);
        aIdle = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, "idleRun", idleToggle);
        aPalette = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, "palette", paletteBox);
        aStyle = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, "style", styleBox);
        aIntensity = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "intensity", intensitySlider);
        aSpeed = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "speed", speedSlider);
        aOffset = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "offset", offsetSlider);
        aFollow = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, "followMeter", followToggle);
        aBeats = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "beats", beatsSlider);
        aUnit = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (a, "unit", unitBox);
        aClick = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, "click", clickToggle);
        aClickVol = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, "clickVol", clickVolSlider);

        firstBox.onChange = [this] { choose (true, firstBox); };
        otherBox.onChange = [this] { choose (false, otherBox); };
        modeBox.onChange = [this] { updateVisibility(); };
        followToggle.onClick = [this] { updateVisibility(); };

        refreshAnimCombos();
        proc.apvts.state.addListener (this);
        updateVisibility();
    }

    ~ControlPanel() override { proc.apvts.state.removeListener (this); }

    int contentHeight() const
    {
        int h = 12;
        for (auto& r : rows) if (r.comp->isVisible()) h += rowH + gap;
        return h + 6;
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xe0100a1e));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 14.f);
        g.setColour (juce::Colour (0x33ffffff));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 14.f, 1.f);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced (12, 6);
        const int labelW = juce::jmin (120, b.getWidth() / 2 - 10);
        b.removeFromTop (6);
        for (auto& r : rows)
        {
            if (! r.comp->isVisible()) { r.label->setVisible (false); continue; }
            auto line = b.removeFromTop (rowH);
            b.removeFromTop (gap);
            r.label->setVisible (true);
            r.label->setBounds (line.removeFromLeft (labelW));
            r.comp->setBounds (line);
        }
    }

    std::function<void()> onLayoutChanged;

private:
    struct Row { std::unique_ptr<juce::Label> label; juce::Component* comp; };
    static constexpr int rowH = 26, gap = 6;

    void addRow (const juce::String& text, juce::Component& c)
    {
        auto l = std::make_unique<juce::Label> ("", text);
        l->setFont (juce::FontOptions (13.f));
        addAndMakeVisible (*l);
        addAndMakeVisible (c);
        rows.push_back ({ std::move (l), &c });
    }

    static void setupSlider (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::LinearHorizontal);
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 54, 20);
    }

    void fillChoice (juce::ComboBox& cb, const char* paramId)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramId)))
            cb.addItemList (p->choices, 1);
    }

    void fillAnimCombo (juce::ComboBox& cb)
    {
        ids.clear();
        juce::String lastCat;
        int itemId = 1;
        for (auto& e : AnimationRegistry::get().all())
        {
            if (e.category != lastCat) { cb.addSectionHeading (e.category); lastCat = e.category; }
            cb.addItem (e.name, itemId++);
            ids.push_back (e.id);
        }
    }

    int indexOfId (const juce::String& id) const
    {
        for (size_t i = 0; i < ids.size(); ++i) if (ids[i] == id) return (int) i;
        return -1;
    }

    void choose (bool first, juce::ComboBox& cb)
    {
        const int i = cb.getSelectedId() - 1;
        if (i >= 0 && i < (int) ids.size()) proc.setAnimationId (first, ids[(size_t) i]);
    }

    void refreshPresets (const juce::String& select = {})
    {
        presetBox.clear (juce::dontSendNotification);
        presetItems = Presets::list();
        bool factorySection = false, userSection = false;
        int selectId = 0;
        for (size_t i = 0; i < presetItems.size(); ++i)
        {
            const auto& it = presetItems[i];
            if (it.factory && ! factorySection) { presetBox.addSectionHeading ("Factory"); factorySection = true; }
            if (! it.factory && ! userSection)  { presetBox.addSectionHeading ("My presets"); userSection = true; }
            presetBox.addItem (it.name, (int) i + 1);
            if (it.name == select) selectId = (int) i + 1;
        }
        if (selectId > 0) presetBox.setSelectedId (selectId, juce::dontSendNotification);
    }

    void loadSelectedPreset()
    {
        const int i = presetBox.getSelectedId() - 1;
        if (i >= 0 && i < (int) presetItems.size()) Presets::load (proc, presetItems[(size_t) i]);
    }

    void askSavePreset()
    {
        auto* aw = new juce::AlertWindow ("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon);
        aw->addTextEditor ("name", presetBox.getText());
        aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        juce::Component::SafePointer<ControlPanel> safe (this);
        aw->enterModalState (true, juce::ModalCallbackFunction::create ([safe, aw] (int result)
        {
            std::unique_ptr<juce::AlertWindow> owner (aw);
            if (result != 1 || safe == nullptr) return;
            const auto name = aw->getTextEditorContents ("name").trim();
            if (Presets::save (safe->proc, name)) safe->refreshPresets (name);
        }), false);
    }

    void deleteSelectedPreset()
    {
        const int i = presetBox.getSelectedId() - 1;
        if (i < 0 || i >= (int) presetItems.size() || presetItems[(size_t) i].factory) return;   // factory presets are read-only
        Presets::remove (presetItems[(size_t) i].name);
        refreshPresets();
    }

    void refreshAnimCombos()
    {
        firstBox.setSelectedId (indexOfId (proc.getAnimationId (true)) + 1, juce::dontSendNotification);
        otherBox.setSelectedId (indexOfId (proc.getAnimationId (false)) + 1, juce::dontSendNotification);
    }

    void randomise()
    {
        if (ids.empty()) return;
        auto& r = juce::Random::getSystemRandom();
        proc.setAnimationId (true, ids[(size_t) r.nextInt ((int) ids.size())]);
        proc.setAnimationId (false, ids[(size_t) r.nextInt ((int) ids.size())]);
    }

    void updateVisibility()
    {
        const bool manual = modeBox.getSelectedId() == 2;
        bpmSlider.setEnabled (manual);
        idleToggle.setEnabled (! manual);
        const bool follow = followToggle.getToggleState();
        beatsSlider.setVisible (! follow);
        unitBox.setVisible (! follow);
        resized();
        if (onLayoutChanged) onLayoutChanged();
    }

    void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier& id) override
    {
        if (id == juce::Identifier ("animFirst") || id == juce::Identifier ("animOther"))
            refreshAnimCombos();
    }

    BeatCanvasProcessor& proc;
    std::vector<Row> rows;
    std::vector<juce::String> ids;

    juce::ComboBox modeBox, firstBox, otherBox, paletteBox, styleBox, unitBox;
    juce::Slider bpmSlider, intensitySlider, speedSlider, offsetSlider, beatsSlider, clickVolSlider;
    juce::ToggleButton idleToggle, followToggle, clickToggle;
    juce::TextButton randomBtn, saveBtn, deleteBtn;
    juce::ComboBox presetBox;
    std::vector<Presets::Item> presetItems;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<CA> aMode, aPalette, aStyle, aUnit;
    std::unique_ptr<SA> aBpm, aIntensity, aSpeed, aOffset, aBeats, aClickVol;
    std::unique_ptr<BA> aIdle, aFollow, aClick;
};

//==============================================================================
// Editor
//==============================================================================
BeatCanvasEditor::BeatCanvasEditor (BeatCanvasProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    laf = std::make_unique<NeonLookAndFeel>();
    setLookAndFeel (laf.get());

    canvas = std::make_unique<AnimationCanvas> (proc);
    panel = std::make_unique<ControlPanel> (proc);
    addAndMakeVisible (*canvas);

    viewport.setViewedComponent (panel.get(), false);
    viewport.setScrollBarsShown (true, false);
    addAndMakeVisible (viewport);

    gear.setClickingTogglesState (true);
    gear.setToggleState (true, juce::dontSendNotification);
    gear.onClick = [this] { viewport.setVisible (gear.getToggleState()); };
    addAndMakeVisible (gear);

    panel->onLayoutChanged = [this] { resized(); };

    setResizable (true, true);
    setResizeLimits (520, 340, 3000, 2000);
    const int w = (int) proc.apvts.state.getProperty ("w", 900);
    const int h = (int) proc.apvts.state.getProperty ("h", 560);
    setSize (juce::jlimit (520, 3000, w), juce::jlimit (340, 2000, h));
}

BeatCanvasEditor::~BeatCanvasEditor()
{
    setLookAndFeel (nullptr);
}

void BeatCanvasEditor::resized()
{
    auto b = getLocalBounds();
    canvas->setBounds (b);
    gear.setBounds (b.getRight() - 44, 8, 36, 28);

    const int pw = juce::jmin (320, b.getWidth() - 20);
    auto area = juce::Rectangle<int> (b.getRight() - pw - 8, 42, pw, b.getHeight() - 42 - 36);
    viewport.setBounds (area);
    const int ph = panel->contentHeight();
    panel->setSize (viewport.getMaximumVisibleWidth(), ph);

    proc.apvts.state.setProperty ("w", getWidth(), nullptr);
    proc.apvts.state.setProperty ("h", getHeight(), nullptr);
}
