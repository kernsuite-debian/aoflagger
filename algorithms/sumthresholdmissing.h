#ifndef SUMTHRESHOLD_MISSING_H
#define SUMTHRESHOLD_MISSING_H

#include "../structures/image2d.h"
#include "../structures/mask2d.h"

#include "sumthreshold.h"

#include <vector>

namespace algorithms {

class SumThresholdMissing {
 public:
  struct VerticalCache {
    std::vector<size_t> positions;
    Image2D validImage;
    Mask2D validMask;
    SumThreshold::VerticalScratch scratch;
  };

  static void Horizontal(const Image2D& input, Mask2D& mask,
                         const Mask2D& missing, Mask2D& scratch, size_t length,
                         num_t threshold) {
    horizontal(input, mask, missing, scratch, length, threshold);
  }

  static void VerticalReference(const Image2D& input, Mask2D& mask,
                                const Mask2D& missing, Mask2D& scratch,
                                size_t length, num_t threshold);

  static void InitializeVertical(VerticalCache& cache, const Image2D& input,
                                 const Mask2D& missing);

  static void Vertical(const Image2D& input, Mask2D& mask,
                       const Mask2D& missing, Mask2D& scratch, size_t length,
                       num_t threshold) {
    // VerticalReference(input, mask, missing, scratch, length, threshold);
    // VerticalStacked(cache, input, mask, missing, scratch, length, threshold);
    VerticalConsecutive(input, mask, missing, scratch, length, threshold);
  }

  static void VerticalConsecutive(const Image2D& input, Mask2D& mask,
                                  const Mask2D& missing, Mask2D& scratch,
                                  size_t length, num_t threshold);

  static void VerticalStacked(VerticalCache& cache, const Image2D& input,
                              Mask2D& mask, const Mask2D& missing,
                              Mask2D& scratch, size_t length, num_t threshold);

 private:
  template <typename ImageLike, typename MaskLike, typename CMaskLike>
  static void horizontal(const ImageLike& input, MaskLike& mask,
                         const CMaskLike& missing, MaskLike& scratch,
                         size_t length, num_t threshold);

  SumThresholdMissing() = delete;
};

}  // namespace algorithms

#endif
