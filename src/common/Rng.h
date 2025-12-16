#pragma once

#include <cstdint>

namespace paretto {
namespace dsp {

class Rng {
public:
  void seed(std::uint32_t s) {
    state_ = (s == 0u) ? 0x12345678u : s;
  }

  [[nodiscard]] std::uint32_t nextU32() {
    std::uint32_t x = state_;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state_ = x;
    return x;
  }

  [[nodiscard]] float nextFloat01() {
    const std::uint32_t v = nextU32();
    return static_cast<float>(v) * (1.0f / 4294967296.0f);
  }

  [[nodiscard]] float nextFloat11() {
    return nextFloat01() * 2.0f - 1.0f;
  }

private:
  std::uint32_t state_{0x12345678u};
};

}  // namespace dsp
}  // namespace paretto
