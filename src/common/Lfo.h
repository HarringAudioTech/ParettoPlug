#pragma once

#include <cmath>

#include "AudioMath.h"

namespace paretto::dsp {

enum class LfoShape {
  Sine,
  Tri,
  Saw,
  Pulse,
};

class Lfo {
public:
  void prepare(double sampleRate) {
    sampleRate_ = sampleRate;
  }

  void reset(float phase01 = 0.0f) {
    phase_ = phase01 - std::floor(phase01);
  }

  void setRateHz(float hz) {
    rateHz_ = std::max(0.0f, hz);
  }

  void setPulseWidth(float pw01) {
    pulseWidth_ = clamp(pw01, 0.01f, 0.99f);
  }

  void setShape(LfoShape shape) { shape_ = shape; }

  [[nodiscard]] float processSample() {
    phase_ += static_cast<double>(rateHz_) / sampleRate_;
    phase_ -= std::floor(phase_);
    return eval(static_cast<float>(phase_));
  }

private:
  [[nodiscard]] float eval(float p01) const {
    switch (shape_) {
      case LfoShape::Sine: {
        const float a = 2.0f * 3.14159265358979323846f;
        return std::sin(a * p01);
      }
      case LfoShape::Tri: {
        const float v = 2.0f * std::fabs(2.0f * (p01 - std::floor(p01 + 0.5f))) - 1.0f;
        return v;
      }
      case LfoShape::Saw: {
        return 2.0f * p01 - 1.0f;
      }
      case LfoShape::Pulse: {
        return (p01 < pulseWidth_) ? 1.0f : -1.0f;
      }
    }

    return 0.0f;
  }

  double sampleRate_{48000.0};
  float rateHz_{1.0f};
  float pulseWidth_{0.5f};
  LfoShape shape_{LfoShape::Sine};
  double phase_{0.0};
};

}  // namespace paretto::dsp
