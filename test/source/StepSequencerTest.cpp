#include <gtest/gtest.h>

#include "common/StepSequencer.h"

namespace paretto_test {

TEST(StepSequencer, StepIndexProgressesAtKnownOffsets) {
  paretto::dsp::StepSequencer seq{};
  seq.prepare(48000.0);
  seq.setBpm(120.0);
  seq.setTiming(true, paretto::dsp::SyncRate::R1_4, 2.0f, 16, 0.0f, 0.0f);

  // At 120 BPM: 1 beat = 24000 samples. 1/4 note is 1 beat => step length ~24000.
  const auto p0 = seq.getPositionAt(0);
  const auto p1 = seq.getPositionAt(24000);
  const auto p2 = seq.getPositionAt(48000);

  ASSERT_EQ(p0.stepIndex, 0);
  ASSERT_EQ(p1.stepIndex, 1);
  ASSERT_EQ(p2.stepIndex, 2);
}

}  // namespace paretto_test
