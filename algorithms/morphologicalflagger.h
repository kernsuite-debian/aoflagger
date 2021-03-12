#ifndef MORPHOLOGICAL_FLAGGER_H
#define MORPHOLOGICAL_FLAGGER_H

#include <string>

#include "../../structures/mask2d.h"

class MorphologicalFlagger {
 public:
  static inline bool SquareContainsFlag(const Mask2D* mask, size_t xLeft,
                                        size_t yTop, size_t xRight,
                                        size_t yBottom);
  static void DilateFlags(Mask2D* mask, size_t timeSize, size_t frequencySize) {
    DilateFlagsHorizontally(mask, timeSize);
    DilateFlagsVertically(mask, frequencySize);
  }
  static void DilateFlagsHorizontally(Mask2D* mask, size_t timeSize);
  static void DilateFlagsVertically(Mask2D* mask, size_t frequencySize);
  static void LineRemover(Mask2D* mask, size_t maxTimeContamination,
                          size_t maxFreqContamination);

 private:
  static void FlagTime(Mask2D* mask, size_t x);
  static void FlagFrequency(Mask2D* mask, size_t y);
};

#endif
