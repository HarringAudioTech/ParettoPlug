#pragma once

#include <algorithm>
#include <cmath>

namespace paretto::dsp {

class OnePoleSmoother {
public:
  void prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoeff();
  }

  void reset(float value) {
    z_ = value;
  }

  void setTimeMs(float timeMs) {
    timeMs_ = timeMs;
    updateCoeff();
  }

  [[nodiscard]] float process(float target) {
    z_ = a_ * z_ + (1.0f - a_) * target;
    return z_;
  }

  [[nodiscard]] float getCurrent() const { return z_; }

private:
  void updateCoeff() {
    const double t = std::max(0.0, static_cast<double>(timeMs_)) * 0.001;
    if (t <= 0.0 || sampleRate_ <= 0.0) {
      a_ = 0.0f;
      return;
    }

    const double tau = t;
    const double a = std::exp(-1.0 / (tau * sampleRate_));
    a_ = static_cast<float>(a);
  }

  double sampleRate_{48000.0};
  float timeMs_{0.0f};
  float a_{0.0f};
  float z_{0.0f};
};

}  // namespace paretto::dsp
