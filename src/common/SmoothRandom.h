#pragma once

#include <algorithm>

#include "Rng.h"
#include "Smoothers.h"

namespace paretto::dsp {

class SmoothRandom {
public:
  void prepare(double sampleRate) {
    smoother_.prepare(sampleRate);
  }

  void reset(float value = 0.0f) {
    held_ = value;
    smoother_.reset(value);
    samplesUntilNext_ = 0;
  }

  void seed(std::uint32_t s) { rng_.seed(s); }

  void setRateHz(float hz) { rateHz_ = std::max(0.001f, hz); }

  void setSmoothMs(float ms) { smoother_.setTimeMs(ms); }

  [[nodiscard]] float processSample(double sampleRate) {
    if (samplesUntilNext_ <= 0) {
      held_ = rng_.nextFloat11();
      samplesUntilNext_ = static_cast<int>(std::max(1.0, sampleRate / static_cast<double>(rateHz_)));
    }

    --samplesUntilNext_;
    return smoother_.process(held_);
  }

private:
  Rng rng_{};
  OnePoleSmoother smoother_{};
  float rateHz_{0.3f};
  float held_{0.0f};
  int samplesUntilNext_{0};
};

}  // namespace paretto::dsp
