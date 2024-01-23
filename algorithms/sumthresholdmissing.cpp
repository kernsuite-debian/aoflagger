#include "sumthresholdmissing.h"

#include "../structures/xyswappedmask2d.h"

#include <vector>
#include <utility>

#include "sumthreshold.h"

namespace algorithms {

template <typename ImageLike, typename MaskLike, typename CMaskLike>
void SumThresholdMissing::horizontal(const ImageLike& input, MaskLike& mask,
                                     const CMaskLike& missing,
                                     MaskLike& scratch, size_t length,
                                     num_t threshold) {
  scratch = mask;
  const size_t width = mask.Width(), height = mask.Height();
  if (length <= width) {
    for (size_t y = 0; y < height; ++y) {
      num_t sum = 0.0;

      // Find first non-missing value for the start of the summation interval
      // xLeft points to the first element of the interval, which is marked as
      // non-missing.
      size_t xLeft = 0;
      while (xLeft != width && missing.Value(xLeft, y)) ++xLeft;

      // xRight points to the last non-missing element of the interval
      size_t xRight = xLeft, countAdded = 0, countTotal = 0;
      while (countTotal + 1 < length && xRight != width) {
        if (!missing.Value(xRight, y)) {
          if (!mask.Value(xRight, y)) {
            sum += input.Value(xRight, y);
            ++countAdded;
          }
          ++countTotal;
        }
        ++xRight;
      }

      while (xRight != width) {
        // Add a sample at the right
        if (!mask.Value(xRight, y)) {
          sum += input.Value(xRight, y);
          ++countAdded;
        }
        // Threshold
        if (countAdded > 0 && std::fabs(sum / countAdded) > threshold) {
          scratch.SetHorizontalValues(xLeft, y, true, xRight - xLeft + 1);
        }
        // subtract one sample at the left
        if (!mask.Value(xLeft, y)) {
          sum -= input.Value(xLeft, y);
          --countAdded;
        }
        do {
          ++xRight;
        } while (xRight != width && missing.Value(xRight, y));
        do {
          ++xLeft;
          // it could happen that xLeft gets to width when the length is one...
          // for other lengths the first test is not necessary.
        } while (xLeft != width && missing.Value(xLeft, y));
      }
    }
  }
  mask = std::move(scratch);
}

template void SumThresholdMissing::horizontal(const Image2D& input,
                                              Mask2D& mask,
                                              const Mask2D& missing,
                                              Mask2D& scratch, size_t length,
                                              num_t threshold);

struct RowData {
  num_t sum = 0.0f;
  size_t yStart = 0;
  size_t nNotFlagged = 0;
  size_t nNotMissing = 0;
};

void SumThresholdMissing::InitializeVertical(VerticalCache& cache,
                                             const Image2D& input,
                                             const Mask2D& missing) {
  cache.positions.assign(input.Width(), 0);
  cache.validImage = Image2D::MakeSetImage(input.Width(), input.Height(), 0.0f);
  cache.validMask = Mask2D::MakeSetMask<false>(input.Width(), input.Height());
  for (size_t y = 0; y != input.Height(); ++y) {
    for (size_t x = 0; x != input.Width(); ++x) {
      if (!missing.Value(x, y)) {
        size_t& pos = cache.positions[x];
        cache.validImage.SetValue(x, pos, input.Value(x, y));
        ++pos;
      }
    }
  }
  cache.scratch = SumThreshold::VerticalScratch(input.Width(), input.Height());
}

void SumThresholdMissing::VerticalStacked(VerticalCache& cache,
                                          const Image2D& input, Mask2D& mask,
                                          const Mask2D& missing,
                                          Mask2D& scratch, size_t length,
                                          num_t threshold) {
  cache.positions.assign(cache.positions.size(), 0);
  for (size_t y = 0; y != input.Height(); ++y) {
    for (size_t x = 0; x != input.Width(); ++x) {
      if (!missing.Value(x, y)) {
        size_t& pos = cache.positions[x];
        cache.validMask.SetValue(x, pos, mask.Value(x, y));
        ++pos;
      }
    }
  }

  SumThreshold::VerticalLarge(&cache.validImage, &cache.validMask, &scratch,
                              &cache.scratch, length, threshold);

  cache.positions.assign(cache.positions.size(), 0);
  for (size_t y = 0; y != input.Height(); ++y) {
    for (size_t x = 0; x != input.Width(); ++x) {
      size_t& pos = cache.positions[x];
      if (!missing.Value(x, y)) {
        if (cache.validMask.Value(x, pos)) mask.SetValue(x, y, true);
        ++pos;
      }
    }
  }
}

void SumThresholdMissing::VerticalConsecutive(const Image2D& input,
                                              Mask2D& mask,
                                              const Mask2D& missing,
                                              Mask2D& scratch, size_t length,
                                              num_t threshold) {
  scratch = mask;
  const size_t width = mask.Width(), height = mask.Height();
  std::vector<RowData> rows(width);
  if (length <= height) {
    for (size_t y = 0; y != height; ++y) {
      for (size_t x = 0; x != width; ++x) {
        RowData& row = rows[x];

        // Add sample if necessary
        if (!missing.Value(x, y)) {
          if (!mask.Value(x, y)) {
            row.sum += input.Value(x, y);
            ++row.nNotFlagged;
          }
          ++row.nNotMissing;
          if (row.nNotMissing == length) {
            // There is a full sequence after having added one more sample:
            // perform threshold and subtract a sample that runs out
            // of the window

            // Threshold
            if (row.nNotFlagged != 0 &&
                std::fabs(row.sum / row.nNotFlagged) > threshold) {
              scratch.SetVerticalValues(x, row.yStart, true,
                                        y - row.yStart + 1);
            }

            // Subtract the oldest sample
            do {
              if (!missing.Value(x, row.yStart)) {
                if (!mask.Value(x, row.yStart)) {
                  row.sum -= input.Value(x, row.yStart);
                  --row.nNotFlagged;
                }
                --row.nNotMissing;
              }
              ++row.yStart;
            } while (row.nNotMissing == length);
          }
        }
      }
    }
  }
  mask = std::move(scratch);
}

void SumThresholdMissing::VerticalReference(const Image2D& input, Mask2D& mask,
                                            const Mask2D& missing,
                                            Mask2D& scratch, size_t length,
                                            num_t threshold) {
  const XYSwappedImage2D<const Image2D> swappedInput(input);
  XYSwappedMask2D<Mask2D> swappedMask(mask);
  const XYSwappedMask2D<const Mask2D> swappedMissing(missing);
  XYSwappedMask2D<Mask2D> swappedScratch(scratch);
  horizontal(swappedInput, swappedMask, swappedMissing, swappedScratch, length,
             threshold);
}

}  // namespace algorithms
