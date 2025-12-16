#pragma once

#include <cstdint>

#include "GateParameters.h"

#include "../common/DspContext.h"
#include "../common/Denormals.h"
#include "../common/Rng.h"
#include "../common/Smoothers.h"
#include "../common/StepSequencer.h"
#include "../common/Svf.h"

namespace paretto {
namespace gate {

class GateEngine {
public:
  void prepare(double sampleRate, std::uint32_t seed);
  void reset();

  void setSeed(std::uint32_t seed);
  void setParameters(const GateParameters& p);

  void process(float* const* channels, int numChannels, int numSamples, const paretto::dsp::ProcessContext& ctx);

  [[nodiscard]] int getLatencySamples() const { return 0; }

private:
  [[nodiscard]] int stepsCount() const;
  void updateGrooveDerived();

  GateParameters params_{};

  paretto::dsp::StepSequencer sequencer_{};
  paretto::dsp::Rng rng_{};

  paretto::dsp::OnePoleSmoother gateSmooth_{};

  struct ChannelState {
    paretto::dsp::SvfLowpass filter{};
  };

  ChannelState ch_[2]{};

  bool stepActive_{true};
  int lastStepIndex_{-1};

  std::uint32_t seed_{0};

  double sampleRate_{48000.0};
  std::int64_t internalSamplePos_{0};

  float grooveSwing_{0.0f};
};

}  // namespace gate
}  // namespace paretto
