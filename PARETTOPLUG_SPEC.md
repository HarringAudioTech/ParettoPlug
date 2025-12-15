# ParettoPlug Engineering Spec (Repo Root)

**Project name:** ParettoPlug  
**Goal:** An ultralight, zero-latency audio-FX suite for creative workflow in FL Studio (default 128-sample buffers) that intentionally prioritizes **audio-thread performance + deterministic behavior** over UI “eye candy”.

This repo is based on Jan Wilczek’s JUCE audio plugin template (CMake + CPM + GoogleTest + C++23).  citeturn0search0

---

## 1) Non-negotiables

### 1.1 Real-time / performance contract
All DSP must obey:

- **0 reported latency** (`getLatencySamples() == 0`)  
- **No heap allocations in `processBlock()`**
- **No locks/mutexes in `processBlock()`**
- **No logging/stdio/file IO in `processBlock()`**
- **No exceptions across audio thread**
- **Denormal-safe** (use `juce::ScopedNoDenormals` and/or flush-to-zero)
- **Stable CPU usage**: no per-sample mode switching; any branching should happen at **control rate** (microblock boundaries)

### 1.2 “Microblock compile” rule
Host block size may vary (FL often uses fixed 128, but do not rely on it). Internally:

- Define `MICROBLOCK = 32` samples.
- At each microblock boundary:
  - Read + smooth parameters
  - Query transport/tempo (if needed)
  - Resolve step boundaries / probability events
  - Fill small arrays (size `MICROBLOCK`) of modulation/destination values
- Inner loop applies those arrays with minimal branching (SIMD-friendly)

### 1.3 Plugin scope (v1)
This repo will ship **three separate audio FX plugins**, sharing a common DSP core:

1. **Parettoplug Gate** — rhythmic/trance gate + tone lane + A/B morph  
2. **Parettoplug Motion** — stripped-down “ultralight ShaperBox” movement generator  
3. **Parettoplug HyperVox** — hyperpop vocal character transformer (no true formant/pitch shifting)

Rationale: separate targets keep each plugin simple, predictable, and fast to load. Shared DSP core prevents duplicated work.

---

## 2) Repo structure & build expectations

### 2.1 Recommended structure
```
/src
  /common
    AudioMath.h/.cpp
    Smoothers.h
    Lfo.h
    EnvFollower.h
    SmoothRandom.h
    StepSequencer.h
    Svf.h
    SoftClipper.h
    DelayLine.h
    DspContext.h
  /gate
    GateProcessor.h/.cpp
    GateParameters.h
  /motion
    MotionProcessor.h/.cpp
    MotionParameters.h
  /hypervox
    HyperVoxProcessor.h/.cpp
    HyperVoxParameters.h
  /plugin
    PluginProcessor.h/.cpp
    PluginEditor.h/.cpp   (minimal UI only)
    PluginTargets.cmake    (if needed)

/tests
  /common
  /gate
  /motion
  /hypervox
  /data
    inputs/
    goldens/
/tools
  render_goldens.cpp (optional helper executable)
```

**Key rule:** Put *all* DSP logic in `/src/**/…Processor.*` and `/src/common`.  
`PluginProcessor` should be a thin wrapper around an effect engine class so tests can exercise DSP without instantiating JUCE plugin wrappers.

### 2.2 Toolchain assumptions (from template)
- CMake-based build, dependencies via CPM, unit tests via GoogleTest. citeturn0search0
- C++23, high warnings, warnings-as-errors.

---

## 3) Common DSP core: required modules

### 3.1 Basic types & conventions
- **Audio sample type:** `float` only for v1
- **Channel layout:** mono/stereo supported; other layouts can be rejected safely
- **dB conversions:** stable helper functions (avoid `powf` per sample; do per microblock or use fast approximations if needed)

### 3.2 Smoothing
Two smoothing mechanisms:

1. **Parameter smoothing (UI/automation)**: per parameter, updated at microblock start
   - Use one-pole smoother or linear ramp over the microblock
2. **Destination smoothing (modulated values)**: applied after combining modulators
   - Must prevent clicks when host automation jumps

Implementation requirement:
- A simple `OnePoleSmoother` for scalars
- A `Ramp32` helper to fill arrays with start/end values efficiently

### 3.3 Mod sources (Motion + Gate)
- `LFO`: phase accumulator, shapes: sine/tri/saw/pulse, optional “shape morph” 0..1  
- `EnvFollower`: rectify + attack/release one-pole → output 0..1  
- `SmoothRandom`: sample-and-hold at rate + one-pole smoothing → output -1..1  
- `StepSequencer`: steps (8/16/32) with swing + phase offset + per-step probability

### 3.4 Filters & nonlinearities
- `SVF` (state-variable filter) with stable behavior at resonance up to a capped value
- `SoftClipper`: at least 2 curves:
  - cubic soft clip
  - tanh-like (approximation acceptable)
  Curve selection occurs **at microblock** boundary (no per-sample switches).

### 3.5 Delay line
- Minimal delay line supporting:
  - integer sample taps (v1)
  - optional fractional via linear interpolation (v1.5)
- Must be denormal-safe and RT-safe.

### 3.6 Deterministic RNG
Used for probability decisions in Gate and random modulation in Motion:
- RNG seeded deterministically per run (configurable in tests)
- For host runs, seed derived from stable values (e.g., sampleRate + plugin instance pointer hash + an incrementing counter) — must not be cryptographic or expensive.

---

## 4) Plugin 1: Parettoplug Gate — Functional Spec

### 4.1 Intent
A rhythmic gate/chopper that’s **not Gross Beat**: it does **VCA gating + optional filter lane**, with **pattern A/B morph** and **groove templates**.

### 4.2 Signal flow
`Input → Gate VCA (lane A) → SVF filter (lane B) → (optional stereo offset) → Wet/Dry → Output`

### 4.3 Parameters (v1)
All parameter IDs must be stable strings (for preset compatibility).

**Timing**
- `syncEnabled` (bool, default true)
- `rate` (enum): {1/1..1/32 plus dotted + triplet}, default 1/8
- `rateHz` (float 0.1..20, default 2.0) used when `syncEnabled=false`
- `steps` (enum {8,16,32}, default 16)
- `swing` (float 0..0.70, default 0.0)
- `phaseDeg` (float 0..360, default 0)

**Gate lane**
- `depth` (float 0..1, default 1)
- `smoothMs` (float 0..50, default 5)
- `floorDb` (float -80..-12, default -60)  (gate “closed” level)
- `prob` (float 0..1, default 1)
- `accent` (float 0..1, default 0.3)

**Tone lane**
- `toneDepth` (float 0..1, default 0.0)
- `toneBaseHz` (float 80..12000, default 2000)
- `toneSpanOct` (float 0..5, default 2)
- `res` (float 0..0.8, default 0.2)

**Pattern**
- `morph` (float 0..1, default 0)
- `groove` (enum): {Straight, Swing, UKG, Jersey, DnB, RandomMicro}, default Straight
- `randomize` (trigger button; non-RT safe operation; must not run in audio thread)

**Mix**
- `mix` (float 0..1, default 1)
- `outputDb` (float -24..+12, default 0)

### 4.4 Pattern model
Two patterns (A and B), each with `steps` entries. Each step contains:
- `level` (0..1)
- `curve` (0..1) (linear→exp-ish shaping)
- `prob` (0..1)
- `accent` (0..1)

`morph` linearly interpolates A and B step values **before** applying probability.

### 4.5 Transport & timing
If `syncEnabled=true`:
- Use `AudioPlayHead::getPosition()`; derive samples-per-beat from BPM.
- If unavailable, fall back to internal phase accumulator using last known BPM or 120 BPM default.

Swing:
- Implement as shifting the *off-beat* step boundary within each pair by `swing` proportion.

### 4.6 Microblock processing
At microblock start:
1. Determine current step index + position within step
2. Resolve probability only when entering a new step:
   - `active = (rng.nextFloat() < stepProb * probGlobal)`
3. Compute gate envelope for each sample in microblock:
   - `gate = floorAmp + active * shaped(level, curve) * depth`
   - apply `smoothMs` as edge smoothing (ramp)
4. Compute cutoff for each sample:
   - `cutoff = toneBaseHz * 2^( toneSpanOct * toneDepth * shapedStepValue )`
   - clamp cutoff to [20 Hz, 0.45*sampleRate]
5. Process samples: multiply by gate, then SVF.

No per-sample branching beyond channel count and loop bounds.

---

## 5) Plugin 2: Parettoplug Motion — Functional Spec

### 5.1 Intent
A minimal “movement generator” (ultralight ShaperBox-like) with **mod sources + routing amounts** and a few taste-based “modes”. No curve drawing.

### 5.2 Signal flow
`Input → Gain mod → SVF cutoff mod → Drive/clip mod → Stereo (pan/width) mod → Wet/Dry → Output`

Any stage with depth 0 becomes effectively neutral without requiring explicit branches.

### 5.3 Parameters (v1)

**Macros**
- `motion` (float 0..1, default 0.5)
- `chaos` (float 0..1, default 0.0)
- `smoothMs` (float 0..200, default 20)
- `mode` (enum {Pulse, Drift, Pump, Wobble, Scatter}, default Drift)
- `mix` (float 0..1, default 1)
- `outputDb` (float -24..+12, default 0)

**Routing**
- `toGain` (bipolar float -1..+1, default 0)
- `toTone` (bipolar float -1..+1, default 0.5)
- `toGrit` (float 0..1, default 0.2)
- `toSpace` (bipolar float -1..+1, default 0.2)

**Sources (advanced)**
- `syncEnabled` (bool, default true)
- `lfoRate` (enum sync divisions OR float Hz; v1 choose enum only), default 1/4
- `lfoShape` (float 0..1, default 0.2)
- `envSense` (float 0..1, default 0.5)
- `rndRateHz` (float 0.05..10, default 0.3)

### 5.4 Mod sources
Compute arrays per microblock:
- `lfo[i]` in [-1,1]
- `env[i]` in [0,1], then remap to [-1,1] via `(env*2-1)`
- `rnd[i]` in [-1,1]

Combine (branchless):
- `mod[i] = sat( wL*lfo[i] + wE*env2[i] + wR*rnd[i] )`

Weights `w*` derive from `mode`, `motion`, `chaos`:
- `motion` scales overall depth
- `chaos` increases `wR` and speeds `rndRateHz` (within clamp)

### 5.5 Destination mapping
- Gain: map mod to a ±dB range (e.g., ±12 dB) then convert to amp per microblock
- Tone: map mod to cutoff multiplier in octaves around a base (choose defaults: base 1.5 kHz, span 3 oct)
- Grit: map mod to drive amount into clipper
- Space: map mod to pan and/or width (MS width in [0.5, 1.5])

All destination targets pass through `smoothMs` slew limiter.

---

## 6) Plugin 3: Parettoplug HyperVox — Functional Spec

### 6.1 Intent
A hyperpop vocal “character” transformer: **presence**, **controlled aggression**, **doubler/widen**, **de-ess clamp**. No heavy pitch/formant shifting.

### 6.2 Signal flow
`Pre HPF → De-ess GR → Presence EQ → Saturation/Clip → Doubler (microdelay/mod) → Width cap → Wet/Dry → Output`

### 6.3 Parameters (v1)

**Macros**
- `hype` (float 0..1, default 0.5)
- `cuteBrat` (bipolar float -1..+1, default 0)  (-1=cute, +1=brat)
- `air` (float 0..1, default 0.3)
- `doubles` (float 0..1, default 0.4)
- `sibilance` (float 0..1, default 0.5)
- `mix` (float 0..1, default 1)
- `outputDb` (float -24..+12, default 0)

**Advanced**
- `hpfHz` (float 40..200, default 80)
- `deEssFreqHz` (float 4000..9000, default 6500)
- `widthCap` (float 0.5..1.5, default 1.2)
- `trash` (float 0..1, default 0)  (optional “telephone/bitcrush” blend, v1 can omit)

### 6.4 De-ess algorithm
- Detection bandpass around `deEssFreqHz` (biquad)
- Envelope follower (fast attack, medium release)
- Gain reduction curve:
  - `g = 1 / (1 + k * env)` where `k` comes from `sibilance` and `air` coupling
- Apply to full-band signal (simple but effective)
- Must be stable and click-free under automation.

### 6.5 Presence EQ
Implement as fixed curves interpolated by `cuteBrat` and `hype`:
- “Cute”: smoother top, less 2–4k bite
- “Brat”: more 2–4k presence + slightly harder saturation
Implementation can be:
- tilt EQ + peaking filter + high shelf
Interpolate filter gains/centers at microblock rate with smoothing.

### 6.6 Doubler
Ultralight stereo doubler:
- 2 taps per channel, delay in 10–25 ms range
- small LFO modulation of delay time (depth controlled by `doubles`)
- mix amount increases with `doubles` and `hype`
No expensive resampling; integer delays are acceptable for v1 (with mild “grainy” character).

### 6.7 Width cap
Apply MS width with clamp to `widthCap` to avoid insane phase issues by default.

---

## 7) Automated testing spec (GoogleTest)

The template repo includes GoogleTest; tests must be runnable via `ctest`. citeturn0search0

### 7.1 Test layers
Implement 3 categories:

1. **Unit tests** (fast invariants, deterministic)
2. **Offline render regression** (golden outputs, tolerant compare)
3. **Performance/RT safety checks** (allocation + timing sanity)

### 7.2 Unit test requirements (all plugins)
For each plugin engine class (not JUCE wrapper):
- **No NaNs/Infs:** process impulses, silence, full-scale sine, and noise; assert finite outputs.
- **Neutral path:** with `mix=0` output == input within epsilon.
- **Reset determinism:** after `reset()`, processing same input with same seed yields same output.
- **Automation jump safety:** step key params at buffer boundaries; ensure max delta is bounded (see “click metric” below).
- **Latency:** engine reports 0 (or wrapper returns 0).

#### Click metric (simple, automated)
Given input `x[n]`, output `y[n]`, compute `d[n] = y[n] - y[n-1]`.
For a “safe” click-free transition test on broadband noise:
- Assert `max(|d[n]|) < 0.5` (tune threshold once you have real behavior)
- Also assert output RMS stays within a sane range (no explosions)

### 7.3 Plugin-specific unit tests
**Gate**
- **Step boundary correctness:** at fixed BPM/rate, verify computed step indices at known sample offsets.
- **Probability determinism:** fixed seed → identical active/inactive pattern across runs.
- **Floor behavior:** with `floorDb=-60`, closed gate never exceeds `floorAmp + small_epsilon`.

**Motion**
- **Bounds:** destination arrays remain clamped (cutoff in [20, 0.45*sr], width in [0.5, 1.5], gain within chosen dB limits).
- **Mode determinism:** `chaos=0` must be deterministic.

**HyperVox**
- **De-ess efficacy:** feed a synthetic signal with strong 6–8k component; increasing `sibilance` must reduce high-band energy (measure bandpassed RMS).
- **Stereo safety:** mono input at default settings should not flip correlation negative; assert correlation > -0.2 (tune as needed).

### 7.4 Offline render regression tests (goldens)
Store **small** test assets (1–3 seconds) in `/tests/data/inputs` and golden outputs in `/tests/data/goldens`.

Inputs (generate programmatically or store tiny wav):
- impulse (mono+stereo)
- sine @ 440 Hz
- sine sweep 20–20k
- pink-ish noise
- short “drum-like” synthetic (noise burst + decay)
- short “vocal-like” synthetic (formant-ish filter on saw + breath noise)

Golden scenarios (minimum):
- Gate: Straight groove, steps=16, morph=0/0.5/1, smooth=0/20ms
- Motion: each mode at motion=0.5 chaos=0 and chaos=0.5
- HyperVox: 3 preset settings (Clean Wide / Brat Crunch / Airy Cute)

Comparison:
- Compute RMS diff and max abs diff between new render and golden.
- Pass if:
  - `RMS_diff < 1e-4` AND `max_diff < 5e-4`
  - (or choose looser thresholds if platform differences appear; keep consistent per OS)

### 7.5 Performance / RT-safety tests
**Allocation test**
- In a dedicated test, process 10k blocks of audio and assert **0 allocations** occurred during processing.
Implementation suggestion:
- Provide a custom `new/delete` counter in test builds (link-time override) OR use platform allocation hooks. The test must fail if allocations occur inside `process()`.

**Timing sanity**
- Not a strict CI gate unless you control runners, but provide a test that measures wall time for N blocks and prints results.
Target guidance at 48kHz, stereo, 128 buffer (typical modern CPU):
- Gate: <0.2% of one core
- Motion: <0.2%
- HyperVox: <0.5%

---

## 8) Implementation milestones (agent-friendly)

### Milestone A — Core DSP library
- Implement common modules with unit tests:
  - smoothers, LFO, env follower, smooth random, SVF, clipper, delay
- Add denormal safety
- Add deterministic RNG

### Milestone B — Parettoplug Gate (first shippable)
- Implement Gate engine + parameters + minimal editor UI
- Add unit tests + 3 golden scenarios

### Milestone C — Parettoplug Motion
- Implement Motion engine + parameters + minimal editor UI
- Add tests + goldens

### Milestone D — Parettoplug HyperVox
- Implement HyperVox engine + parameters + minimal editor UI
- Add tests + goldens

---

## 9) Minimal UI guidelines (deliberately boring)
- Single panel, no heavy animation, no OpenGL.
- Knobs/sliders + dropdowns only.
- Presets can be deferred; if implemented, keep to JUCE ValueTreeState + simple menu.
- Provide “?” tooltips only if trivial.

---

## 10) Acceptance checklist (definition of “done”)
A plugin is accepted when:
- Builds in Debug/Release
- Runs in FL Studio without instability
- Reports 0 latency
- Passes unit tests + golden tests
- Allocation test passes (0 allocations in audio thread)
- No NaNs/Infs on stress inputs
- CPU remains stable under automation + extreme parameter values

---

## Appendix: Pseudocode skeleton (microblock)

```cpp
void Engine::process(AudioBuffer<float>& buf, const ProcessContext& ctx) {
  ScopedNoDenormals snd;
  const int N = buf.getNumSamples();
  for (int pos = 0; pos < N; pos += MICROBLOCK) {
    const int blockN = std::min(MICROBLOCK, N - pos);

    // 1) control-rate update
    readAndSmoothParams(blockN, ctx);
    updateTransport(blockN, ctx);
    compileModArrays(blockN, ctx); // fills arrays sized MICROBLOCK

    // 2) audio-rate apply
    for (int ch = 0; ch < buf.getNumChannels(); ++ch) {
      auto* x = buf.getWritePointer(ch) + pos;
      for (int i = 0; i < blockN; ++i) {
        float y = x[i];
        // apply gate/gain/filter/drive/etc using precomputed arrays
        x[i] = y;
      }
    }
  }
}
```

---

## Notes for the coding agent
- Keep per-plugin engine independent of JUCE so it can be tested directly.
- Do not optimize prematurely; first achieve correctness + deterministic tests, then micro-optimize.
- Any “randomize pattern” button must not execute inside audio thread; schedule it for message thread and apply at microblock boundary safely.
