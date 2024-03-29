#ifndef MORPHOLOGY_H
#define MORPHOLOGY_H

#include <vector>
#include <map>

#include "../structures/mask2d.h"
#include "../structures/segmentedimage.h"

namespace algorithms {

class Morphology {
 public:
  Morphology()
      : _hLineEnlarging(1),
        _vLineEnlarging(1),
        _hDensityEnlargeRatio(0.5),
        _vDensityEnlargeRatio(0.5) {}
  ~Morphology() {}

  void SegmentByMaxLength(const Mask2D* mask, SegmentedImagePtr output);
  void SegmentByLengthRatio(const Mask2D* mask, SegmentedImagePtr output);
  void Cluster(SegmentedImagePtr segmentedImage);
  void RemoveSmallSegments(SegmentedImagePtr segmentedImage,
                           size_t thresholdLevel);
  void Classify(SegmentedImagePtr segmentedImage);

  static size_t BROADBAND_SEGMENT, LINE_SEGMENT, BLOB_SEGMENT;

 private:
  struct SegmentInfo {
    SegmentInfo()
        : segment(0),
          top(0),
          left(0),
          bottom(0),
          right(0),
          count(0),
          width(0),
          height(0),
          xTotal(0),
          yTotal(0),
          mark(false) {}

    size_t segment;
    size_t top, left, bottom, right;
    size_t count;
    size_t width, height;
    size_t xTotal, yTotal;
    bool mark;

    void AddPoint(size_t x, size_t y) {
      if (x < left) left = x;
      if (x >= right) right = x + 1;
      if (y < top) top = y;
      if (y >= bottom) bottom = y + 1;
      xTotal += x;
      yTotal += y;
      ++count;
    }
    int HorizontalDistance(const SegmentInfo& other) const {
      if (other.left > right)
        return (int)other.left - (int)right;
      else if (left > other.right)
        return (int)left - (int)other.right;
      else
        return 0;
    }
    int VerticalDistance(const SegmentInfo& other) const {
      if (other.top > bottom)
        return (int)other.top - (int)bottom;
      else if (top > other.bottom)
        return (int)top - (int)other.bottom;
      else
        return 0;
    }
    bool Contains(size_t x, size_t y) const {
      return (x >= left && x < right && y >= top && y < bottom);
    }
  };

  void calculateOpenings(const Mask2D* mask, int** values);
  void calculateOpenings(const Mask2D* mask, Mask2DPtr* values, int** hCounts,
                         int** vCounts);
  void calculateVerticalCounts(const Mask2D* mask, int** values);
  void calculateHorizontalCounts(const Mask2D* mask, int** values);
  void floodFill(const Mask2D* mask, SegmentedImagePtr output,
                 const int* const* lengthWidthValues, size_t x, size_t y,
                 size_t value);
  void floodFill(const Mask2D* mask, SegmentedImagePtr output,
                 Mask2DPtr* matrices, size_t x, size_t y, size_t z,
                 size_t value, int** hCounts, int** vCounts);
  std::map<size_t, SegmentInfo> createSegmentMap(
      SegmentedImageCPtr segmentedImage) const;

  size_t _hLineEnlarging;
  size_t _vLineEnlarging;
  double _hDensityEnlargeRatio, _vDensityEnlargeRatio;
};

}  // namespace algorithms

#endif
