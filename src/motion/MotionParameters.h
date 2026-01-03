#pragma once

#include "common/StepSequencer.h"

namespace paretto {
namespace motion {

enum class MotionMode {
  Pulse,
  Drift,
  Pump,
  Wobble,
  Scatter,
};

struct MotionParameters {
  float motion{0.5f};
  float chaos{0.0f};
  float smoothMs{20.0f};
  MotionMode mode{MotionMode::Drift};
  float mix{1.0f};
  float outputDb{0.0f};

  float toGain{0.0f};
  float toTone{0.5f};
  float toGrit{0.2f};
  float toSpace{0.2f};

  bool syncEnabled{true};
  paretto::dsp::SyncRate lfoRate{paretto::dsp::SyncRate::R1_4};
  float lfoShape{0.2f};
  float envSense{0.5f};
  float rndRateHz{0.3f};
};

}  // namespace motion
}  // namespace paretto
