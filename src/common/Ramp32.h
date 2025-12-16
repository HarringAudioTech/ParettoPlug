#pragma once

#include <array>

#include "Constants.h"

namespace paretto::dsp {

inline void fillRamp(std::array<float, MicroblockSize>& out, int n, float start, float end) {
  if (n <= 0) {
    return;
  }

  if (n == 1) {
    out[0] = end;
    return;
  }

  const float step = (end - start) / static_cast<float>(n - 1);
  float v = start;
  for (int i = 0; i < n; ++i) {
    out[static_cast<std::size_t>(i)] = v;
    v += step;
  }
}

}  // namespace paretto::dsp
