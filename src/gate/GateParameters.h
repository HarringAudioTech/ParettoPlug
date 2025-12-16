#pragma once

#include <array>

#include "../common/Constants.h"
#include "../common/StepSequencer.h"

namespace paretto {
namespace gate {

enum class Steps {
  S8 = 8,
  S16 = 16,
  S32 = 32,
};

enum class Groove {
  Straight,
  Swing,
  UKG,
  Jersey,
  DnB,
  RandomMicro,
};

struct GateStep {
  float level{1.0f};
  float curve{0.0f};
  float prob{1.0f};
  float accent{0.0f};
};

struct GatePattern {
  std::array<GateStep, paretto::dsp::MaxSteps> steps{};
};

struct GateParameters {
  bool syncEnabled{true};
  paretto::dsp::SyncRate rate{paretto::dsp::SyncRate::R1_8};
  float rateHz{2.0f};
  Steps steps{Steps::S16};
  float swing{0.0f};
  float phaseDeg{0.0f};

  float depth{1.0f};
  float smoothMs{5.0f};
  float floorDb{-60.0f};
  float prob{1.0f};
  float accent{0.3f};

  float toneDepth{0.0f};
  float toneBaseHz{2000.0f};
  float toneSpanOct{2.0f};
  float res{0.2f};

  float morph{0.0f};
  Groove groove{Groove::Straight};

  float mix{1.0f};
  float outputDb{0.0f};

  GatePattern patternA{};
  GatePattern patternB{};
};

}  // namespace gate
}  // namespace paretto
