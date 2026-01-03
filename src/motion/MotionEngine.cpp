#include "motion/MotionEngine.h"

#include <algorithm>
#include <cmath>

#include "common/AudioMath.h"
#include "common/Constants.h"
#include "common/Denormals.h"
#include "common/EnvFollower.h"
#include "common/Lfo.h"
#include "common/SmoothRandom.h"
#include "common/SoftClipper.h"
#include "common/Smoothers.h"
#include "common/Svf.h"

namespace paretto {
namespace motion {
namespace {

inline float clamp11(float x) { return paretto::dsp::clamp(x, -1.0f, 1.0f); }

inline float panGainL(float pan) {
  const float p = paretto::dsp::clamp(pan, -1.0f, 1.0f);
  return std::sqrt(0.5f * (1.0f - p));
}

inline float panGainR(float pan) {
  const float p = paretto::dsp::clamp(pan, -1.0f, 1.0f);
  return std::sqrt(0.5f * (1.0f + p));
}

inline paretto::dsp::LfoShape shapeFrom01(float v01) {
  const float x = paretto::dsp::clamp(v01, 0.0f, 1.0f);
  if (x < 0.25f) {
    return paretto::dsp::LfoShape::Sine;
  }
  if (x < 0.50f) {
    return paretto::dsp::LfoShape::Tri;
  }
  if (x < 0.75f) {
    return paretto::dsp::LfoShape::Saw;
  }
  return paretto::dsp::LfoShape::Pulse;
}

inline void weightsForMode(MotionMode mode, float& wL, float& wE, float& wR) {
  switch (mode) {
    case MotionMode::Pulse:
      wL = 1.0f;
      wE = 0.15f;
      wR = 0.10f;
      return;
    case MotionMode::Drift:
      wL = 0.75f;
      wE = 0.25f;
      wR = 0.25f;
      return;
    case MotionMode::Pump:
      wL = 0.25f;
      wE = 1.0f;
      wR = 0.15f;
      return;
    case MotionMode::Wobble:
      wL = 1.0f;
      wE = 0.15f;
      wR = 0.30f;
      return;
    case MotionMode::Scatter:
      wL = 0.20f;
      wE = 0.20f;
      wR = 1.0f;
      return;
  }

  wL = 0.75f;
  wE = 0.25f;
  wR = 0.25f;
}

}  // namespace

void MotionEngine::prepare(double sampleRate, int /*maxBlockSize*/) {
  sampleRate_ = (sampleRate > 0.0) ? sampleRate : 48000.0;

  lfo_.prepare(sampleRate_);
  env_.prepare(sampleRate_);
  env_.setAttackReleaseMs(10.0f, 100.0f);

  rnd_.prepare(sampleRate_);

  svfL_.prepare(sampleRate_);
  svfR_.prepare(sampleRate_);

  gainAmp_.prepare(sampleRate_);
  cutoffHz_.prepare(sampleRate_);
  drive_.prepare(sampleRate_);
  pan_.prepare(sampleRate_);
  width_.prepare(sampleRate_);

  reset();
}

void MotionEngine::reset() {
  lfo_.reset(0.0f);
  env_.reset();

  rnd_.seed(seed_);
  rnd_.reset(0.0f);

  svfL_.reset();
  svfR_.reset();

  gainAmp_.reset(1.0f);
  cutoffHz_.reset(1500.0f);
  drive_.reset(1.0f);
  pan_.reset(0.0f);
  width_.reset(1.0f);
}

void MotionEngine::setSeed(std::uint32_t seed) {
  seed_ = seed;
  rnd_.seed(seed_);
}

void MotionEngine::setParameters(const MotionParameters& p) { params_ = p; }

void MotionEngine::process(float** channels, int numChannels, int numSamples, const paretto::dsp::ProcessContext& ctx) {
  paretto::dsp::ScopedFlushToZero ftz;

  if (numChannels <= 0 || channels == nullptr || numSamples <= 0) {
    return;
  }

  const bool stereo = (numChannels >= 2 && channels[0] != nullptr && channels[1] != nullptr);
  float* ch0 = channels[0];
  float* ch1 = stereo ? channels[1] : nullptr;

  const float smoothMs = paretto::dsp::clamp(params_.smoothMs, 0.0f, 200.0f);
  gainAmp_.setTimeMs(smoothMs);
  cutoffHz_.setTimeMs(smoothMs);
  drive_.setTimeMs(smoothMs);
  pan_.setTimeMs(smoothMs);
  width_.setTimeMs(smoothMs);

  lfo_.setShape(shapeFrom01(params_.lfoShape));
  lfo_.setPulseWidth(0.5f);

  double bpm = ctx.transport.bpm;
  if (!ctx.transport.valid || bpm <= 1.0) {
    bpm = 120.0;
  }

  const double beatsPerSec = bpm / 60.0;
  const double beatsPerCycle = paretto::dsp::beatsPerStep(params_.lfoRate);
  const float lfoHz = static_cast<float>((beatsPerCycle > 0.0) ? (beatsPerSec / beatsPerCycle) : beatsPerSec);
  lfo_.setRateHz(lfoHz);

  const float chaos = paretto::dsp::clamp(params_.chaos, 0.0f, 1.0f);
  const float motion = paretto::dsp::clamp(params_.motion, 0.0f, 1.0f);

  const float rndRate = paretto::dsp::clamp(params_.rndRateHz * (1.0f + 4.0f * chaos), 0.05f, 10.0f);
  rnd_.setRateHz(rndRate);
  rnd_.setSmoothMs(std::max(1.0f, smoothMs));

  const float envSense = paretto::dsp::clamp(params_.envSense, 0.0f, 1.0f);

  float wL = 0.0f;
  float wE = 0.0f;
  float wR = 0.0f;
  weightsForMode(params_.mode, wL, wE, wR);

  wR = wR + chaos * 0.8f;
  wL = wL * (1.0f - 0.2f * chaos);
  wE = wE * (0.2f + 1.6f * envSense);

  const float toGain = paretto::dsp::clamp(params_.toGain, -1.0f, 1.0f);
  const float toTone = paretto::dsp::clamp(params_.toTone, -1.0f, 1.0f);
  const float toGrit = paretto::dsp::clamp(params_.toGrit, 0.0f, 1.0f);
  const float toSpace = paretto::dsp::clamp(params_.toSpace, -1.0f, 1.0f);

  const float baseHz = 1500.0f;
  const float spanOct = 3.0f;

  const float mix = paretto::dsp::clamp(params_.mix, 0.0f, 1.0f);
  const float outAmp = paretto::dsp::dbToAmp(params_.outputDb);

  for (int i = 0; i < numSamples; ++i) {
    const float inL = ch0[i];
    const float inR = stereo ? ch1[i] : 0.0f;
    const float mono = stereo ? 0.5f * (inL + inR) : inL;

    float env = env_.processSample(mono);
    env = paretto::dsp::clamp(env, 0.0f, 1.0f);
    const float env2 = env * 2.0f - 1.0f;

    const float lfo = lfo_.processSample();
    const float rnd = rnd_.processSample(sampleRate_);

    float mod = clamp11(wL * lfo + wE * env2 + wR * rnd);
    mod = clamp11(mod * motion);

    const float gainDb = mod * toGain * 12.0f;
    const float gainAmpT = paretto::dsp::dbToAmp(gainDb);

    const float cutoffT = baseHz * paretto::dsp::pow2(mod * toTone * spanOct);
    const float cutoffHz = cutoffHz_.process(paretto::dsp::clamp(cutoffT, 20.0f, 20000.0f));

    const float driveT = 1.0f + (0.5f * (mod + 1.0f)) * (toGrit * 10.0f);
    const float drive = std::max(1.0f, drive_.process(driveT));

    const float panT = mod * toSpace;
    const float pan = pan_.process(panT);

    const float widthT = paretto::dsp::clamp(1.0f + 0.5f * mod * toSpace, 0.5f, 1.5f);
    const float width = width_.process(widthT);

    const float gainAmp = gainAmp_.process(gainAmpT);

    svfL_.setCutoffHz(cutoffHz);
    svfR_.setCutoffHz(cutoffHz);

    float wetL = svfL_.processSample(inL * gainAmp);
    float wetR = stereo ? svfR_.processSample(inR * gainAmp) : 0.0f;

    wetL = clipper_.processSample(wetL * drive);
    wetR = stereo ? clipper_.processSample(wetR * drive) : 0.0f;

    if (stereo) {
      float mid = 0.5f * (wetL + wetR);
      float side = 0.5f * (wetL - wetR);
      side *= width;
      wetL = mid + side;
      wetR = mid - side;

      const float gl = panGainL(pan);
      const float gr = panGainR(pan);
      wetL *= gl * 1.41421356f;
      wetR *= gr * 1.41421356f;
    }

    if (!paretto::dsp::isFinite(wetL)) {
      wetL = 0.0f;
    }
    if (stereo && !paretto::dsp::isFinite(wetR)) {
      wetR = 0.0f;
    }

    const float outL = (1.0f - mix) * inL + mix * wetL;
    const float outR = stereo ? ((1.0f - mix) * inR + mix * wetR) : 0.0f;

    ch0[i] = outL * outAmp;
    if (stereo) {
      ch1[i] = outR * outAmp;
    }
  }
}

}  // namespace motion
}  // namespace paretto
