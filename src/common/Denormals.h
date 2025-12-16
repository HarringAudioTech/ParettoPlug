#pragma once

#include <cstdint>

#if defined(_M_IX86) || defined(_M_X64) || defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace paretto::dsp {

class ScopedFlushToZero {
public:
  ScopedFlushToZero() {
#if defined(_M_IX86) || defined(_M_X64) || defined(__SSE__)
    oldMxcsr_ = _mm_getcsr();
    auto mxcsr = oldMxcsr_;
    mxcsr |= (1u << 15);  // FTZ
    mxcsr |= (1u << 6);   // DAZ
    _mm_setcsr(mxcsr);
#endif
  }

  ~ScopedFlushToZero() {
#if defined(_M_IX86) || defined(_M_X64) || defined(__SSE__)
    _mm_setcsr(oldMxcsr_);
#endif
  }

  ScopedFlushToZero(const ScopedFlushToZero&) = delete;
  ScopedFlushToZero& operator=(const ScopedFlushToZero&) = delete;

private:
#if defined(_M_IX86) || defined(_M_X64) || defined(__SSE__)
  unsigned int oldMxcsr_{};
#endif
};

}  // namespace paretto::dsp
