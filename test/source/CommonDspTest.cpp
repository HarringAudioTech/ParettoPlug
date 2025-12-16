#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "common/AudioMath.h"
#include "common/EnvFollower.h"
#include "common/Rng.h"
#include "common/Smoothers.h"

namespace paretto_test {

TEST(AudioMath, DbAmpRoundTrip) {
  const float db = -12.0f;
  const float amp = paretto::dsp::dbToAmp(db);
  const float db2 = paretto::dsp::ampToDb(amp);
  ASSERT_NEAR(db2, db, 1.0e-4f);
}

TEST(Rng, DeterministicSequence) {
  paretto::dsp::Rng a{};
  paretto::dsp::Rng b{};
  a.seed(123u);
  b.seed(123u);

  for (int i = 0; i < 1000; ++i) {
    ASSERT_EQ(a.nextU32(), b.nextU32());
  }
}

TEST(OnePoleSmoother, Converges) {
  paretto::dsp::OnePoleSmoother s{};
  s.prepare(48000.0);
  s.setTimeMs(10.0f);
  s.reset(0.0f);

  float y = 0.0f;
  for (int i = 0; i < 48000; ++i) {
    y = s.process(1.0f);
  }

  ASSERT_GT(y, 0.99f);
}

TEST(EnvFollower, RespondsAndFinite) {
  paretto::dsp::EnvFollower env{};
  env.prepare(48000.0);
  env.setAttackReleaseMs(1.0f, 50.0f);
  env.reset();

  for (int i = 0; i < 1024; ++i) {
    const float x = (i < 128) ? 1.0f : 0.0f;
    const float e = env.processSample(x);
    ASSERT_TRUE(paretto::dsp::isFinite(e));
    ASSERT_GE(e, 0.0f);
  }

  ASSERT_GT(env.getCurrent(), 0.0f);
}

}  // namespace paretto_test
