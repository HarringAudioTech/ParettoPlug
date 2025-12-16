#pragma once

#include <cstdint>

namespace paretto {
namespace dsp {

struct TransportInfo {
  bool valid{};
  bool playing{};
  double bpm{120.0};
  double ppqPosition{0.0};
  std::int64_t timeInSamples{0};
};

struct ProcessContext {
  double sampleRate{48000.0};
  TransportInfo transport{};
};

}  // namespace dsp
}  // namespace paretto
