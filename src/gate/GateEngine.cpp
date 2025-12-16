#include "GateEngine.h"

#include <algorithm>
#include <cstddef>

#include "../common/AudioMath.h"

namespace paretto {
namespace gate {

void GateEngine::prepare(double sampleRate, std::uint32_t seed) {
  sampleRate_ = sampleRate;
  internalSamplePos_ = 0;

  sequencer_.prepare(sampleRate);

  seed_ = seed;
  rng_.seed(seed_);

  gateSmooth_.prepare(sampleRate);
  gateSmooth_.reset(0.0f);

  ch_[0].filter.prepare(sampleRate);
  ch_[1].filter.prepare(sampleRate);

  reset();
}

void GateEngine::reset() {
  internalSamplePos_ = 0;
  lastStepIndex_ = -1;
  stepActive_ = true;

  rng_.seed(seed_);

  gateSmooth_.reset(0.0f);

  ch_[0].filter.reset();
  ch_[1].filter.reset();

  updateGrooveDerived();
}

void GateEngine::setSeed(std::uint32_t seed) {
  seed_ = seed;
  rng_.seed(seed_);
}

void GateEngine::setParameters(const GateParameters& p) {
  params_ = p;
  updateGrooveDerived();
}

int GateEngine::stepsCount() const {
  return static_cast<int>(params_.steps);
}

void GateEngine::updateGrooveDerived() {
  grooveSwing_ = params_.swing;

  switch (params_.groove) {
    case Groove::Straight: grooveSwing_ = 0.0f; break;
    case Groove::Swing: grooveSwing_ = params_.swing; break;
    case Groove::UKG: grooveSwing_ = std::max(params_.swing, 0.25f); break;
    case Groove::Jersey: grooveSwing_ = std::max(params_.swing, 0.15f); break;
    case Groove::DnB: grooveSwing_ = std::max(params_.swing, 0.1f); break;
    case Groove::RandomMicro: grooveSwing_ = params_.swing; break;
  }
}

void GateEngine::process(float* const* channels, int numChannels, int numSamples, const paretto::dsp::ProcessContext& ctx) {
  paretto::dsp::ScopedFlushToZero ftz;

  const float floorAmp = paretto::dsp::dbToAmp(paretto::dsp::clamp(params_.floorDb, -120.0f, 0.0f));
  const float outAmp = paretto::dsp::dbToAmp(paretto::dsp::clamp(params_.outputDb, -60.0f, 24.0f));
  const float mix = paretto::dsp::clamp(params_.mix, 0.0f, 1.0f);

  gateSmooth_.setTimeMs(paretto::dsp::clamp(params_.smoothMs, 0.0f, 200.0f));

  const int steps = stepsCount();

  double bpm = 120.0;
  std::int64_t baseSamplePos = internalSamplePos_;
  if (ctx.transport.valid) {
    bpm = (ctx.transport.bpm > 1.0) ? ctx.transport.bpm : 120.0;
    if (ctx.transport.timeInSamples != 0) {
      baseSamplePos = ctx.transport.timeInSamples;
    }
  }

  sequencer_.setBpm(bpm);
  sequencer_.setTiming(params_.syncEnabled, params_.rate, params_.rateHz, steps, grooveSwing_, params_.phaseDeg);

  for (int pos = 0; pos < numSamples; pos += paretto::dsp::MicroblockSize) {
    const int blockN = std::min(paretto::dsp::MicroblockSize, numSamples - pos);

    int jitterSamples = 0;
    if (params_.groove == Groove::RandomMicro) {
      const float r = rng_.nextFloat11();
      jitterSamples = static_cast<int>(r * 2.0f);
    }

    const auto stepPos = sequencer_.getPositionAt(baseSamplePos + pos + jitterSamples);
    const int stepIdx = stepPos.stepIndex;

    if (stepIdx != lastStepIndex_) {
      const auto& a = params_.patternA.steps[static_cast<std::size_t>(stepIdx)];
      const auto& b = params_.patternB.steps[static_cast<std::size_t>(stepIdx)];
      const float stepProb = paretto::dsp::lerp(a.prob, b.prob, paretto::dsp::clamp(params_.morph, 0.0f, 1.0f));
      const float p = paretto::dsp::clamp(stepProb * paretto::dsp::clamp(params_.prob, 0.0f, 1.0f), 0.0f, 1.0f);
      stepActive_ = rng_.nextFloat01() < p;
      lastStepIndex_ = stepIdx;
    }

    const auto& a = params_.patternA.steps[static_cast<std::size_t>(stepIdx)];
    const auto& b = params_.patternB.steps[static_cast<std::size_t>(stepIdx)];

    const float morph = paretto::dsp::clamp(params_.morph, 0.0f, 1.0f);
    const float level = paretto::dsp::lerp(a.level, b.level, morph);
    const float curve = paretto::dsp::lerp(a.curve, b.curve, morph);
    const float accent = paretto::dsp::lerp(a.accent, b.accent, morph);

    const float shaped = paretto::dsp::shape01(level, curve);
    const float accentGain = 1.0f + paretto::dsp::clamp(params_.accent, 0.0f, 1.0f) * paretto::dsp::clamp(accent, 0.0f, 1.0f);
    const float gateLevel = paretto::dsp::clamp(shaped * accentGain, 0.0f, 1.0f);

    const float depth = paretto::dsp::clamp(params_.depth, 0.0f, 1.0f);
    const float active = stepActive_ ? 1.0f : 0.0f;
    const float gateTarget = floorAmp + active * gateLevel * depth;

    const float toneDepth = paretto::dsp::clamp(params_.toneDepth, 0.0f, 1.0f);
    const bool toneEnabled = toneDepth > 0.0f;
    const float toneSpan = paretto::dsp::clamp(params_.toneSpanOct, 0.0f, 8.0f);
    const float baseHz = paretto::dsp::clamp(params_.toneBaseHz, 20.0f, static_cast<float>(0.45 * sampleRate_));

    const float toneMod = shaped;
    if (toneEnabled) {
      const float cutoff = paretto::dsp::clamp(baseHz * paretto::dsp::pow2(toneSpan * toneDepth * toneMod), 20.0f,
                                               static_cast<float>(0.45 * sampleRate_));
      const float res = paretto::dsp::clamp(params_.res, 0.0f, 0.99f);
      if (numChannels > 0) {
        ch_[0].filter.setResonance(res);
        ch_[0].filter.setCutoffHz(cutoff);
      }
      if (numChannels > 1) {
        ch_[1].filter.setResonance(res);
        ch_[1].filter.setCutoffHz(cutoff);
      }
    }

    for (int ch = 0; ch < numChannels; ++ch) {
      auto* x = channels[ch] + pos;
      for (int i = 0; i < blockN; ++i) {
        const float in = x[i];

        const float g = gateSmooth_.process(gateTarget);
        float y = in * g;

        if (toneEnabled && ch < 2) {
          y = ch_[ch].filter.processSample(y);
          if (!paretto::dsp::isFinite(y)) {
            y = 0.0f;
          }
        }

        const float wet = y * outAmp;
        x[i] = paretto::dsp::lerp(in, wet, mix);
      }
    }
  }

  internalSamplePos_ += numSamples;
}

}  // namespace gate
}  // namespace paretto
