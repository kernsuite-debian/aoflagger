#ifndef SUMTHRESHOLD_H
#define SUMTHRESHOLD_H

#include <cstddef>
#include <cstring>

#include "../structures/image2d.h"
#include "../structures/mask2d.h"

namespace algorithms {

class SumThreshold {
 public:
  struct VerticalScratch {
    VerticalScratch();
    VerticalScratch(size_t width, size_t height);
    std::unique_ptr<int[], decltype(&free)> lastFlaggedPos;
    std::unique_ptr<num_t[], decltype(&free)> sum;
    std::unique_ptr<int[], decltype(&free)> count;
  };

  template <size_t Length>
  static void Horizontal(const Image2D* input, Mask2D* mask, num_t threshold);

  template <size_t Length>
  static void Vertical(const Image2D* input, Mask2D* mask, num_t threshold);

  template <size_t Length>
  static void HorizontalLarge(const Image2D* input, Mask2D* mask,
                              Mask2D* scratch, num_t threshold);

/* We always want to compile SSE for 64-bit Intel. Note that code will only be
   executed if the CPU where the binary is run supports SSE. However, code can
   only be compiled successfully if either __SSE__ is defined or if we're on
   64-bit Intel (since we're not cross-compiling) */
#if defined(__SSE__) || defined(__x86_64__)
  template <size_t Length>
  __attribute__((target("sse"))) static void VerticalLargeSSE(
      const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);

  __attribute__((target("sse"))) static void VerticalLargeSSE(
      const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
      num_t threshold);

  template <size_t Length>
  __attribute__((target("sse"))) static void HorizontalLargeSSE(
      const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);

  __attribute__((target("sse"))) static void HorizontalLargeSSE(
      const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
      num_t threshold);

#endif  // defined(__SSE__) || defined(__x86_64__)

/* We always want to compile AVX2 for 64-bit Intel. Note that code will only be
   executed if the CPU where the binary is run supports AVX2. However, code can
   only be compiled successfully if either __AVX2__ is defined or if we're on
   64-bit Intel (since we're not cross-compiling) */
#if defined(__AVX2__) || defined(__x86_64__)
  template <size_t Length>
  __attribute__((target("avx2"))) static void VerticalLargeAVX(
      const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold);

  __attribute__((target("avx2"))) static void VerticalLargeAVX(
      const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
      num_t threshold);

  __attribute__((target("avx2"))) static void HorizontalAVXDumas(
      const Image2D* input, Mask2D* mask, size_t length, num_t threshold);

  __attribute__((target("avx2"))) static void VerticalAVXDumas(
      const Image2D* input, Mask2D* mask, VerticalScratch* scratch,
      size_t length, num_t threshold);

  template <size_t Length>
  __attribute__((target("avx2"))) static void HorizontalAVXDumas(
      const Image2D* input, Mask2D* mask, num_t threshold);

  template <size_t Length>
  __attribute__((target("avx2"))) static void VerticalAVXDumas(
      const Image2D* input, Mask2D* mask, VerticalScratch* scratch,
      num_t threshold);

#endif  // defined(__AVX2__) || defined(__x86_64__)

  template <size_t Length>
  static void VerticalLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch,
                            num_t threshold);

  template <size_t Length>
  static void Large(const Image2D* input, Mask2D* mask, num_t hThreshold,
                    num_t vThreshold) {
    HorizontalLarge<Length>(input, mask, hThreshold);
    VerticalLarge<Length>(input, mask, vThreshold);
  }

  static void VerticalLarge(const Image2D* input, Mask2D* mask, Mask2D* scratch,
                            VerticalScratch* vScratch, size_t length,
                            num_t threshold) {
#if defined(__AVX2__) || defined(__x86_64__)
    if (__builtin_cpu_supports("avx2")) {
      VerticalAVXDumas(input, mask, vScratch, length, threshold);
      return;
    }
#endif
#if defined(__SSE__) || defined(__x86_64__)
    if (__builtin_cpu_supports("sse")) {
      VerticalLargeSSE(input, mask, scratch, length, threshold);
      return;
    }
#endif
    VerticalLargeReference(input, mask, scratch, length, threshold);
  }

  static void VerticalLargeReference(const Image2D* input, Mask2D* mask,
                                     Mask2D* scratch, size_t length,
                                     num_t threshold);

  static void HorizontalLargeReference(const Image2D* input, Mask2D* mask,
                                       Mask2D* scratch, size_t length,
                                       num_t threshold);

  static void HorizontalLarge(const Image2D* input, Mask2D* mask,
                              Mask2D* scratch, size_t length, num_t threshold) {
#if defined(__AVX2__) || defined(__x86_64__)
    if (__builtin_cpu_supports("avx2")) {
      if (length >= 64)
        HorizontalAVXDumas(input, mask, length, threshold);
      else
        HorizontalLargeSSE(input, mask, scratch, length, threshold);
      return;
    }
#endif
#if defined(__SSE__) || defined(__x86_64__)
    if (__builtin_cpu_supports("sse")) {
      HorizontalLargeSSE(input, mask, scratch, length, threshold);
      return;
    }
#endif
    HorizontalLargeReference(input, mask, scratch, length, threshold);
  }
};

}  // namespace algorithms

#endif
