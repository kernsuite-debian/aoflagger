#include "siroperator.h"

namespace algorithms {

template <typename MaskLikeA, typename MaskLikeB>
void SIROperator::operateHorizontallyMissing(MaskLikeA& mask,
                                             const MaskLikeB& missing,
                                             num_t eta) {
  const unsigned width = mask.Width(), maxWSize = width + 1;
  std::unique_ptr<num_t[]> values(new num_t[width]), w(new num_t[maxWSize]);
  std::unique_ptr<unsigned[]> minIndices(new unsigned[maxWSize]),
      maxIndices(new unsigned[maxWSize]);

  for (unsigned row = 0; row < mask.Height(); ++row) {
    unsigned nAvailable = 0;
    for (unsigned i = 0; i < width; ++i) {
      if (!missing.Value(i, row)) {
        if (mask.Value(i, row))
          values[nAvailable] = eta;
        else
          values[nAvailable] = eta - 1.0;
        ++nAvailable;
      }
    }

    if (nAvailable != 0) {
      unsigned wSize = nAvailable + 1;
      w[0] = 0.0;
      unsigned currentMinIndex = 0;
      minIndices[0] = 0;
      for (unsigned i = 1; i != wSize; ++i) {
        w[i] = w[i - 1] + values[i - 1];

        if (w[i] < w[currentMinIndex]) {
          currentMinIndex = i;
        }
        minIndices[i] = currentMinIndex;
      }

      // Calculate the maximum suffixes
      unsigned currentMaxIndex = wSize - 1;
      for (unsigned i = nAvailable - 1; i != 0; --i) {
        maxIndices[i] = currentMaxIndex;
        if (w[i] > w[currentMaxIndex]) {
          currentMaxIndex = i;
        }
      }
      maxIndices[0] = currentMaxIndex;

      // See if max sequence exceeds limit.
      nAvailable = 0;
      for (unsigned i = 0; i != width; ++i) {
        if (!missing.Value(i, row)) {
          const num_t maxW =
              w[maxIndices[nAvailable]] - w[minIndices[nAvailable]];
          mask.SetValue(i, row, (maxW >= 0.0));
          ++nAvailable;
        }
      }
    }
  }
}

template void SIROperator::operateHorizontallyMissing(Mask2D& mask,
                                                      const Mask2D& missing,
                                                      num_t eta);
template void SIROperator::operateHorizontallyMissing(
    XYSwappedMask2D<Mask2D>& mask, const XYSwappedMask2D<const Mask2D>& missing,
    num_t eta);

template <typename MaskLikeA, typename MaskLikeB>
void SIROperator::operateHorizontallyMissing(MaskLikeA& mask,
                                             const MaskLikeB& missing,
                                             num_t eta, num_t penalty) {
  const size_t width = mask.Width();
  const size_t maxWSize = width + 1;
  std::unique_ptr<num_t[]> values(new num_t[width]);
  std::unique_ptr<num_t[]> w(new num_t[maxWSize]);
  std::unique_ptr<size_t[]> minIndices(new size_t[maxWSize]);
  std::unique_ptr<size_t[]> maxIndices(new size_t[maxWSize]);

  const num_t penaltyValue = (eta - 1.0) * penalty;
  for (size_t row = 0; row < mask.Height(); ++row) {
    for (size_t i = 0; i != width; ++i) {
      if (missing.Value(i, row))
        values[i] = penaltyValue;
      else if (mask.Value(i, row))
        values[i] = eta;
      else
        values[i] = eta - 1.0;
    }

    w[0] = 0.0;
    size_t currentMinIndex = 0;
    minIndices[0] = 0;
    for (size_t i = 1; i != width + 1; ++i) {
      w[i] = w[i - 1] + values[i - 1];

      if (w[i] < w[currentMinIndex]) {
        currentMinIndex = i;
      }
      minIndices[i] = currentMinIndex;
    }

    // Calculate the maximum suffixes
    size_t currentMaxIndex = width;
    for (size_t i = width - 1; i != 0; --i) {
      maxIndices[i] = currentMaxIndex;
      if (w[i] > w[currentMaxIndex]) {
        currentMaxIndex = i;
      }
    }
    maxIndices[0] = currentMaxIndex;

    // See if max sequence exceeds limit.
    for (size_t i = 0; i != width; ++i) {
      if (!missing.Value(i, row)) {
        const num_t maxW = w[maxIndices[i]] - w[minIndices[i]];
        mask.SetValue(i, row, maxW >= 0.0);
      }
    }
  }
}

template void SIROperator::operateHorizontallyMissing(Mask2D& mask,
                                                      const Mask2D& missing,
                                                      num_t eta, num_t penalty);
template void SIROperator::operateHorizontallyMissing(
    XYSwappedMask2D<Mask2D>& mask, const XYSwappedMask2D<const Mask2D>& missing,
    num_t eta, num_t penalty);

}  // namespace algorithms
