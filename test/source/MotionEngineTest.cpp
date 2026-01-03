#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "motion/MotionEngine.h"

namespace paretto_test {

static bool allFinite(const std::vector<float>& v) {
  return std::all_of(v.begin(), v.end(), [](float x) { return std::isfinite(static_cast<double>(x)) != 0; });
}

TEST(MotionEngine, MixZeroIsNeutral) {
  paretto::motion::MotionEngine eng{};
  eng.prepare(48000.0, 512);
  eng.setSeed(123u);

  paretto::motion::MotionParameters p{};
  p.mix = 0.0f;
  p.motion = 1.0f;
  p.toGain = 1.0f;
  p.toTone = 1.0f;
  p.toGrit = 1.0f;
  p.toSpace = 1.0f;
  eng.setParameters(p);

  std::vector<float> left(256);
  std::vector<float> right(256);
  for (int i = 0; i < 256; ++i) {
    left[static_cast<std::size_t>(i)] = (i % 2 == 0) ? 0.2f : -0.2f;
    right[static_cast<std::size_t>(i)] = left[static_cast<std::size_t>(i)];
  }

  float* chans[2] = {left.data(), right.data()};
  paretto::dsp::ProcessContext ctx{};
  ctx.sampleRate = 48000.0;
  ctx.transport.valid = false;

  eng.process(chans, 2, 256, ctx);

  for (int i = 0; i < 256; ++i) {
    ASSERT_NEAR(left[static_cast<std::size_t>(i)], ((i % 2 == 0) ? 0.2f : -0.2f), 1.0e-6f);
    ASSERT_NEAR(right[static_cast<std::size_t>(i)], ((i % 2 == 0) ? 0.2f : -0.2f), 1.0e-6f);
  }
}

TEST(MotionEngine, ResetDeterminismWithSeed) {
  paretto::motion::MotionEngine a{};
  paretto::motion::MotionEngine b{};
  a.prepare(48000.0, 512);
  b.prepare(48000.0, 512);
  a.setSeed(42u);
  b.setSeed(42u);

  paretto::motion::MotionParameters p{};
  p.motion = 0.8f;
  p.chaos = 0.4f;
  p.toGain = 0.3f;
  p.toTone = 0.4f;
  p.toGrit = 0.5f;
  p.toSpace = 0.3f;
  p.syncEnabled = false;
  p.rndRateHz = 1.5f;
  p.mix = 1.0f;
  a.setParameters(p);
  b.setParameters(p);

  std::vector<float> in(512, 0.1f);
  std::vector<float> outA = in;
  std::vector<float> outB = in;

  paretto::dsp::ProcessContext ctx{};
  ctx.sampleRate = 48000.0;
  ctx.transport.valid = false;

  float* chansA[1] = {outA.data()};
  float* chansB[1] = {outB.data()};

  a.process(chansA, 1, 512, ctx);
  b.process(chansB, 1, 512, ctx);

  const auto firstA = outA;

  ASSERT_EQ(outA.size(), outB.size());
  for (std::size_t i = 0; i < outA.size(); ++i) {
    ASSERT_NEAR(outA[i], outB[i], 1.0e-6f);
  }

  ASSERT_TRUE(allFinite(outA));
  ASSERT_TRUE(allFinite(outB));

  a.reset();
  b.reset();

  outA = in;
  outB = in;
  chansA[0] = outA.data();
  chansB[0] = outB.data();

  a.process(chansA, 1, 512, ctx);
  b.process(chansB, 1, 512, ctx);

  for (std::size_t i = 0; i < outA.size(); ++i) {
    ASSERT_NEAR(outA[i], outB[i], 1.0e-6f);
    ASSERT_NEAR(outA[i], firstA[i], 1.0e-6f);
  }
}

}  // namespace paretto_test
