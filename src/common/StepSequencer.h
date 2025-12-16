#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "AudioMath.h"
#include "Constants.h"

namespace paretto {
namespace dsp {

enum class SyncRate {
  R1_1,
  R1_2,
  R1_4,
  R1_8,
  R1_16,
  R1_32,
  R1_1_Dotted,
  R1_2_Dotted,
  R1_4_Dotted,
  R1_8_Dotted,
  R1_16_Dotted,
  R1_32_Dotted,
  R1_1_Triplet,
  R1_2_Triplet,
  R1_4_Triplet,
  R1_8_Triplet,
  R1_16_Triplet,
  R1_32_Triplet,
};

[[nodiscard]] inline double beatsPerStep(SyncRate rate) {
  auto base = [](int denom) { return 4.0 / static_cast<double>(denom); };

  switch (rate) {
    case SyncRate::R1_1: return base(1);
    case SyncRate::R1_2: return base(2);
    case SyncRate::R1_4: return base(4);
    case SyncRate::R1_8: return base(8);
    case SyncRate::R1_16: return base(16);
    case SyncRate::R1_32: return base(32);

    case SyncRate::R1_1_Dotted: return base(1) * 1.5;
    case SyncRate::R1_2_Dotted: return base(2) * 1.5;
    case SyncRate::R1_4_Dotted: return base(4) * 1.5;
    case SyncRate::R1_8_Dotted: return base(8) * 1.5;
    case SyncRate::R1_16_Dotted: return base(16) * 1.5;
    case SyncRate::R1_32_Dotted: return base(32) * 1.5;

    case SyncRate::R1_1_Triplet: return base(1) * (2.0 / 3.0);
    case SyncRate::R1_2_Triplet: return base(2) * (2.0 / 3.0);
    case SyncRate::R1_4_Triplet: return base(4) * (2.0 / 3.0);
    case SyncRate::R1_8_Triplet: return base(8) * (2.0 / 3.0);
    case SyncRate::R1_16_Triplet: return base(16) * (2.0 / 3.0);
    case SyncRate::R1_32_Triplet: return base(32) * (2.0 / 3.0);
  }

  return base(8);
}

struct StepPosition {
  int stepIndex{};
  std::int64_t stepStartSample{};
  std::int64_t stepLengthSamples{1};
  float position01{};
};

class StepSequencer {
public:
  void prepare(double sampleRate) { sampleRate_ = sampleRate; }

  static int clampSteps(int v) {
    if (v < 1) {
      return 1;
    }
    if (v > MaxSteps) {
      return MaxSteps;
    }
    return v;
  }

  void setTiming(bool syncEnabled, SyncRate rate, float rateHz, int steps, float swing, float phaseDeg) {
    syncEnabled_ = syncEnabled;
    rate_ = rate;
    rateHz_ = rateHz;
    steps_ = clampSteps(steps);
    swing_ = std::min(std::max(swing, 0.0f), 0.95f);
    phaseDeg_ = std::min(std::max(phaseDeg, 0.0f), 360.0f);
    recalc();
  }

  void setBpm(double bpm) {
    bpm_ = (bpm > 1.0) ? bpm : 120.0;
    recalc();
  }

  void reset() { sampleOffset_ = 0; }

  void advance(std::int64_t numSamples) { sampleOffset_ += numSamples; }

  [[nodiscard]] StepPosition getPositionAt(std::int64_t absoluteSamplePosition) const {
    const std::int64_t pos = absoluteSamplePosition + phaseOffsetSamples_;
    const std::int64_t cycle = (cycleLengthSamples_ > 0) ? cycleLengthSamples_ : 1;
    std::int64_t x = pos % cycle;
    if (x < 0) {
      x += cycle;
    }

    std::int64_t start = 0;
    for (int i = 0; i < steps_; ++i) {
      const std::int64_t len = stepLengthsSamples_[static_cast<std::size_t>(i)];
      if (x < start + len) {
        const float p01 = static_cast<float>(static_cast<double>(x - start) / static_cast<double>(std::max<std::int64_t>(1, len)));
        return StepPosition{.stepIndex = i, .stepStartSample = start, .stepLengthSamples = len, .position01 = p01};
      }
      start += len;
    }

    const std::int64_t len = stepLengthsSamples_[0];
    return StepPosition{.stepIndex = 0, .stepStartSample = 0, .stepLengthSamples = len, .position01 = 0.0f};
  }

  [[nodiscard]] StepPosition getPositionFromInternalClock() const {
    return getPositionAt(sampleOffset_);
  }

  [[nodiscard]] std::int64_t getCycleLengthSamples() const { return cycleLengthSamples_; }

private:
  void recalc() {
    const double sr = (sampleRate_ > 0.0) ? sampleRate_ : 48000.0;

    double baseStepSamplesD = 0.0;
    if (syncEnabled_) {
      const double bps = bpm_ / 60.0;
      const double spb = sr / bps;
      baseStepSamplesD = spb * beatsPerStep(rate_);
    } else {
      const double hz = std::max(0.001, static_cast<double>(rateHz_));
      baseStepSamplesD = sr / hz;
    }

    const std::int64_t base = static_cast<std::int64_t>(std::max(1.0, baseStepSamplesD));

    std::int64_t sum = 0;
    for (int i = 0; i < steps_; ++i) {
      const bool firstInPair = (i % 2) == 0;
      const double m = firstInPair ? (1.0 + static_cast<double>(swing_)) : (1.0 - static_cast<double>(swing_));
      const std::int64_t len = static_cast<std::int64_t>(std::max(1.0, static_cast<double>(base) * m));
      stepLengthsSamples_[static_cast<std::size_t>(i)] = len;
      sum += len;
    }

    cycleLengthSamples_ = std::max<std::int64_t>(1, sum);
    phaseOffsetSamples_ = static_cast<std::int64_t>(static_cast<double>(cycleLengthSamples_) * (static_cast<double>(phaseDeg_) / 360.0));
  }

  double sampleRate_{48000.0};
  double bpm_{120.0};
  bool syncEnabled_{true};
  SyncRate rate_{SyncRate::R1_8};
  float rateHz_{2.0f};
  int steps_{16};
  float swing_{0.0f};
  float phaseDeg_{0.0f};

  std::array<std::int64_t, MaxSteps> stepLengthsSamples_{};
  std::int64_t cycleLengthSamples_{1};
  std::int64_t phaseOffsetSamples_{0};
  std::int64_t sampleOffset_{0};
};

}  // namespace dsp
}  // namespace paretto
