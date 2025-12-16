#pragma once

#include <cmath>

#include "AudioMath.h"

namespace paretto::dsp {

class EnvFollower {
public:
  void prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoeffs();
  }

  void reset() { env_ = 0.0f; }

  void setAttackReleaseMs(float attackMs, float releaseMs) {
    attackMs_ = attackMs;
    releaseMs_ = releaseMs;
    updateCoeffs();
  }

  [[nodiscard]] float processSample(float x) {
    const float rect = std::fabs(x);
    const float c = (rect > env_) ? attack_ : release_;
    env_ = c * env_ + (1.0f - c) * rect;
    return env_;
  }

  [[nodiscard]] float getCurrent() const { return env_; }

private:
  void updateCoeffs() {
    attack_ = coeffMs(attackMs_);
    release_ = coeffMs(releaseMs_);
  }

  [[nodiscard]] float coeffMs(float ms) const {
    const double t = std::max(0.0, static_cast<double>(ms)) * 0.001;
    if (t <= 0.0 || sampleRate_ <= 0.0) {
      return 0.0f;
    }

    const double a = std::exp(-1.0 / (t * sampleRate_));
    return static_cast<float>(a);
  }

  double sampleRate_{48000.0};
  float attackMs_{10.0f};
  float releaseMs_{100.0f};
  float attack_{0.0f};
  float release_{0.0f};
  float env_{0.0f};
};

}  // namespace paretto::dsp
