#pragma once

#include <cstdint>

#include "common/DspContext.h"
#include "common/EnvFollower.h"
#include "common/Lfo.h"
#include "common/SmoothRandom.h"
#include "common/SoftClipper.h"
#include "common/Smoothers.h"
#include "common/Svf.h"
#include "motion/MotionParameters.h"

namespace paretto {
namespace motion {

class MotionEngine {
public:
  void prepare(double sampleRate, int maxBlockSize);
  void reset();

  void setSeed(std::uint32_t seed);
  void setParameters(const MotionParameters& p);

  void process(float** channels, int numChannels, int numSamples, const paretto::dsp::ProcessContext& ctx);

private:
  double sampleRate_{48000.0};
  std::uint32_t seed_{0};

  MotionParameters params_{};

  paretto::dsp::Lfo lfo_{};
  paretto::dsp::EnvFollower env_{};
  paretto::dsp::SmoothRandom rnd_{};
  paretto::dsp::SvfLowpass svfL_{};
  paretto::dsp::SvfLowpass svfR_{};
  paretto::dsp::SoftClipper clipper_{};

  paretto::dsp::OnePoleSmoother gainAmp_{};
  paretto::dsp::OnePoleSmoother cutoffHz_{};
  paretto::dsp::OnePoleSmoother drive_{};
  paretto::dsp::OnePoleSmoother pan_{};
  paretto::dsp::OnePoleSmoother width_{};
};

}  // namespace motion
}  // namespace paretto
