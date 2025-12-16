#pragma once

#include <cmath>

#include "AudioMath.h"

namespace paretto {
namespace dsp {

class SvfLowpass {
public:
  void prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoeffs();
  }

  void reset() {
    ic1eq_ = 0.0f;
    ic2eq_ = 0.0f;
  }

  void setCutoffHz(float hz) {
    cutoffHz_ = hz;
    updateCoeffs();
  }

  void setResonance(float res01) {
    res01_ = clamp(res01, 0.0f, 0.99f);
    updateCoeffs();
  }

  [[nodiscard]] float processSample(float v0) {
    const float v1 = (a1_ * v0 - ic2eq_) * a2_;
    const float v2 = ic1eq_ + a1_ * v1;
    ic1eq_ = 2.0f * v1 + ic1eq_;
    ic2eq_ = 2.0f * v2 + ic2eq_;
    return v2;
  }

private:
  void updateCoeffs() {
    const float fc = clamp(cutoffHz_, 20.0f, static_cast<float>(0.45 * sampleRate_));
    const float g = std::tan(3.14159265358979323846f * fc / static_cast<float>(sampleRate_));
    const float k = 0.1f + (2.0f - 0.1f) * (1.0f - res01_);

    const float a1 = 1.0f / (1.0f + g * (g + k));
    const float a2 = g * a1;

    a1_ = a1;
    a2_ = a2;
  }

  double sampleRate_{48000.0};
  float cutoffHz_{2000.0f};
  float res01_{0.2f};
  float a1_{0.0f};
  float a2_{0.0f};

  float ic1eq_{0.0f};
  float ic2eq_{0.0f};
};

}  // namespace dsp
}  // namespace paretto
