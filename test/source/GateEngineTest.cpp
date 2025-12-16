#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "gate/GateEngine.h"

namespace paretto_test {

static bool allFinite(const std::vector<float>& v) {
  return std::all_of(v.begin(), v.end(), [](float x) { return std::isfinite(static_cast<double>(x)) != 0; });
}

TEST(GateEngine, MixZeroIsNeutral) {
  paretto::gate::GateEngine eng{};
  eng.prepare(48000.0, 123u);

  paretto::gate::GateParameters p{};
  p.mix = 0.0f;
  p.depth = 1.0f;
  p.toneDepth = 0.0f;
  eng.setParameters(p);

  std::vector<float> left(256);
  std::vector<float> right(256);
  for (int i = 0; i < 256; ++i) {
    left[static_cast<std::size_t>(i)] = (i % 2 == 0) ? 0.25f : -0.25f;
    right[static_cast<std::size_t>(i)] = left[static_cast<std::size_t>(i)];
  }

  float* chans[2] = {left.data(), right.data()};
  paretto::dsp::ProcessContext ctx{};
  ctx.sampleRate = 48000.0;
  ctx.transport.valid = false;

  eng.process(chans, 2, 256, ctx);

  for (int i = 0; i < 256; ++i) {
    ASSERT_NEAR(left[static_cast<std::size_t>(i)], ((i % 2 == 0) ? 0.25f : -0.25f), 1.0e-6f);
    ASSERT_NEAR(right[static_cast<std::size_t>(i)], ((i % 2 == 0) ? 0.25f : -0.25f), 1.0e-6f);
  }
}

TEST(GateEngine, ResetDeterminismWithSeed) {
  paretto::gate::GateEngine a{};
  paretto::gate::GateEngine b{};
  a.prepare(48000.0, 42u);
  b.prepare(48000.0, 42u);

  paretto::gate::GateParameters p{};
  p.prob = 0.5f;
  p.steps = paretto::gate::Steps::S16;
  p.syncEnabled = false;
  p.rateHz = 2.0f;
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
  }

  for (std::size_t i = 0; i < outA.size(); ++i) {
    ASSERT_NEAR(outA[i], firstA[i], 1.0e-6f);
  }
}

TEST(GateEngine, FloorIsRespectedWhenClosed) {
  paretto::gate::GateEngine eng{};
  eng.prepare(48000.0, 7u);

  paretto::gate::GateParameters p{};
  p.mix = 1.0f;
  p.floorDb = -60.0f;
  p.depth = 1.0f;
  p.prob = 0.0f;  // force inactive steps
  p.syncEnabled = false;
  p.rateHz = 1.0f;
  eng.setParameters(p);

  std::vector<float> x(256, 1.0f);
  float* chans[1] = {x.data()};

  paretto::dsp::ProcessContext ctx{};
  ctx.sampleRate = 48000.0;
  ctx.transport.valid = false;

  eng.process(chans, 1, 256, ctx);

  const float floorAmp = paretto::dsp::dbToAmp(-60.0f);
  for (float v : x) {
    ASSERT_LE(std::fabs(v), floorAmp + 1.0e-3f);
  }

  ASSERT_TRUE(allFinite(x));
}

}  // namespace paretto_test
