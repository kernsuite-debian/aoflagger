#include "sumthreshold.h"

#include "../structures/image2d.h"

#include "../util/logger.h"

#include <boost/align/aligned_alloc.hpp>

#if defined(__SSE__) || defined(__x86_64__)

#include <stdint.h>
#include <xmmintrin.h>
#include <emmintrin.h>

#endif

namespace algorithms {

SumThreshold::VerticalScratch::VerticalScratch()
    : lastFlaggedPos(nullptr, [](void*) noexcept {}),
      sum(nullptr, [](void*) noexcept {}),
      count(nullptr, [](void*) noexcept {}) {}

SumThreshold::VerticalScratch::VerticalScratch(size_t width, size_t)
    : lastFlaggedPos(static_cast<int*>(boost::alignment::aligned_alloc(
                         64, sizeof(int) * width)),
                     &free),
      sum(static_cast<num_t*>(
              boost::alignment::aligned_alloc(64, sizeof(num_t) * width)),
          &free),
      count(static_cast<int*>(
                boost::alignment::aligned_alloc(64, sizeof(int) * width)),
            &free) {}

template <size_t Length>
void SumThreshold::Horizontal(const Image2D* input, Mask2D* mask,
                              num_t threshold) {
  if (Length <= input->Width()) {
    const size_t width = input->Width() - Length + 1;
    for (size_t y = 0; y < input->Height(); ++y) {
      for (size_t x = 0; x < width; ++x) {
        num_t sum = 0.0;
        size_t count = 0;
        for (size_t i = 0; i < Length; ++i) {
          if (!mask->Value(x + i, y)) {
            sum += input->Value(x + i, y);
            count++;
          }
        }
        if (count > 0 && fabs(sum / count) > threshold) {
          for (size_t i = 0; i < Length; ++i) mask->SetValue(x + i, y, true);
        }
      }
    }
  }
}

template <size_t Length>
void SumThreshold::Vertical(const Image2D* input, Mask2D* mask,
                            num_t threshold) {
  if (Length <= input->Height()) {
    const size_t height = input->Height() - Length + 1;
    for (size_t y = 0; y < height; ++y) {
      for (size_t x = 0; x < input->Width(); ++x) {
        num_t sum = 0.0;
        size_t count = 0;
        for (size_t i = 0; i < Length; ++i) {
          if (!mask->Value(x, y + i)) {
            sum += input->Value(x, y + i);
            count++;
          }
        }
        if (count > 0 && fabs(sum / count) > threshold) {
          for (size_t i = 0; i < Length; ++i) mask->SetValue(x, y + i, true);
        }
      }
    }
  }
}

template <size_t Length>
void SumThreshold::HorizontalLarge(const Image2D* input, Mask2D* mask,
                                   Mask2D* scratch, num_t threshold) {
  *scratch = *mask;
  const size_t width = mask->Width(), height = mask->Height();
  if (Length <= width) {
    for (size_t y = 0; y < height; ++y) {
      num_t sum = 0.0;
      size_t count = 0, xLeft, xRight;

      for (xRight = 0; xRight < Length - 1; ++xRight) {
        if (!mask->Value(xRight, y)) {
          sum += input->Value(xRight, y);
          count++;
        }
      }

      xLeft = 0;
      while (xRight < width) {
        // add the sample at the right
        if (!mask->Value(xRight, y)) {
          sum += input->Value(xRight, y);
          ++count;
        }
        // Check
        if (count > 0 && std::fabs(sum / count) > threshold) {
          scratch->SetHorizontalValues(xLeft, y, true, Length);
        }
        // subtract the sample at the left
        if (!mask->Value(xLeft, y)) {
          sum -= input->Value(xLeft, y);
          --count;
        }
        ++xLeft;
        ++xRight;
      }
    }
  }
  *mask = std::move(*scratch);
}

template <size_t Length>
void SumThreshold::VerticalLarge(const Image2D* input, Mask2D* mask,
                                 Mask2D* scratch, num_t threshold) {
  *scratch = *mask;
  const size_t width = mask->Width(), height = mask->Height();
  if (Length <= height) {
    for (size_t x = 0; x < width; ++x) {
      num_t sum = 0.0;
      size_t count = 0, yTop, yBottom;

      for (yBottom = 0; yBottom < Length - 1; ++yBottom) {
        if (!mask->Value(x, yBottom)) {
          sum += input->Value(x, yBottom);
          ++count;
        }
      }

      yTop = 0;
      while (yBottom < height) {
        // add the sample at the bottom
        if (!mask->Value(x, yBottom)) {
          sum += input->Value(x, yBottom);
          ++count;
        }
        // Check
        if (count > 0 && std::fabs(sum / count) > threshold) {
          for (size_t i = 0; i < Length; ++i)
            scratch->SetValue(x, yTop + i, true);
        }
        // subtract the sample at the top
        if (!mask->Value(x, yTop)) {
          sum -= input->Value(x, yTop);
          --count;
        }
        ++yTop;
        ++yBottom;
      }
    }
  }
  *mask = std::move(*scratch);
}

void SumThreshold::HorizontalLargeReference(const Image2D* input, Mask2D* mask,
                                            Mask2D* scratch, size_t length,
                                            num_t threshold) {
  switch (length) {
    case 1:
      Horizontal<1>(input, mask, threshold);
      break;
    case 2:
      HorizontalLarge<2>(input, mask, scratch, threshold);
      break;
    case 4:
      HorizontalLarge<4>(input, mask, scratch, threshold);
      break;
    case 8:
      HorizontalLarge<8>(input, mask, scratch, threshold);
      break;
    case 16:
      HorizontalLarge<16>(input, mask, scratch, threshold);
      break;
    case 32:
      HorizontalLarge<32>(input, mask, scratch, threshold);
      break;
    case 64:
      HorizontalLarge<64>(input, mask, scratch, threshold);
      break;
    case 128:
      HorizontalLarge<128>(input, mask, scratch, threshold);
      break;
    case 256:
      HorizontalLarge<256>(input, mask, scratch, threshold);
      break;
    default:
      throw std::runtime_error("Invalid value for length");
  }
}

void SumThreshold::VerticalLargeReference(const Image2D* input, Mask2D* mask,
                                          Mask2D* scratch, size_t length,
                                          num_t threshold) {
  switch (length) {
    case 1:
      Vertical<1>(input, mask, threshold);
      break;
    case 2:
      VerticalLarge<2>(input, mask, scratch, threshold);
      break;
    case 4:
      VerticalLarge<4>(input, mask, scratch, threshold);
      break;
    case 8:
      VerticalLarge<8>(input, mask, scratch, threshold);
      break;
    case 16:
      VerticalLarge<16>(input, mask, scratch, threshold);
      break;
    case 32:
      VerticalLarge<32>(input, mask, scratch, threshold);
      break;
    case 64:
      VerticalLarge<64>(input, mask, scratch, threshold);
      break;
    case 128:
      VerticalLarge<128>(input, mask, scratch, threshold);
      break;
    case 256:
      VerticalLarge<256>(input, mask, scratch, threshold);
      break;
    default:
      throw std::runtime_error("Invalid value for length");
  }
}

#if defined(__SSE__) || defined(__x86_64__)

__attribute__((target("sse"))) void SumThreshold::VerticalLargeSSE(
    const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
    num_t threshold) {
  switch (length) {
    case 1:
      Vertical<1>(input, mask, threshold);
      break;
    case 2:
      VerticalLargeSSE<2>(input, mask, scratch, threshold);
      break;
    case 4:
      VerticalLargeSSE<4>(input, mask, scratch, threshold);
      break;
    case 8:
      VerticalLargeSSE<8>(input, mask, scratch, threshold);
      break;
    case 16:
      VerticalLargeSSE<16>(input, mask, scratch, threshold);
      break;
    case 32:
      VerticalLargeSSE<32>(input, mask, scratch, threshold);
      break;
    case 64:
      VerticalLargeSSE<64>(input, mask, scratch, threshold);
      break;
    case 128:
      VerticalLargeSSE<128>(input, mask, scratch, threshold);
      break;
    case 256:
      VerticalLargeSSE<256>(input, mask, scratch, threshold);
      break;
    default:
      throw std::runtime_error("Invalid value for length");
  }
}

__attribute__((target("sse"))) void SumThreshold::HorizontalLargeSSE(
    const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
    num_t threshold) {
  switch (length) {
    case 1:
      Horizontal<1>(input, mask, threshold);
      break;
    case 2:
      HorizontalLargeSSE<2>(input, mask, scratch, threshold);
      break;
    case 4:
      HorizontalLargeSSE<4>(input, mask, scratch, threshold);
      break;
    case 8:
      HorizontalLargeSSE<8>(input, mask, scratch, threshold);
      break;
    case 16:
      HorizontalLargeSSE<16>(input, mask, scratch, threshold);
      break;
    case 32:
      HorizontalLargeSSE<32>(input, mask, scratch, threshold);
      break;
    case 64:
      HorizontalLargeSSE<64>(input, mask, scratch, threshold);
      break;
    case 128:
      HorizontalLargeSSE<128>(input, mask, scratch, threshold);
      break;
    case 256:
      HorizontalLargeSSE<256>(input, mask, scratch, threshold);
      break;
    default:
      throw std::runtime_error("Invalid value for length");
  }
}
#endif  // defined(__SSE__) || defined(__x86_64__)

#if defined(__AVX2__) || defined(__x86_64__)

__attribute__((target("avx2"))) void SumThreshold::VerticalLargeAVX(
    const Image2D* input, Mask2D* mask, Mask2D* scratch, size_t length,
    num_t threshold) {
  switch (length) {
    case 1:
      Vertical<1>(input, mask, threshold);
      break;
    case 2:
      VerticalLargeAVX<2>(input, mask, scratch, threshold);
      break;
    case 4:
      VerticalLargeAVX<4>(input, mask, scratch, threshold);
      break;
    case 8:
      VerticalLargeAVX<8>(input, mask, scratch, threshold);
      break;
    case 16:
      VerticalLargeAVX<16>(input, mask, scratch, threshold);
      break;
    case 32:
      VerticalLargeAVX<32>(input, mask, scratch, threshold);
      break;
    case 64:
      VerticalLargeAVX<64>(input, mask, scratch, threshold);
      break;
    case 128:
      VerticalLargeAVX<128>(input, mask, scratch, threshold);
      break;
    case 256:
      VerticalLargeAVX<256>(input, mask, scratch, threshold);
      break;
    default:
      throw std::runtime_error("Invalid value for length");
  }
}

__attribute__((target("avx2"))) void SumThreshold::HorizontalAVXDumas(
    const Image2D* input, Mask2D* mask, size_t length, num_t threshold) {
  switch (length) {
    case 1:
      HorizontalAVXDumas<1>(input, mask, threshold);
      break;
    case 2:
      HorizontalAVXDumas<2>(input, mask, threshold);
      break;
    case 4:
      HorizontalAVXDumas<4>(input, mask, threshold);
      break;
    case 8:
      HorizontalAVXDumas<8>(input, mask, threshold);
      break;
    case 16:
      HorizontalAVXDumas<16>(input, mask, threshold);
      break;
    case 32:
      HorizontalAVXDumas<32>(input, mask, threshold);
      break;
    case 64:
      HorizontalAVXDumas<64>(input, mask, threshold);
      break;
    case 128:
      HorizontalAVXDumas<128>(input, mask, threshold);
      break;
    case 256:
      HorizontalAVXDumas<256>(input, mask, threshold);
      break;
    default:
      throw std::runtime_error("Invalid value for length");
  }
}

__attribute__((target("avx2"))) void SumThreshold::VerticalAVXDumas(
    const Image2D* input, Mask2D* mask, VerticalScratch* scratch, size_t length,
    num_t threshold) {
  switch (length) {
    case 1:
      VerticalAVXDumas<1>(input, mask, scratch, threshold);
      break;
    case 2:
      VerticalAVXDumas<2>(input, mask, scratch, threshold);
      break;
    case 4:
      VerticalAVXDumas<4>(input, mask, scratch, threshold);
      break;
    case 8:
      VerticalAVXDumas<8>(input, mask, scratch, threshold);
      break;
    case 16:
      VerticalAVXDumas<16>(input, mask, scratch, threshold);
      break;
    case 32:
      VerticalAVXDumas<32>(input, mask, scratch, threshold);
      break;
    case 64:
      VerticalAVXDumas<64>(input, mask, scratch, threshold);
      break;
    case 128:
      VerticalAVXDumas<128>(input, mask, scratch, threshold);
      break;
    case 256:
      VerticalAVXDumas<256>(input, mask, scratch, threshold);
      break;
    default:
      throw std::runtime_error("Invalid value for length");
  }
}

#endif  // defined(__AVX2__) || defined(__x86_64__)

#if defined(__SSE__) || defined(__x86_64__)
/**
 * The SSE version of the Vertical SumThreshold algorithm using intrinsics.
 *
 * The SumThreshold algorithm is applied on 4 time steps at a time. Since the
 * SSE has instructions that operate on 4 floats at a time, this could in theory
 * speed up the processing with a factor of 4. However, a lot of time is lost
 * shuffling the data in the right order/registers, and since a timestep
 * consists of 1 byte booleans and 4 byte floats, there's a penalty. Finally,
 * since the 4 timesteps have to be processed exactly the same way, conditional
 * branches had to be replaced by conditional moves.
 *
 * The average profit of SSE intrinsics vs no SSE seems to be about a factor
 * of 1.5 to 2-3, depending on the Length parameter, but also depending on the
 * number of flags (With Length=256, it can make a factor of 3 difference). It
 * might also vary on different processors; e.g. on my Desktop Xeon with older
 * gcc, the profit was less pronounced, while my Intel i5 at home showed an avg
 * factor of over 2 difference.
 *
 * The algorithm works with Length=1, but since that is a normal thresholding
 * operation, computing a sumthreshold has a lot of overhead, hence is not
 * optimal at that size.
 */
template <size_t Length>
__attribute__((target("sse"))) void SumThreshold::VerticalLargeSSE(
    const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold) {
  *scratch = *mask;
  const size_t width = mask->Width(), height = mask->Height();
  const __m128 zero4 = _mm_set1_ps(0.0);
  const __m128i zero4i = _mm_set1_epi32(0);
  const __m128i ones4 = _mm_set1_epi32(1);
  const __m128 threshold4Pos = _mm_set1_ps(threshold);
  const __m128 threshold4Neg = _mm_set1_ps(-threshold);
  if (Length <= height) {
    for (size_t x = 0; x < width; x += 4) {
      __m128 sum4 = _mm_set1_ps(0.0);
      __m128i count4 = _mm_set1_epi32(0);
      size_t yBottom;

      for (yBottom = 0; yBottom + 1 < Length; ++yBottom) {
        const bool* rowPtr = mask->ValuePtr(x, yBottom);

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        const __m128 conditionMask = _mm_castsi128_ps(_mm_cmpeq_epi32(
            _mm_set_epi32(rowPtr[3], rowPtr[2], rowPtr[1], rowPtr[0]), zero4i));

        // Conditionally increment counters
        count4 = _mm_add_epi32(
            count4, _mm_and_si128(_mm_castps_si128(conditionMask), ones4));

        // Add values with conditional move
        const __m128 m =
            _mm_and_ps(_mm_load_ps(input->ValuePtr(x, yBottom)), conditionMask);
        sum4 =
            _mm_add_ps(sum4, _mm_or_ps(m, _mm_andnot_ps(conditionMask, zero4)));
      }

      size_t yTop = 0;
      while (yBottom < height) {
        // ** Add the 4 sample at the bottom **

        // get a ptr
        const bool* rowPtr = mask->ValuePtr(x, yBottom);

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        __m128 conditionMask = _mm_castsi128_ps(_mm_cmpeq_epi32(
            _mm_set_epi32(rowPtr[3], rowPtr[2], rowPtr[1], rowPtr[0]), zero4i));

        // Conditionally increment counters
        count4 = _mm_add_epi32(
            count4, _mm_and_si128(_mm_castps_si128(conditionMask), ones4));

        // Add values with conditional move
        sum4 = _mm_add_ps(
            sum4, _mm_or_ps(_mm_and_ps(_mm_load_ps(input->ValuePtr(x, yBottom)),
                                       conditionMask),
                            _mm_andnot_ps(conditionMask, zero4)));

        // ** Check sum **

        // if sum/count > threshold || sum/count < -threshold
        const __m128 avg4 = _mm_div_ps(sum4, _mm_cvtepi32_ps(count4));
        const unsigned flagConditions =
            _mm_movemask_ps(_mm_cmpgt_ps(avg4, threshold4Pos)) |
            _mm_movemask_ps(_mm_cmplt_ps(avg4, threshold4Neg));
        // | _mm_movemask_ps(_mm_cmplt_ps(count4, zero4i));

        // The assumption is that most of the values are actually not
        // thresholded, hence, if this is the case, we circumvent the whole loop
        // at the cost of one extra comparison:
        if (flagConditions != 0) {
          union {
            bool theChars[4];
            uint32_t theInt;
          } outputValues = {
              {(flagConditions & 1) != 0, (flagConditions & 2) != 0,
               (flagConditions & 4) != 0, (flagConditions & 8) != 0}};

          for (size_t i = 0; i < Length; ++i) {
            uint32_t* outputPtr =
                reinterpret_cast<uint32_t*>(scratch->ValuePtr(x, yTop + i));
            *outputPtr |= outputValues.theInt;
          }
        }

        // ** Subtract the sample at the top **

        // get a ptr
        const bool* tRowPtr = mask->ValuePtr(x, yTop);

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        conditionMask = _mm_castsi128_ps(_mm_cmpeq_epi32(
            _mm_set_epi32(tRowPtr[3], tRowPtr[2], tRowPtr[1], tRowPtr[0]),
            zero4i));

        // Conditionally decrement counters
        count4 = _mm_sub_epi32(
            count4, _mm_and_si128(_mm_castps_si128(conditionMask), ones4));

        // Subtract values with conditional move
        sum4 = _mm_sub_ps(
            sum4, _mm_or_ps(_mm_and_ps(_mm_load_ps(input->ValuePtr(x, yTop)),
                                       conditionMask),
                            _mm_andnot_ps(conditionMask, zero4)));

        // ** Next... **
        ++yTop;
        ++yBottom;
      }
    }
  }
  std::swap(*mask, *scratch);
}

template <size_t Length>
__attribute__((target("sse"))) void SumThreshold::HorizontalLargeSSE(
    const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold) {
  // The idea of the horizontal SSE version is to read four ('y') rows and
  // process them simultaneously.

  // Currently, this SSE horizontal version is not significant faster
  // (less than ~3%) than the
  // Non-SSE horizontal version. This has probably to do with
  // rather randomly reading through the set (first (0,0)-(0,3), then
  // (1,0)-(1,3), etc) this introduces cache misses and/or many smaller reading
  // requests

  *scratch = *mask;
  const size_t width = mask->Width(), height = mask->Height();
  const __m128 zero4 = _mm_set1_ps(0.0);
  const __m128i zero4i = _mm_set1_epi32(0);
  const __m128i ones4 = _mm_set1_epi32(1);
  const __m128 threshold4Pos = _mm_set1_ps(threshold);
  const __m128 threshold4Neg = _mm_set1_ps(-threshold);
  if (Length <= width) {
    for (size_t y = 0; y < height; y += 4) {
      __m128 sum4 = _mm_set1_ps(0.0);
      __m128i count4 = _mm_set1_epi32(0);
      size_t xRight;

      const bool *rFlagPtrA = mask->ValuePtr(0, y + 3),
                 *rFlagPtrB = mask->ValuePtr(0, y + 2),
                 *rFlagPtrC = mask->ValuePtr(0, y + 1),
                 *rFlagPtrD = mask->ValuePtr(0, y);
      const num_t *rValPtrA = input->ValuePtr(0, y + 3),
                  *rValPtrB = input->ValuePtr(0, y + 2),
                  *rValPtrC = input->ValuePtr(0, y + 1),
                  *rValPtrD = input->ValuePtr(0, y);

      for (xRight = 0; xRight + 1 < Length; ++xRight) {
        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        const __m128 conditionMask = _mm_castsi128_ps(_mm_cmpeq_epi32(
            _mm_set_epi32(*rFlagPtrA, *rFlagPtrB, *rFlagPtrC, *rFlagPtrD),
            zero4i));

        // Conditionally increment counters (nr unflagged samples)
        count4 = _mm_add_epi32(
            count4, _mm_and_si128(_mm_castps_si128(conditionMask), ones4));

        // Load 4 samples
        const __m128 v = _mm_set_ps(*rValPtrA, *rValPtrB, *rValPtrC, *rValPtrD);

        // Add values with conditional move
        sum4 = _mm_add_ps(sum4, _mm_or_ps(_mm_and_ps(v, conditionMask),
                                          _mm_andnot_ps(conditionMask, zero4)));

        ++rFlagPtrA;
        ++rFlagPtrB;
        ++rFlagPtrC;
        ++rFlagPtrD;

        ++rValPtrA;
        ++rValPtrB;
        ++rValPtrC;
        ++rValPtrD;
      }

      size_t xLeft = 0;
      const bool *lFlagPtrA = mask->ValuePtr(0, y + 3),
                 *lFlagPtrB = mask->ValuePtr(0, y + 2),
                 *lFlagPtrC = mask->ValuePtr(0, y + 1),
                 *lFlagPtrD = mask->ValuePtr(0, y);
      const num_t *lValPtrA = input->ValuePtr(0, y + 3),
                  *lValPtrB = input->ValuePtr(0, y + 2),
                  *lValPtrC = input->ValuePtr(0, y + 1),
                  *lValPtrD = input->ValuePtr(0, y);

      while (xRight < width) {
        // ** Add the sample at the right **

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        __m128 conditionMask = _mm_castsi128_ps(_mm_cmpeq_epi32(
            _mm_set_epi32(*rFlagPtrA, *rFlagPtrB, *rFlagPtrC, *rFlagPtrD),
            zero4i));

        // Conditionally increment counters
        count4 = _mm_add_epi32(
            count4, _mm_and_si128(_mm_castps_si128(conditionMask), ones4));

        // Load 4 samples
        __m128 v = _mm_set_ps(*rValPtrA, *rValPtrB, *rValPtrC, *rValPtrD);

        // Add values with conditional move (sum4 += (v & m) | (m & ~0) ).
        sum4 = _mm_add_ps(sum4, _mm_or_ps(_mm_and_ps(v, conditionMask),
                                          _mm_andnot_ps(conditionMask, zero4)));

        // ** Check sum **

        // if sum/count > threshold || sum/count < -threshold
        const __m128 count4AsSingle = _mm_cvtepi32_ps(count4);
        const unsigned flagConditions =
            _mm_movemask_ps(
                _mm_cmpgt_ps(_mm_div_ps(sum4, count4AsSingle), threshold4Pos)) |
            _mm_movemask_ps(
                _mm_cmplt_ps(_mm_div_ps(sum4, count4AsSingle), threshold4Neg));

        if ((flagConditions & 1) != 0)
          scratch->SetHorizontalValues(xLeft, y, true, Length);
        if ((flagConditions & 2) != 0)
          scratch->SetHorizontalValues(xLeft, y + 1, true, Length);
        if ((flagConditions & 4) != 0)
          scratch->SetHorizontalValues(xLeft, y + 2, true, Length);
        if ((flagConditions & 8) != 0)
          scratch->SetHorizontalValues(xLeft, y + 3, true, Length);

        // ** Subtract the sample at the left **

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        conditionMask = _mm_castsi128_ps(_mm_cmpeq_epi32(
            _mm_set_epi32(*lFlagPtrA, *lFlagPtrB, *lFlagPtrC, *lFlagPtrD),
            zero4i));

        // Conditionally decrement counters
        count4 = _mm_sub_epi32(
            count4, _mm_and_si128(_mm_castps_si128(conditionMask), ones4));

        // Load 4 samples
        v = _mm_set_ps(*lValPtrA, *lValPtrB, *lValPtrC, *lValPtrD);

        // Subtract values with conditional move
        sum4 = _mm_sub_ps(sum4, _mm_or_ps(_mm_and_ps(v, conditionMask),
                                          _mm_andnot_ps(conditionMask, zero4)));

        // ** Next... **
        ++xLeft;
        ++xRight;

        ++rFlagPtrA;
        ++rFlagPtrB;
        ++rFlagPtrC;
        ++rFlagPtrD;

        ++lFlagPtrA;
        ++lFlagPtrB;
        ++lFlagPtrC;
        ++lFlagPtrD;

        ++rValPtrA;
        ++rValPtrB;
        ++rValPtrC;
        ++rValPtrD;

        ++lValPtrA;
        ++lValPtrB;
        ++lValPtrC;
        ++lValPtrD;
      }
    }
  }
  std::swap(*mask, *scratch);
}

template void SumThreshold::VerticalLargeSSE<2>(const Image2D* input,
                                                Mask2D* mask, Mask2D* scratch,
                                                num_t threshold);
template void SumThreshold::VerticalLargeSSE<4>(const Image2D* input,
                                                Mask2D* mask, Mask2D* scratch,
                                                num_t threshold);
template void SumThreshold::VerticalLargeSSE<8>(const Image2D* input,
                                                Mask2D* mask, Mask2D* scratch,
                                                num_t threshold);
template void SumThreshold::VerticalLargeSSE<16>(const Image2D* input,
                                                 Mask2D* mask, Mask2D* scratch,
                                                 num_t threshold);
template void SumThreshold::VerticalLargeSSE<32>(const Image2D* input,
                                                 Mask2D* mask, Mask2D* scratch,
                                                 num_t threshold);
template void SumThreshold::VerticalLargeSSE<64>(const Image2D* input,
                                                 Mask2D* mask, Mask2D* scratch,
                                                 num_t threshold);
template void SumThreshold::VerticalLargeSSE<128>(const Image2D* input,
                                                  Mask2D* mask, Mask2D* scratch,
                                                  num_t threshold);
template void SumThreshold::VerticalLargeSSE<256>(const Image2D* input,
                                                  Mask2D* mask, Mask2D* scratch,
                                                  num_t threshold);

template void SumThreshold::HorizontalLargeSSE<2>(const Image2D* input,
                                                  Mask2D* mask, Mask2D* scratch,
                                                  num_t threshold);
template void SumThreshold::HorizontalLargeSSE<4>(const Image2D* input,
                                                  Mask2D* mask, Mask2D* scratch,
                                                  num_t threshold);
template void SumThreshold::HorizontalLargeSSE<8>(const Image2D* input,
                                                  Mask2D* mask, Mask2D* scratch,
                                                  num_t threshold);
template void SumThreshold::HorizontalLargeSSE<16>(const Image2D* input,
                                                   Mask2D* mask,
                                                   Mask2D* scratch,
                                                   num_t threshold);
template void SumThreshold::HorizontalLargeSSE<32>(const Image2D* input,
                                                   Mask2D* mask,
                                                   Mask2D* scratch,
                                                   num_t threshold);
template void SumThreshold::HorizontalLargeSSE<64>(const Image2D* input,
                                                   Mask2D* mask,
                                                   Mask2D* scratch,
                                                   num_t threshold);
template void SumThreshold::HorizontalLargeSSE<128>(const Image2D* input,
                                                    Mask2D* mask,
                                                    Mask2D* scratch,
                                                    num_t threshold);
template void SumThreshold::HorizontalLargeSSE<256>(const Image2D* input,
                                                    Mask2D* mask,
                                                    Mask2D* scratch,
                                                    num_t threshold);

#endif

#if defined(__AVX2__) || defined(__x86_64__)

#include <immintrin.h>

template <size_t Length>
__attribute__((target("avx2"))) void SumThreshold::VerticalLargeAVX(
    const Image2D* input, Mask2D* mask, Mask2D* scratch, num_t threshold) {
  *scratch = *mask;
  const size_t width = mask->Width(), height = mask->Height();
  const __m256 zero8 = _mm256_set1_ps(0.0);
  const __m256i zero8i = _mm256_set1_epi32(0);
  const __m256i ones8 = _mm256_set1_epi32(1);
  const __m256 threshold8Pos = _mm256_set1_ps(threshold);
  const __m256 threshold8Neg = _mm256_set1_ps(-threshold);
  if (Length <= height) {
    size_t x = 0;
    for (; x + 4 < width; x += 8) {
      __m256 sum8 = _mm256_set1_ps(0.0);
      __m256i count8 = _mm256_set1_epi32(0);
      size_t yBottom;

      for (yBottom = 0; yBottom + 1 < Length; ++yBottom) {
        const bool* rowPtr = mask->ValuePtr(x, yBottom);

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        const __m256 conditionMask = _mm256_castsi256_ps(_mm256_cmpeq_epi32(
            _mm256_set_epi32(rowPtr[7], rowPtr[6], rowPtr[5], rowPtr[4],
                             rowPtr[3], rowPtr[2], rowPtr[1], rowPtr[0]),
            zero8i));

        // Conditionally increment counters
        count8 = _mm256_add_epi32(
            count8,
            _mm256_and_si256(_mm256_castps_si256(conditionMask), ones8));

        // Add values with conditional move
        const __m256 m = _mm256_and_ps(
            _mm256_load_ps(input->ValuePtr(x, yBottom)), conditionMask);
        sum8 = _mm256_add_ps(
            sum8, _mm256_or_ps(m, _mm256_andnot_ps(conditionMask, zero8)));
      }

      size_t yTop = 0;
      while (yBottom < height) {
        // ** Add the 8 sample at the bottom **

        // get a ptr
        const bool* rowPtr = mask->ValuePtr(x, yBottom);

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        __m256 conditionMask = _mm256_castsi256_ps(_mm256_cmpeq_epi32(
            _mm256_set_epi32(rowPtr[7], rowPtr[6], rowPtr[5], rowPtr[4],
                             rowPtr[3], rowPtr[2], rowPtr[1], rowPtr[0]),
            zero8i));

        // Conditionally increment counters
        count8 = _mm256_add_epi32(
            count8,
            _mm256_and_si256(_mm256_castps_si256(conditionMask), ones8));

        // Add values with conditional move
        sum8 = _mm256_add_ps(
            sum8, _mm256_or_ps(
                      _mm256_and_ps(_mm256_load_ps(input->ValuePtr(x, yBottom)),
                                    conditionMask),
                      _mm256_andnot_ps(conditionMask, zero8)));

        // ** Check sum **

        // if sum/count > threshold || sum/count < -threshold
        const __m256 avg8 = _mm256_div_ps(sum8, _mm256_cvtepi32_ps(count8));
        // float a[8];
        // _mm256_storeu_ps(a, avg8);
        // std::cout << a[0] << '\n';
        const int flagConditions =
            _mm256_movemask_ps(_mm256_cmp_ps(avg8, threshold8Pos, _CMP_GT_OQ)) |
            _mm256_movemask_ps(_mm256_cmp_ps(avg8, threshold8Neg, _CMP_LT_OQ));

        // The assumption is that most of the values are actually not
        // thresholded, hence, if this is the case, we circumvent the whole loop
        // at the cost of one extra comparison:
        if (flagConditions != 0) {
          // std::cout << "y=" << yTop << ", flagConditions = " <<
          // flagConditions << "\n";
          union {
            bool theChars[8];
            uint64_t theInt;
          } outputValues = {
              {(flagConditions & 1) != 0, (flagConditions & 2) != 0,
               (flagConditions & 4) != 0, (flagConditions & 8) != 0,
               (flagConditions & 16) != 0, (flagConditions & 32) != 0,
               (flagConditions & 64) != 0, (flagConditions & 128) != 0}};

          for (size_t i = 0; i < Length; ++i) {
            uint64_t* outputPtr =
                reinterpret_cast<uint64_t*>(scratch->ValuePtr(x, yTop + i));
            *outputPtr |= outputValues.theInt;
          }
        }

        // ** Subtract the sample at the top **

        // get a ptr
        const bool* tRowPtr = mask->ValuePtr(x, yTop);

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        conditionMask = _mm256_castsi256_ps(_mm256_cmpeq_epi32(
            _mm256_set_epi32(tRowPtr[7], tRowPtr[6], tRowPtr[5], tRowPtr[4],
                             tRowPtr[3], tRowPtr[2], tRowPtr[1], tRowPtr[0]),
            zero8i));

        // Conditionally decrement counters
        count8 = _mm256_sub_epi32(
            count8,
            _mm256_and_si256(_mm256_castps_si256(conditionMask), ones8));

        // Subtract values with conditional move
        sum8 = _mm256_sub_ps(
            sum8,
            _mm256_or_ps(_mm256_and_ps(_mm256_load_ps(input->ValuePtr(x, yTop)),
                                       conditionMask),
                         _mm256_andnot_ps(conditionMask, zero8)));

        // ** Next... **
        ++yTop;
        ++yBottom;
      }
    }

    // =============================================================================
    // Since images are aligned on 4 values and we are stepping with 8 values,
    // there might be 4 values left. These are done with the 'SSE' algorithm.
    // (this code is a literal copy of the above SSE algorithm)
    // =============================================================================
    if (x < width) {
      const __m128i zero4i = _mm_set1_epi32(0);
      const __m128i ones4 = _mm_set1_epi32(1);
      const __m128 zero4 = _mm_set1_ps(0.0);
      const __m128 threshold4Pos = _mm_set1_ps(threshold);
      const __m128 threshold4Neg = _mm_set1_ps(-threshold);
      __m128 sum4 = _mm_set1_ps(0.0);
      __m128i count4 = _mm_set1_epi32(0);
      size_t yBottom;

      for (yBottom = 0; yBottom + 1 < Length; ++yBottom) {
        const bool* rowPtr = mask->ValuePtr(x, yBottom);

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        const __m128 conditionMask = _mm_castsi128_ps(_mm_cmpeq_epi32(
            _mm_set_epi32(rowPtr[3], rowPtr[2], rowPtr[1], rowPtr[0]), zero4i));

        // Conditionally increment counters
        count4 = _mm_add_epi32(
            count4, _mm_and_si128(_mm_castps_si128(conditionMask), ones4));

        // Add values with conditional move
        const __m128 m =
            _mm_and_ps(_mm_load_ps(input->ValuePtr(x, yBottom)), conditionMask);
        sum4 =
            _mm_add_ps(sum4, _mm_or_ps(m, _mm_andnot_ps(conditionMask, zero4)));
      }

      size_t yTop = 0;
      while (yBottom < height) {
        // ** Add the 4 sample at the bottom **

        // get a ptr
        const bool* rowPtr = mask->ValuePtr(x, yBottom);

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        __m128 conditionMask = _mm_castsi128_ps(_mm_cmpeq_epi32(
            _mm_set_epi32(rowPtr[3], rowPtr[2], rowPtr[1], rowPtr[0]), zero4i));

        // Conditionally increment counters
        count4 = _mm_add_epi32(
            count4, _mm_and_si128(_mm_castps_si128(conditionMask), ones4));

        // Add values with conditional move
        sum4 = _mm_add_ps(
            sum4, _mm_or_ps(_mm_and_ps(_mm_load_ps(input->ValuePtr(x, yBottom)),
                                       conditionMask),
                            _mm_andnot_ps(conditionMask, zero4)));

        // ** Check sum **

        // if sum/count > threshold || sum/count < -threshold
        const __m128 avg4 = _mm_div_ps(sum4, _mm_cvtepi32_ps(count4));
        const unsigned flagConditions =
            _mm_movemask_ps(_mm_cmpgt_ps(avg4, threshold4Pos)) |
            _mm_movemask_ps(_mm_cmplt_ps(avg4, threshold4Neg));
        // | _mm_movemask_ps(_mm_cmplt_ps(count4, zero4i));

        // The assumption is that most of the values are actually not
        // thresholded, hence, if this is the case, we circumvent the whole loop
        // at the cost of one extra comparison:
        if (flagConditions != 0) {
          union {
            bool theChars[4];
            unsigned theInt;
          } outputValues = {
              {(flagConditions & 1) != 0, (flagConditions & 2) != 0,
               (flagConditions & 4) != 0, (flagConditions & 8) != 0}};

          for (size_t i = 0; i < Length; ++i) {
            unsigned* outputPtr =
                reinterpret_cast<unsigned*>(scratch->ValuePtr(x, yTop + i));

            *outputPtr |= outputValues.theInt;
          }
        }

        // ** Subtract the sample at the top **

        // get a ptr
        const bool* tRowPtr = mask->ValuePtr(x, yTop);

        // Assign each integer to one bool in the mask
        // Convert true to 0xFFFFFFFF and false to 0
        conditionMask = _mm_castsi128_ps(_mm_cmpeq_epi32(
            _mm_set_epi32(tRowPtr[3], tRowPtr[2], tRowPtr[1], tRowPtr[0]),
            zero4i));

        // Conditionally decrement counters
        count4 = _mm_sub_epi32(
            count4, _mm_and_si128(_mm_castps_si128(conditionMask), ones4));

        // Subtract values with conditional move
        sum4 = _mm_sub_ps(
            sum4, _mm_or_ps(_mm_and_ps(_mm_load_ps(input->ValuePtr(x, yTop)),
                                       conditionMask),
                            _mm_andnot_ps(conditionMask, zero4)));

        // ** Next... **
        ++yTop;
        ++yBottom;
      }
    }
  }
  std::swap(*mask, *scratch);
}

template void SumThreshold::VerticalLargeAVX<2>(const Image2D* input,
                                                Mask2D* mask, Mask2D* scratch,
                                                num_t threshold);
template void SumThreshold::VerticalLargeAVX<4>(const Image2D* input,
                                                Mask2D* mask, Mask2D* scratch,
                                                num_t threshold);
template void SumThreshold::VerticalLargeAVX<8>(const Image2D* input,
                                                Mask2D* mask, Mask2D* scratch,
                                                num_t threshold);
template void SumThreshold::VerticalLargeAVX<16>(const Image2D* input,
                                                 Mask2D* mask, Mask2D* scratch,
                                                 num_t threshold);
template void SumThreshold::VerticalLargeAVX<32>(const Image2D* input,
                                                 Mask2D* mask, Mask2D* scratch,
                                                 num_t threshold);
template void SumThreshold::VerticalLargeAVX<64>(const Image2D* input,
                                                 Mask2D* mask, Mask2D* scratch,
                                                 num_t threshold);
template void SumThreshold::VerticalLargeAVX<128>(const Image2D* input,
                                                  Mask2D* mask, Mask2D* scratch,
                                                  num_t threshold);
template void SumThreshold::VerticalLargeAVX<256>(const Image2D* input,
                                                  Mask2D* mask, Mask2D* scratch,
                                                  num_t threshold);

template <size_t Length>
__attribute__((target("avx2"))) void SumThreshold::VerticalAVXDumas(
    const Image2D* input, Mask2D* mask, VerticalScratch* scratch,
    num_t threshold) {
  if (Length <= mask->Height()) {
    int* lastFlaggedPos = scratch->lastFlaggedPos.get();
    num_t* sum = scratch->sum.get();
    int* count = scratch->count.get();

    std::fill(lastFlaggedPos, lastFlaggedPos + input->Width(), -1);
    std::fill(sum, sum + input->Width(), 0);
    std::fill(count, count + input->Width(), 0);

    constexpr int vectorWidth = 8;
    const int parallelizableLength =
        (int)mask->Width() - (int)mask->Width() % vectorWidth;
    const __m256 threshold_m256 = _mm256_set1_ps(threshold);
    const __m256 sign_mask_m256 = _mm256_xor_ps(
        _mm256_set1_ps(-0.0f), _mm256_castsi256_ps(_mm256_set1_epi32(-1)));

    // Truncates dwords to bytes for each 256 bit
    const __m256i shuffle_1f126i_cvtepi32_epu8_m256i = _mm256_set_epi8(
        '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF',
        '\xFF', '\xFF', '\xFF', '\x0c', '\x08', '\x04', '\x00', '\xFF', '\xFF',
        '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF', '\xFF',
        '\xFF', '\x0c', '\x08', '\x04', '\x00');

    const __m128i first_dword_true_m128i = _mm_set_epi8(
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x01',
        '\x01', '\x01', '\x01', '\x01', '\x01', '\x01', '\x01');

    constexpr int64_t true_m32i = 0x01010101L;

    // Set sum and count for initial window position
    for (int maxRow = 0; maxRow < (int)Length - 1; ++maxRow) {
      for (int iCol = 0; iCol < parallelizableLength; iCol += vectorWidth) {
        /*
         * Implements:
         *    sum(iCol) += input(maxRow, iCol) * !mask(maxRow, iCol);
         *    count(iCol) += !mask(maxRow, iCol);
         */

        // Load sum vector
        __m256 sum_m256 = _mm256_load_ps(&sum[iCol]);

        // Load count vector
        __m256i count_m256i = _mm256_load_si256((__m256i*)&count[iCol]);

        // Load input vector
        const __m256 input_m256 = _mm256_load_ps(input->ValuePtr(iCol, maxRow));

        // Load mask vector
        const __m64 mask_m64 = *(__m64*)mask->ValuePtr(iCol, maxRow);
        const __m128i mask_m128i = _mm_set_epi64(_mm_setzero_si64(), mask_m64);
        const __m128i imask_m128i =
            _mm_xor_si128(first_dword_true_m128i,
                          mask_m128i);  // Invert mask vector
        const __m256i imask_m256i = _mm256_cvtepu8_epi32(imask_m128i);
        const __m256 imask_m256 = _mm256_cvtepi32_ps(imask_m256i);

        // Sum += input * !mask
        const __m256 tmp_m256 = _mm256_mul_ps(imask_m256, input_m256);
        sum_m256 = _mm256_add_ps(sum_m256, tmp_m256);

        // Store sum
        _mm256_store_ps(&sum[iCol], sum_m256);

        // count += !mask
        count_m256i = _mm256_add_epi32(count_m256i, imask_m256i);

        // Store count
        _mm256_store_si256((__m256i*)&count[iCol], count_m256i);
      }
      for (int iCol = parallelizableLength; iCol < (int)mask->Width(); ++iCol) {
        sum[iCol] += input->Value(iCol, maxRow) * !mask->Value(iCol, maxRow);
        count[iCol] += !mask->Value(iCol, maxRow);
      }
    }

    // Iterate through positions
    for (int maxRow = (int)Length - 1; maxRow < (int)mask->Height(); ++maxRow) {
      const int minRow = maxRow - (int)Length + 1;

      // load maxRow vector
      const __m256i maxRow_m256i = _mm256_set1_epi32(maxRow);

      // Load 'minRow take 1' vector
      const __m256i minRowt1_m256i = _mm256_set1_epi32(minRow - 1);

      for (int iCol = 0; iCol < parallelizableLength; iCol += vectorWidth) {
        // Load sum vector
        __m256 sum_m256 = _mm256_load_ps(&sum[iCol]);

        // Load count vector
        __m256i count_m256i = _mm256_load_si256((__m256i*)&count[iCol]);

        /*
         * Implements:
         *    sum(iCol) += input(maxRow, iCol) * !mask(maxRow, iCol);
         *    count(iCol) += !mask(maxRow, iCol);
         */
        {
          // Load input vector
          const __m256 input_m256 =
              _mm256_load_ps(input->ValuePtr(iCol, maxRow));

          // Load mask vector
          const __m64 mask_m64 = *(__m64*)mask->ValuePtr(iCol, maxRow);
          const __m128i mask_m128i =
              _mm_set_epi64(_mm_setzero_si64(), mask_m64);
          const __m128i imask_m128i = _mm_xor_si128(
              first_dword_true_m128i, mask_m128i);  // Invert mask vector
          const __m256i imask_m256i = _mm256_cvtepu8_epi32(imask_m128i);
          const __m256 imask_m256 = _mm256_cvtepi32_ps(imask_m256i);

          // Sum += input * !mask
          const __m256 tmp_m256 = _mm256_mul_ps(imask_m256, input_m256);
          sum_m256 = _mm256_add_ps(sum_m256, tmp_m256);

          // count += !mask
          count_m256i = _mm256_add_epi32(count_m256i, imask_m256i);
        }

        /*
         * Implements:
         * if (abs(sum(iCol)) > count(iCol) * threshold)
         *    lastFlaggedPos(iCol) = maxRow;
         */
        {
          // cast count
          const __m256 count_m256 = _mm256_cvtepi32_ps(count_m256i);

          // tmp1 = threshold * count
          const __m256 tmp1_m256 = _mm256_mul_ps(threshold_m256, count_m256);

          // tmp2 = abs(sum)
          const __m256 tmp2_m256 = _mm256_and_ps(sum_m256, sign_mask_m256);

          // tmp3 = tmp2 > tmp1
          const __m256 tmp3_m256 =
              _mm256_cmp_ps(tmp2_m256, tmp1_m256, _CMP_GT_OQ);

          // cast tmp3
          const __m256i tmp3_m256i = _mm256_castps_si256(tmp3_m256);

          // store lastFlaggedPos
          _mm256_maskstore_epi32(&lastFlaggedPos[iCol], tmp3_m256i,
                                 maxRow_m256i);
        }

        /*
         * Implements:
         *    sum(iCol) -= input(minRow, iCol) * !mask(minRow, iCol);
         *    count(iCol) -= !mask(minRow, iCol);
         */
        {
          // Load input vector
          const __m256 input_m256 =
              _mm256_load_ps(input->ValuePtr(iCol, minRow));

          // Load mask vector
          const __m64 mask_m64 = *(__m64*)mask->ValuePtr(iCol, minRow);
          const __m128i mask_m128i =
              _mm_set_epi64(_mm_setzero_si64(), mask_m64);
          const __m128i imask_m128i = _mm_xor_si128(
              first_dword_true_m128i, mask_m128i);  // Invert mask vector
          const __m256i imask_m256i = _mm256_cvtepu8_epi32(imask_m128i);
          const __m256 imask_m256 = _mm256_cvtepi32_ps(imask_m256i);

          // Sum -= input * !mask
          const __m256 tmp_m256 = _mm256_mul_ps(imask_m256, input_m256);
          sum_m256 = _mm256_sub_ps(sum_m256, tmp_m256);

          // count -= !mask
          count_m256i = _mm256_sub_epi32(count_m256i, imask_m256i);
        }

        // Store sum
        _mm256_store_ps(&sum[iCol], sum_m256);

        // Store count
        _mm256_store_si256((__m256i*)&count[iCol], count_m256i);

        /*
         * Implements:
         * mask(minRow, iCol) |= (lastFlaggedPos(iCol) > minRow - 1);
         */
        {
          // Load lastFlaggedPos vector
          const __m256i lastFlaggedPos_m256i =
              _mm256_load_si256((__m256i*)&lastFlaggedPos[iCol]);

          __m256i tmp_m256i =
              _mm256_cmpgt_epi32(lastFlaggedPos_m256i, minRowt1_m256i);

          tmp_m256i = _mm256_shuffle_epi8(tmp_m256i,
                                          shuffle_1f126i_cvtepi32_epu8_m256i);

          ((int32_t*)mask->ValuePtr(iCol, minRow))[0] |=
              true_m32i & _mm256_extract_epi32(tmp_m256i, 0);
          ((int32_t*)mask->ValuePtr(iCol, minRow))[1] |=
              true_m32i & _mm256_extract_epi32(tmp_m256i, 4);
        }
      }
      for (int iCol = parallelizableLength; iCol < (int)mask->Width(); ++iCol) {
        const int minRow = maxRow - (int)Length + 1;

        // add the sample at the right
        sum[iCol] += input->Value(iCol, maxRow) * !mask->Value(iCol, maxRow);
        count[iCol] += !mask->Value(iCol, maxRow);

        // Check current pos
        lastFlaggedPos[iCol] += int(abs(sum[iCol]) > count[iCol] * threshold) *
                                (maxRow - lastFlaggedPos[iCol]);

        // subtract the sample past the left
        sum[iCol] -= input->Value(iCol, minRow) * !mask->Value(iCol, minRow);
        count[iCol] -= !mask->Value(iCol, minRow);

        // Flag left edge
        *mask->ValuePtr(iCol, minRow) |= (lastFlaggedPos[iCol] >= minRow);
      }
    }

    // Flag last window
    for (int minRow = (int)mask->Height() - (int)Length + 1;
         minRow < (int)mask->Height(); ++minRow) {
      const __m256i minRowt1_m256i = _mm256_set1_epi32(minRow - 1);
      for (int iCol = 0; iCol < parallelizableLength; iCol += 8) {
        /*
         * Implements:
         *    mask(minRow, iCol) |= (lastFlaggedPos(iCol) > minRow - 1);
         */

        const __m256i lastFlaggedPos_m256i =
            _mm256_load_si256((__m256i*)&lastFlaggedPos[iCol]);

        const __m256i tmp1_m256i =
            _mm256_cmpgt_epi32(lastFlaggedPos_m256i, minRowt1_m256i);

        const __m256i tmp2_m256i =
            _mm256_shuffle_epi8(tmp1_m256i, shuffle_1f126i_cvtepi32_epu8_m256i);

        ((int32_t*)mask->ValuePtr(iCol, minRow))[0] |=
            true_m32i & _mm256_extract_epi32(tmp2_m256i, 0);
        ((int32_t*)mask->ValuePtr(iCol, minRow))[1] |=
            true_m32i & _mm256_extract_epi32(tmp2_m256i, 4);
      }
      for (int iCol = parallelizableLength; iCol < (int)mask->Width(); ++iCol) {
        *mask->ValuePtr(iCol, minRow) |= (lastFlaggedPos[iCol] > minRow - 1);
      }
    }
  }
}

template <size_t Length>
__attribute__((target("avx2"))) void SumThreshold::HorizontalAVXDumas(
    const Image2D* input, Mask2D* mask, num_t threshold) {
  if (Length <= mask->Width()) {
#ifndef NDEBUG
    if (input->Stride() * 7 * sizeof(float) > 0xFFFFFFFF) {
      throw std::runtime_error("Array too big for gather intrinsic");
    }
#endif

    const __m256i gather_input_indexes_m256i =
        _mm256_set_epi32(input->Stride() * 7 * sizeof(float),
                         input->Stride() * 6 * sizeof(float),
                         input->Stride() * 5 * sizeof(float),
                         input->Stride() * 4 * sizeof(float),
                         input->Stride() * 3 * sizeof(float),
                         input->Stride() * 2 * sizeof(float),
                         input->Stride() * 1 * sizeof(float),
                         input->Stride() * 0 * sizeof(float));

    const size_t stride = mask->Stride();
    const __m256i gather_mask_indexes_m256i = _mm256_set_epi32(
        (stride * 7 - 3) * sizeof(bool), (stride * 6 - 2) * sizeof(bool),
        (stride * 5 - 1) * sizeof(bool), stride * 4 * sizeof(bool),
        stride * 3 * sizeof(bool), stride * 2 * sizeof(bool),
        stride * 1 * sizeof(bool), stride * 0 * sizeof(bool));

    const __m256i epi32_lo_set_m256i = _mm256_set_epi32(
        0xFF000000, 0xFF0000, 0xFF00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    const __m256i epi32_1_m256i = _mm256_set_epi32(1, 1, 1, 1, 1, 1, 1, 1);
    constexpr int vectorWidth = 8;
    const __m256 sign_mask_m256 = _mm256_xor_ps(
        _mm256_set1_ps(-0.0f), _mm256_castsi256_ps(_mm256_set1_epi32(-1)));
    const __m256 threshold_m256 = _mm256_set1_ps(threshold);

    for (int iRow = 0;
         iRow < (int)mask->Height() - (int)mask->Height() % vectorWidth;
         iRow += vectorWidth) {
      __m256 sum_m256;
      __m256i count_m256i;
      __m256i lastFlaggedPos_m256i = _mm256_set1_epi32(-1);

      sum_m256 = _mm256_setzero_ps();
      count_m256i = _mm256_setzero_si256();
      // Set sum and count for initial window position
      for (int maxCol = 0; maxCol < (int)Length - 1; ++maxCol) {
        /*
         * Implements:
         *    sum += input(iRow, maxCol) * !mask(iRow, maxCol);
         *    count += !mask(iRow, maxCol);
         */

        const __m256 input_m256 = _mm256_i32gather_ps(
            input->ValuePtr(maxCol, iRow), gather_input_indexes_m256i, 1);
        const __m256i mask_m256i = _mm256_i32gather_epi32(
            (int*)mask->ValuePtr(maxCol, iRow), gather_mask_indexes_m256i, 1);
        const __m256i tmp1_m256i =
            _mm256_and_si256(mask_m256i, epi32_lo_set_m256i);
        const __m256i tmp2_m256i =
            _mm256_cmpeq_epi32(tmp1_m256i, _mm256_setzero_si256());
        const __m256 tmp3_m256 = _mm256_castsi256_ps(tmp2_m256i);
        const __m256 tmp4_m256 = _mm256_and_ps(tmp3_m256, input_m256);

        sum_m256 = _mm256_add_ps(sum_m256, tmp4_m256);

        const __m256i tmp5_m256i = _mm256_and_si256(tmp2_m256i, epi32_1_m256i);

        count_m256i = _mm256_add_epi32(count_m256i, tmp5_m256i);
      }

      // Iterate through positions
      for (int maxCol = (int)Length - 1; maxCol < (int)mask->Width();
           ++maxCol) {
        const int minCol = maxCol - (int)Length + 1;

        /*
         * Implements:
         *    sum += input(iRow, maxCol) * !mask(iRow, maxCol);
         *    count += !mask(iRow,maxCol);
         */
        {
          const __m256 input_m256 = _mm256_i32gather_ps(
              input->ValuePtr(maxCol, iRow), gather_input_indexes_m256i, 1);
          const __m256i mask_m256i = _mm256_i32gather_epi32(
              (int*)mask->ValuePtr(maxCol, iRow), gather_mask_indexes_m256i, 1);
          const __m256i tmp1_m256i =
              _mm256_and_si256(mask_m256i, epi32_lo_set_m256i);
          const __m256i tmp2_m256i =
              _mm256_cmpeq_epi32(tmp1_m256i, _mm256_setzero_si256());
          const __m256 tmp3_m256 = _mm256_castsi256_ps(tmp2_m256i);
          const __m256 tmp4_m256 = _mm256_and_ps(tmp3_m256, input_m256);

          sum_m256 = _mm256_add_ps(sum_m256, tmp4_m256);

          const __m256i tmp5_m256i =
              _mm256_and_si256(tmp2_m256i, epi32_1_m256i);

          count_m256i = _mm256_add_epi32(count_m256i, tmp5_m256i);
        }

        /*
         * Implements:
         * if (abs(sum) > count * threshold)
         *    lastFlaggedPos = maxCol;
         */
        {
          // cast count
          const __m256 count_m256 = _mm256_cvtepi32_ps(count_m256i);

          // tmp1 = threshold * count
          const __m256 tmp1_m256 = _mm256_mul_ps(threshold_m256, count_m256);

          // tmp2 = abs(sum)
          const __m256 tmp2_m256 = _mm256_and_ps(sum_m256, sign_mask_m256);

          // tmp3 = tmp2 > tmp1
          const __m256 tmp3_m256 =
              _mm256_cmp_ps(tmp2_m256, tmp1_m256, _CMP_GT_OQ);

          // cast tmp3
          const __m256i tmp3_m256i = _mm256_castps_si256(tmp3_m256);

          // get maxCol vector`
          const __m256i maxCol_m256i = _mm256_set1_epi32(maxCol);

          // store lastFlaggedPos
          lastFlaggedPos_m256i = _mm256_blendv_epi8(lastFlaggedPos_m256i,
                                                    maxCol_m256i, tmp3_m256i);
        }

        /*
         * Implements:
         *    sum -= input(iRow, minCol) * !mask(iRow, minCol);
         *    count -= !mask(iRow, minCol);
         */
        {
          const __m256 input_m256 = _mm256_i32gather_ps(
              input->ValuePtr(minCol, iRow), gather_input_indexes_m256i, 1);
          const __m256i mask_m256i = _mm256_i32gather_epi32(
              (int*)mask->ValuePtr(minCol, iRow), gather_mask_indexes_m256i, 1);
          const __m256i tmp1_m256i =
              _mm256_and_si256(mask_m256i, epi32_lo_set_m256i);
          const __m256i tmp2_m256i =
              _mm256_cmpeq_epi32(tmp1_m256i, _mm256_setzero_si256());
          const __m256 tmp3_m256 = _mm256_castsi256_ps(tmp2_m256i);
          const __m256 tmp4_m256 = _mm256_and_ps(tmp3_m256, input_m256);

          sum_m256 = _mm256_sub_ps(sum_m256, tmp4_m256);

          const __m256i tmp5_m256i =
              _mm256_and_si256(tmp2_m256i, epi32_1_m256i);

          count_m256i = _mm256_sub_epi32(count_m256i, tmp5_m256i);
        }

        /*
         * Implements:
         *    mask(iRow, minCol) |= (lastFlaggedPos > minCol - 1);
         */
        {
          const __m256i minColt1_m256i = _mm256_set1_epi32(minCol - 1);
          const __m256i masked_m256i =
              _mm256_cmpgt_epi32(lastFlaggedPos_m256i, minColt1_m256i);
          const __m256i boolMask_m256i =
              _mm256_and_si256(masked_m256i, epi32_1_m256i);

          // 5 asm instructions/store with regular bool cast (extract, mov,
          // test, setne, or) 1 asm instructions/store with reinterpret hack
          uint8_t* maskPtr = (uint8_t*)mask->ValuePtr(minCol, iRow);
          maskPtr[mask->Stride() * 0] |=
              static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 0));
          maskPtr[mask->Stride() * 1] |=
              static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 1));
          maskPtr[mask->Stride() * 2] |=
              static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 2));
          maskPtr[mask->Stride() * 3] |=
              static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 3));
          maskPtr[mask->Stride() * 4] |=
              static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 4));
          maskPtr[mask->Stride() * 5] |=
              static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 5));
          maskPtr[mask->Stride() * 6] |=
              static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 6));
          maskPtr[mask->Stride() * 7] |=
              static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 7));
        }
      }

      // Flag last window
      for (int minCol = (int)mask->Width() - Length + 1;
           minCol < (int)mask->Width(); ++minCol) {
        /*
         * Implements:
         *    mask(iRow, minCol) |= (lastFlaggedPos > minCol - 1);
         */

        const __m256i minColt1_m256i = _mm256_set1_epi32(minCol - 1);
        const __m256i masked_m256i =
            _mm256_cmpgt_epi32(lastFlaggedPos_m256i, minColt1_m256i);
        const __m256i boolMask_m256i =
            _mm256_and_si256(masked_m256i, epi32_1_m256i);

        uint8_t* maskPtr = (uint8_t*)mask->ValuePtr(minCol, iRow);

        maskPtr[mask->Stride() * 0] |=
            static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 0));
        maskPtr[mask->Stride() * 1] |=
            static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 1));
        maskPtr[mask->Stride() * 2] |=
            static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 2));
        maskPtr[mask->Stride() * 3] |=
            static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 3));
        maskPtr[mask->Stride() * 4] |=
            static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 4));
        maskPtr[mask->Stride() * 5] |=
            static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 5));
        maskPtr[mask->Stride() * 6] |=
            static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 6));
        maskPtr[mask->Stride() * 7] |=
            static_cast<uint8_t>(_mm256_extract_epi32(boolMask_m256i, 7));
      }
    }

    // non-vectorised remainder
    for (int iRow =
             (int)mask->Height() - (int)mask->Height() % (int)vectorWidth;
         iRow < (int)mask->Height(); ++iRow) {
      int lastFlaggedPos = -1;
      num_t sum = 0;
      int count = 0;

      // Do prefix sum and count
      for (int maxCol = 0; maxCol < (int)Length - 1; ++maxCol) {
        sum += input->Value(maxCol, iRow) * !mask->Value(maxCol, iRow);
        count += !mask->Value(maxCol, iRow);
      }

      // Iterate through positions
      for (int maxCol = (int)Length - 1; maxCol < (int)mask->Width();
           ++maxCol) {
        const int minCol = maxCol - Length + 1;

        // add the sample at the right
        sum += input->Value(maxCol, iRow) * !mask->Value(maxCol, iRow);
        count += !mask->Value(maxCol, iRow);

        // Check current pos
        lastFlaggedPos +=
            int(abs(sum) > count * threshold) * (maxCol - lastFlaggedPos);

        // subtract the sample at the left
        sum -= input->Value(minCol, iRow) * !mask->Value(minCol, iRow);
        count -= !mask->Value(minCol, iRow);

        // Flag left edge
        *mask->ValuePtr(minCol, iRow) |= (lastFlaggedPos >= minCol);
      }

      // Flag last window
      for (int minCol = (int)mask->Width() - (int)Length + 1;
           minCol < (int)mask->Width(); ++minCol) {
        *mask->ValuePtr(minCol, iRow) |= (lastFlaggedPos >= minCol);
      }
    }
  }
}

template void SumThreshold::VerticalAVXDumas<2>(const Image2D* input,
                                                Mask2D* mask,
                                                VerticalScratch* scratch,
                                                num_t threshold);
template void SumThreshold::VerticalAVXDumas<4>(const Image2D* input,
                                                Mask2D* mask,
                                                VerticalScratch* scratch,
                                                num_t threshold);
template void SumThreshold::VerticalAVXDumas<8>(const Image2D* input,
                                                Mask2D* mask,
                                                VerticalScratch* scratch,
                                                num_t threshold);
template void SumThreshold::VerticalAVXDumas<16>(const Image2D* input,
                                                 Mask2D* mask,
                                                 VerticalScratch* scratch,
                                                 num_t threshold);
template void SumThreshold::VerticalAVXDumas<32>(const Image2D* input,
                                                 Mask2D* mask,
                                                 VerticalScratch* scratch,
                                                 num_t threshold);
template void SumThreshold::VerticalAVXDumas<64>(const Image2D* input,
                                                 Mask2D* mask,
                                                 VerticalScratch* scratch,
                                                 num_t threshold);
template void SumThreshold::VerticalAVXDumas<128>(const Image2D* input,
                                                  Mask2D* mask,
                                                  VerticalScratch* scratch,
                                                  num_t threshold);
template void SumThreshold::VerticalAVXDumas<256>(const Image2D* input,
                                                  Mask2D* mask,
                                                  VerticalScratch* scratch,
                                                  num_t threshold);

template void SumThreshold::HorizontalAVXDumas<2>(const Image2D* input,
                                                  Mask2D* mask,
                                                  num_t threshold);
template void SumThreshold::HorizontalAVXDumas<4>(const Image2D* input,
                                                  Mask2D* mask,
                                                  num_t threshold);
template void SumThreshold::HorizontalAVXDumas<8>(const Image2D* input,
                                                  Mask2D* mask,
                                                  num_t threshold);
template void SumThreshold::HorizontalAVXDumas<16>(const Image2D* input,
                                                   Mask2D* mask,
                                                   num_t threshold);
template void SumThreshold::HorizontalAVXDumas<32>(const Image2D* input,
                                                   Mask2D* mask,
                                                   num_t threshold);
template void SumThreshold::HorizontalAVXDumas<64>(const Image2D* input,
                                                   Mask2D* mask,
                                                   num_t threshold);
template void SumThreshold::HorizontalAVXDumas<128>(const Image2D* input,
                                                    Mask2D* mask,
                                                    num_t threshold);
template void SumThreshold::HorizontalAVXDumas<256>(const Image2D* input,
                                                    Mask2D* mask,
                                                    num_t threshold);

#endif  // defined(__AVX2__) || defined(__x86_64__)

}  // namespace algorithms
