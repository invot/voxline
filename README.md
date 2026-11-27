# voxline

A vocal chain in one JUCE plugin: tune, EQ, de-ess, compress, drive, move, delay,
reverb, width and limit. Seven macros cover the fast pass; the Detail view has the
whole chain. VST3 and standalone for Windows, built with JUCE 8.0.6.

## The two views

**Shape** is the default. Presets on the left, the seven macros across the top, a
live EQ curve and reverb/delay tail below them, and a clickable signal path at the
bottom. Click any stage in the signal path to jump straight to that module in
Detail.

**Detail** is the granular view. Every module has an on/off switch and all of its
parameters. Double-click a slider to reset it.

The header has the preset browser, previous/next, bypass and the input/output
meters. The footer has the momentary **THROW** button (holds the delay send open)
and the reported PDC.

## Signal flow

```
Tune -> EQ -> De-ess -> Comp 1 -> Comp 2 -> Drive -> Motion -> Delay -> Reverb -> Width -> Level -> Limiter
```

## Presets

| # | Preset | Character | Good for |
|---|--------|-----------|----------|
| 01 | Close / Intimate | dry, forward, detailed | solo verse, close mic, spoken word |
| 02 | Grand / Stadium | wide, long tail, ducked | big choruses, live, anthemic |
| 03 | Chorus Stack | layered doubles, wide, glued | stacked harmonies and ad-libs |
| 04 | Polished Pop | hard tuned, bright, throw | modern pop lead |
| 05 | Ethereal | washed, shimmering, soft | ambient vocals, intros, textures |
| 06 | Robotic | hard tuned, crushed, metallic | effect vocal, robot hook |
| 07 | Grunge | parallel grit, fast limit, raw | rock, punk, aggressive takes |
| 08 | Telephone | band-passed, mono, lo-fi | FX, skits, filtered verses |
| 09 | Rap / Front | tight, dry, present | rap lead, fast delivery |
| 10 | Air / Whisper | breathy, close, delicate | quiet, intimate, whisper layers |

Presets are starting points. The quickest useful moves are **Space** (how much
time effect sits around the voice) and **Tone** (how bright or dark the chain is).
The preset rail and the header both update when a host changes programs.

## Macros

| Macro | What it does |
|-------|--------------|
| **Press** | Shifts both compressor thresholds; more press = more density |
| **Tone** | Tilts the EQ and opens or closes the presence range |
| **Tune** | Correction amount, from 0% (free) to 100% (hard tuned) |
| **Grit** | Saturator drive, in parallel under the clean signal |
| **Width** | Mid/side spread; 0% mono, 100% natural, 200% wide |
| **Space** | Scales the delay and reverb sends |
| **Level** | Output trim before the limiter |

## Detail modules

- **Tune** — key, scale, retune speed, formant shift and a sub-octave layer. A YIN
  detector finds the note; the shifter is an exact time-stretch plus cubic
  resample, not a quantised granular shift.
- **EQ** — low cut, low/body/presence/air bands with frequency, gain and Q, plus a
  top roll-off. The Tone macro tilts the whole curve.
- **De-ess** — split-band sibilance control with frequency, threshold and maximum
  reduction.
- **Comp 1** — fast peak control, threshold/ratio/attack/release/mix/makeup.
- **Comp 2** — slow leveling, same controls; high ratio plus low mix is the
  parallel-smash move.
- **Drive** — tube, tape, fuzz or crush, with drive, tone, parallel mix and bit
  depth for the crush mode.
- **Motion** — chorus, flanger, doubler or microshift, with rate, depth, feedback,
  width and mix.
- **Delay** — free time or host sync (1/32 to 1/2, including dotted and triplet),
  feedback, tone, send, ping-pong and ducking. **THROW** momentarily opens the
  send from the footer.
- **Reverb** — eight-line FDN with size, pre-delay, damping, width, send, ducking,
  return high/low cut and shimmer.
- **Width** — mid/side spread; the same parameter as the Width macro.
- **Output** — trim, limiter on/off, ceiling and release.

## Latency

The tuner is always running so the latency is constant whether Tune is on, off or
the plugin is bypassed. At 48 kHz the chain reports **2734 samples (about 57 ms)**:
2048 samples of Signalsmith Stretch, 512 samples of FIFO priming, 102 samples of
buffering, and 72 samples of limiter lookahead. The host's delay compensation
handles it; the footer shows the reported PDC and the current sample rate.

The processed path's onset can appear a few samples later than the bypass impulse
because of the phase-vocoder smear; the reported PDC is the value to use.

## Build

The project uses the bundled toolchain next to the sibling `SightLine` checkout:

- JUCE 8.0.6 at `../SightLine/.tools/JUCE-8.0.6`
- MinGW GCC 16.2 and CMake 3.31.6 under `../SightLine/.tools`

```bash
./build.sh           # configure if needed, build the VST3 and standalone
./build.sh tests     # build and run the headless DSP checks
./build.sh smoke     # build and run the processor/editor smoke + snapshots
./build.sh clean     # remove build/ first
```

Outputs:

```
build/Voxline_artefacts/Release/VST3/voxline.vst3
build/Voxline_artefacts/Release/Standalone/voxline.exe
```

The build script sets the MinGW force-includes (JUCE 8 needs a UI Automation enum
that GCC 16's Windows headers omit) and the `SIGHTLINE_MANIFEST_RC` variable that
the vendored JUCE checkout uses to give its `juceaide` helper a Common Controls v6
manifest. Build through `build.sh`; a direct CMake configure will not set those.

If JUCE is somewhere else, pass `-DVX_JUCE_DIR=...` to CMake, or edit `VX_JUCE_DIR`
in `CMakeLists.txt`. If the toolchain moves, edit `TOOLS` in `build.sh`.

The smoke target writes `docs/screenshots/*.png` through
`VX_SNAPSHOT_DIR=...`; `build.sh smoke` sets it for you.

## Checks

- `VoxlineTests` covers the biquads, compressor, de-esser, limiter, delay,
  reverb tail, exact tuning, bypass latency, processed latency, and full-chain
  stability. All checks pass.
- `VoxlineSmoke` renders all ten presets with a synthetic vocal (finite, non-silent,
  under the ceiling), verifies program changes and saved/restored state, and renders
  both UI pages.
- The standalone was launched and stayed running; the editor snapshots were
  inspected at 1180x800.
- Not checked here: a real DAW host scan/load, audio device I/O, or sample rates
  other than 48 kHz. The VST3 is unsigned, so Windows may warn on first load.

## Layout

```
Source/core/        JUCE-free DSP: tune, EQ, dynamics, drive, motion, delay, FDN reverb, chain
Source/ui/          Theme, widgets, Shape page, Detail page
Source/              Parameters, presets, processor, editor
Tests/               main.cpp (DSP checks), smoke.cpp (plugin/editor checks)
third_party/         Signalsmith Stretch and Signalsmith Linear (MIT) + licenses
```

## Licensing

The plugin code is provided under the **AGPL-3.0**, matching the JUCE 8 AGPL
terms. A commercial JUCE licence can be used instead if you have one.

Vendored third-party code:

- Signalsmith Stretch, MIT, `third_party/signalsmith-stretch.h` and
  `third_party/signalsmith-stretch-LICENSE.txt`.
- Signalsmith Linear 0.6.6, MIT, `third_party/signalsmith-linear/` and
  `third_party/signalsmith-linear-LICENSE.txt`.
- JUCE 8.0.6, AGPL-3.0 or commercial; see the JUCE checkout for its licence.
