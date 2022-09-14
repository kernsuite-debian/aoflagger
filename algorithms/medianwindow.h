#ifndef MEDIANWINDOW_H
#define MEDIANWINDOW_H

#include <algorithm>
#include <vector>

#include "../structures/samplerow.h"

namespace algorithms {

template <typename NumType>
class MedianWindow {
 public:
  static void SubtractMedian(SampleRow& sampleRow, unsigned windowSize) {
    if (windowSize > sampleRow.Size() * 2) windowSize = sampleRow.Size() * 2;
    SampleRow copy(sampleRow);
    MedianWindow<num_t> window;
    unsigned rightPtr, leftPtr = 0;
    for (rightPtr = 0; rightPtr < ((windowSize - 1) / 2); ++rightPtr) {
      if (!copy.ValueIsMissing(rightPtr)) window.Add(copy.Value(rightPtr));
    }

    for (unsigned i = 0; i < sampleRow.Size(); ++i) {
      if (rightPtr < sampleRow.Size()) {
        if (!copy.ValueIsMissing(rightPtr)) window.Add(copy.Value(rightPtr));
        ++rightPtr;
      }
      if (rightPtr > windowSize) {
        if (!copy.ValueIsMissing(leftPtr)) window.Remove(copy.Value(leftPtr));
        ++leftPtr;
      }
      sampleRow.SetValue(i, copy.Value(i) - window.Median());
    }
  }

 private:
  std::vector<NumType> data_;
  void Add(NumType sample) {
    // Add after the last value, to reduce the number of elements to shift.
    data_.insert(std::upper_bound(data_.begin(), data_.end(), sample), sample);
  }
  void Remove(NumType sample) {
    // Remove the last occurance, to reduce the number of elements to shift.
    data_.erase(std::upper_bound(data_.begin(), data_.end(), sample) - 1);
  }

  NumType Median() const {
    if (data_.size() == 0) return std::numeric_limits<NumType>::quiet_NaN();
    if (data_.size() % 2 == 0) {
      unsigned m = data_.size() / 2 - 1;
      typename std::vector<NumType>::const_iterator i = data_.begin();
      i += m;
      NumType lMid = *i;
      ++i;
      NumType rMid = *i;
      return (lMid + rMid) / 2.0;
    } else {
      unsigned m = data_.size() / 2;
      typename std::vector<NumType>::const_iterator i = data_.begin();
      i += m;
      return *i;
    }
  }
};

}  // namespace algorithms

#endif
