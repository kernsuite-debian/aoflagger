#ifndef SEGMENTEDIMAGE_H
#define SEGMENTEDIMAGE_H

#include <cstring>
#include <memory>

typedef std::shared_ptr<class SegmentedImage> SegmentedImagePtr;
typedef std::shared_ptr<const class SegmentedImage> SegmentedImageCPtr;

class SegmentedImage {
 public:
  ~SegmentedImage() {
    for (size_t y = 0; y < _height; ++y) delete[] _data[y];
    delete[] _data;
  }

  static SegmentedImage* Create(size_t width, size_t height,
                                size_t initialValue = 0) {
    SegmentedImage* image = new SegmentedImage(width, height);
    for (size_t y = 0; y < height; ++y) {
      for (size_t x = 0; x < width; ++x) image->_data[y][x] = initialValue;
    }
    return image;
  }
  static SegmentedImagePtr CreatePtr(size_t width, size_t height,
                                     size_t initialValue = 0) {
    return SegmentedImagePtr(Create(width, height, initialValue));
  }

  static SegmentedImage* CreateUnset(size_t width, size_t height) {
    return new SegmentedImage(width, height);
  }
  static SegmentedImagePtr CreateUnsetPtr(size_t width, size_t height) {
    return SegmentedImagePtr(CreateUnset(width, height));
  }
  static SegmentedImagePtr CreateCopy(SegmentedImagePtr segmentedImage) {
    SegmentedImage* copy =
        CreateUnset(segmentedImage->_width, segmentedImage->_height);
    for (size_t y = 0; y < segmentedImage->_height; ++y) {
      for (size_t x = 0; x < segmentedImage->_width; ++x) {
        copy->SetValue(x, y, segmentedImage->Value(x, y));
      }
    }
    copy->_segmentCount = segmentedImage->_segmentCount;
    return SegmentedImagePtr(copy);
  }

  size_t Value(size_t x, size_t y) const { return _data[y][x]; }
  void SetValue(size_t x, size_t y, size_t newValue) { _data[y][x] = newValue; }

  size_t Width() const { return _width; }
  size_t Height() const { return _height; }

  size_t NewSegmentValue() {
    ++_segmentCount;
    return _segmentCount;
  }
  size_t SegmentCount() const { return _segmentCount; }

  void MergeSegments(size_t destinationSegment, size_t otherSegment) {
    for (size_t y = 0; y < _height; ++y) {
      for (size_t x = 0; x < _width; ++x)
        if (_data[y][x] == otherSegment) _data[y][x] = destinationSegment;
    }
  }
  void RemoveSegment(size_t segment) {
    for (size_t y = 0; y < _height; ++y) {
      for (size_t x = 0; x < _width; ++x)
        if (_data[y][x] == segment) _data[y][x] = 0;
    }
  }
  void RemoveSegment(size_t segment, size_t xLeft, size_t xRight, size_t yTop,
                     size_t yBottom) {
    for (size_t y = yTop; y < yBottom; ++y) {
      for (size_t x = xLeft; x < xRight; ++x)
        if (_data[y][x] == segment) _data[y][x] = 0;
    }
  }

 private:
  SegmentedImage(size_t width, size_t height)
      : _width(width),
        _height(height),
        _data(new size_t*[height]),
        _segmentCount(0) {
    for (size_t y = 0; y < height; ++y) _data[y] = new size_t[width];
  }
  SegmentedImage(const SegmentedImage&) = delete;
  SegmentedImage& operator=(const SegmentedImage&) = delete;

  size_t _width, _height;
  size_t** _data;
  size_t _segmentCount;
};

#endif
