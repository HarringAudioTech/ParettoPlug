#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace paretto::dsp {

class DelayLine {
public:
  void prepare(int maxDelaySamples) {
    buffer_.assign(static_cast<std::size_t>(std::max(1, maxDelaySamples + 1)), 0.0f);
    writeIndex_ = 0;
  }

  void reset() {
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    writeIndex_ = 0;
  }

  void push(float x) {
    buffer_[writeIndex_] = x;
    writeIndex_ = (writeIndex_ + 1) % buffer_.size();
  }

  [[nodiscard]] float read(int delaySamples) const {
    const int d = std::max(0, delaySamples);
    const std::size_t size = buffer_.size();
    const std::size_t ri = (writeIndex_ + size - (static_cast<std::size_t>(d) % size)) % size;
    return buffer_[ri];
  }

private:
  std::vector<float> buffer_{};
  std::size_t writeIndex_{0};
};

}  // namespace paretto::dsp
