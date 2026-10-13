# BeatCanvas – CLAUDE.md

A visual metronome **Audio Unit** for Logic Pro (macOS), built with **JUCE 8 / C++17**. It passes audio through
untouched and renders beat-synced animations. Also builds as VST3 + Standalone (that is how we iterate on Windows).
The owner (Zevik) writes in Hebrew; reply in Hebrew, keep code, comments and commit messages in English.

## Commands

```bash
# macOS: build universal AU, run sync test, install to ~/Library/Audio/Plug-Ins/Components, run auval
./scripts/build_install_mac.sh

# Windows (dev only – AU does not exist here). Refresh PATH first if tools were just installed.
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target BeatCanvas_Standalone BeatCanvas_VST3 ClockTest --parallel
.\build\Release\ClockTest.exe                                       # sync-engine test, no JUCE needed
.\build\BeatCanvas_artefacts\Release\Standalone\BeatCanvas.exe      # run the plugin UI
```

CI (`.github/workflows/build-macos.yml`) builds on macOS, runs `auval -v aufx Bcnv Zvkd` and uploads
`BeatCanvas-macOS.zip`. **It is the only place the AU is actually validated** – check it after changes that touch
processor, parameters, bus layout or CMake.

## Architecture (data flow)

```
Logic playhead ─▶ PluginProcessor::processBlock (audio thread)
                    └─ computes "effective transport" {ppq, bpm, barStart, running}
                    └─ TransportMailbox.write()   (lock-free seqlock, no allocs, no locks)
                    └─ optional sample-accurate click
UI thread (VBlankAttachment, ~60 fps) ─▶ AnimationCanvas::tick
                    └─ mailbox.read() ─▶ BeatClock.feed()/evaluate()  ─▶ BeatInfo ─▶ Animation::paint(FrameContext)
```

- `Source/BeatClock.h` – **pure C++, no JUCE** (so `Tests/ClockTest.cpp` can run anywhere). The UI never counts beats
  itself: it extrapolates from the latest host anchor and PLL-corrects (15 %) towards each new one; errors > 0.5 quarter
  notes (seek/loop) snap. This is what guarantees no cumulative drift. Do not replace it with a UI-side timer counter.
- The processor is the single source of truth for both visuals and click. In Manual mode and when the host is stopped
  (with "Run when stopped") it free-runs a `freeRunPpq` counter and publishes it like a host position.
- Beat unit: `ppq` is in quarter notes; a beat lasts `4/denominator` quarters. Bar origin = host `barStartPpq` when
  following the host meter, else 0.
- `Source/Animation.h` – `Animation` base class, `FrameContext`, registry + `BC_REGISTER_ANIMATION`.
- `Source/Animations/*.h` – the library, one animation per header. **CMake globs this folder and generates
  `build/generated/AnimationIndex.inc`, included by `AnimationRegistry.cpp`.** Re-run CMake configure after adding a file.
- `Source/PluginEditor.cpp` – `AnimationCanvas` (clock, cross-fade between first-beat/other-beat animations, beat dots),
  `ControlPanel` (APVTS attachments, presets UI), `NeonLookAndFeel`.
- `Source/Presets.*` – factory presets (in code) + user presets as `.bcpreset` XML in `Documents/BeatCanvas/Presets`.

## Rules that must not be broken

- **No allocation, locking, logging or file I/O in `processBlock`.** The mailbox is the only channel to the UI.
- **Stable IDs.** Parameter IDs (`createLayout`), animation IDs (`BC_REGISTER_ANIMATION` first arg) and the AU codes
  (`PLUGIN_MANUFACTURER_CODE Zvkd`, `PLUGIN_CODE Bcnv`) are baked into saved Logic projects and presets. Renaming or
  reordering them breaks users' sessions. Add new ones; never repurpose old ones. Animation choice is stored as a
  string id in `apvts.state` ("animFirst"/"animOther") precisely so adding animations is safe.
- **Do not put animation registrars in a `.cpp` that is only linked through the plugin's static `SharedCode` library** –
  the linker drops unreferenced objects and the animation silently disappears. That is why animations are headers
  included from `AnimationRegistry.cpp`.
- Animations: treat `FrameContext` as read-only, use `ctx.dt`/`ctx.time` (already scaled by Speed) rather than wall
  clocks, use `ctx.pulse`/`ctx.accent`/`ctx.beat` for sync, and use palette colours (`col(i)`, `grad(t)`) – never
  hard-code a colour scheme. `beat.phase` can exceed 1 while an animation is the outgoing side of a cross-fade.
  Keep per-frame cost low (no per-frame heap churn in hot loops; cap particle counts via `ParticleSystem` capacity).
- Rendering stays on `juce::Graphics` (no OpenGL: deprecated on macOS and flaky inside plugin windows).

## Verification workflow

1. `ClockTest` must print `ALL OK` after any change to `BeatClock.h` (it simulates 20 min of jittered blocks,
   tempo change, seek, 6/8, pre-roll, stopped transport).
2. Build `BeatCanvas_Standalone` and look at it. To screenshot **only the plugin window** (never the whole screen –
   it can contain the user's private windows) use `PrintWindow` on the process's `MainWindowHandle` and save into the
   scratchpad, not the repo.
3. Push and check the macOS workflow (`auval` is the real gate for Logic compatibility).
4. Never claim AU/Logic behaviour is verified from Windows; say what was and wasn't run.

## Environment gotchas

- Windows has no AU. `COPY_PLUGIN_AFTER_BUILD` is macOS-only because copying VST3 to `Program Files` needs admin.
- After `winget install`, the current shell does not see new PATH entries – reload `Machine`+`User` PATH before cmake.
- First configure downloads JUCE 8.0.4 via FetchContent (~90 s).
- Unsigned/ad-hoc-signed AU shared with friends needs `xattr -cr BeatCanvas.component` (see README).
- JUCE is AGPLv3/commercial: fine for personal/open-source use; closed commercial distribution needs a JUCE licence.

## Roadmap ideas (not built yet)
Audio-level reactivity, auto-cycle animations every N bars, more animations (lightning, birds, psychedelic space),
UI auto-hiding panel at small window sizes.
