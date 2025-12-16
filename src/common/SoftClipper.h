#pragma once

#include <cmath>

#include "AudioMath.h"

namespace paretto::dsp {

enum class ClipCurve {
  Cubic,
  TanhApprox,
};

class SoftClipper {
public:
  void setCurve(ClipCurve c) { curve_ = c; }

  [[nodiscard]] float processSample(float x) const {
    switch (curve_) {
      case ClipCurve::Cubic: {
        const float y = clamp(x, -1.5f, 1.5f);
        return y - (y * y * y) / 3.0f;
      }
      case ClipCurve::TanhApprox: {
        const float y = clamp(x, -3.0f, 3.0f);
        const float y2 = y * y;
        return y * (27.0f + y2) / (27.0f + 9.0f * y2);
      }
    }

    return x;
  }

private:
  ClipCurve curve_{ClipCurve::Cubic};
};

}  // namespace paretto::dsp
