#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace paretto {
namespace dsp {

[[nodiscard]] inline float clamp(float x, float lo, float hi) {
  return std::min(hi, std::max(lo, x));
}

[[nodiscard]] inline float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

[[nodiscard]] inline float dbToAmp(float db) {
  return std::pow(10.0f, db / 20.0f);
}

[[nodiscard]] inline float ampToDb(float amp) {
  const float a = std::max(amp, 1.0e-20f);
  return 20.0f * std::log10(a);
}

[[nodiscard]] inline float pow2(float x) {
  return std::exp2(x);
}

[[nodiscard]] inline float shape01(float x01, float curve01) {
  const float x = clamp(x01, 0.0f, 1.0f);
  const float c = clamp(curve01, 0.0f, 1.0f);

  const float exponent = 1.0f + 4.0f * c;
  return std::pow(x, exponent);
}

[[nodiscard]] inline bool isFinite(float x) {
  return std::isfinite(static_cast<double>(x)) != 0;
}

}  // namespace dsp
}  // namespace paretto
