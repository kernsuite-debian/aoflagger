#include "morphologicalflagger.h"

bool MorphologicalFlagger::SquareContainsFlag(const Mask2D* mask, size_t xLeft,
                                              size_t yTop, size_t xRight,
                                              size_t yBottom) {
  for (size_t y = yTop; y <= yBottom; ++y) {
    for (size_t x = xLeft; x <= xRight; ++x) {
      if (mask->Value(x, y)) return true;
    }
  }
  return false;
}

void MorphologicalFlagger::DilateFlagsHorizontally(Mask2D* mask,
                                                   size_t timeSize) {
  if (timeSize != 0) {
    Mask2D destination(Mask2D::MakeUnsetMask(mask->Width(), mask->Height()));
    if (timeSize > mask->Width()) timeSize = mask->Width();
    const int intSize = (int)timeSize;

    for (size_t y = 0; y < mask->Height(); ++y) {
      int dist = intSize + 1;
      for (size_t x = 0; x < timeSize; ++x) {
        if (mask->Value(x, y)) dist = -intSize;
        dist++;
      }
      for (size_t x = 0; x < mask->Width() - timeSize; ++x) {
        if (mask->Value(x + timeSize, y)) dist = -intSize;
        if (dist <= intSize) {
          destination.SetValue(x, y, true);
          dist++;
        } else {
          destination.SetValue(x, y, false);
        }
      }
      for (size_t x = mask->Width() - timeSize; x < mask->Width(); ++x) {
        if (dist <= intSize) {
          destination.SetValue(x, y, true);
          dist++;
        } else {
          destination.SetValue(x, y, false);
        }
      }
    }
    *mask = std::move(destination);
  }
}

void MorphologicalFlagger::DilateFlagsVertically(Mask2D* mask,
                                                 size_t frequencySize) {
  if (frequencySize != 0) {
    Mask2D destination(Mask2D::MakeUnsetMask(mask->Width(), mask->Height()));
    if (frequencySize > mask->Height()) frequencySize = mask->Height();
    const int intSize = (int)frequencySize;

    for (size_t x = 0; x < mask->Width(); ++x) {
      int dist = intSize + 1;
      for (size_t y = 0; y < frequencySize; ++y) {
        if (mask->Value(x, y)) dist = -intSize;
        dist++;
      }
      for (size_t y = 0; y < mask->Height() - frequencySize; ++y) {
        if (mask->Value(x, y + frequencySize)) dist = -intSize;
        if (dist <= intSize) {
          destination.SetValue(x, y, true);
          dist++;
        } else {
          destination.SetValue(x, y, false);
        }
      }
      for (size_t y = mask->Height() - frequencySize; y < mask->Height(); ++y) {
        if (dist <= intSize) {
          destination.SetValue(x, y, true);
          dist++;
        } else {
          destination.SetValue(x, y, false);
        }
      }
    }
    *mask = std::move(destination);
  }
}

void MorphologicalFlagger::LineRemover(Mask2D* mask,
                                       size_t maxTimeContamination,
                                       size_t maxFreqContamination) {
  for (size_t x = 0; x < mask->Width(); ++x) {
    size_t count = 0;
    for (size_t y = 0; y < mask->Height(); ++y) {
      if (mask->Value(x, y)) ++count;
    }
    if (count > maxFreqContamination) FlagTime(mask, x);
  }

  for (size_t y = 0; y < mask->Height(); ++y) {
    size_t count = 0;
    for (size_t x = 0; x < mask->Width(); ++x) {
      if (mask->Value(x, y)) ++count;
    }
    if (count > maxTimeContamination) FlagFrequency(mask, y);
  }
}

void MorphologicalFlagger::FlagTime(Mask2D* mask, size_t x) {
  for (size_t y = 0; y < mask->Height(); ++y) {
    mask->SetValue(x, y, true);
  }
}

void MorphologicalFlagger::FlagFrequency(Mask2D* mask, size_t y) {
  for (size_t x = 0; x < mask->Width(); ++x) {
    mask->SetValue(x, y, true);
  }
}
