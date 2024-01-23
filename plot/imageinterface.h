#ifndef IMAGE_INTERFACE_H
#define IMAGE_INTERFACE_H

#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

class ImageInterface {
 public:
  ImageInterface() : _width(0), _height(0), _stride(0) {}
  ImageInterface(size_t width, size_t height, size_t stride)
      : _width(width), _height(height), _stride(stride) {}
  virtual ~ImageInterface() {}
  virtual const float* Data() const = 0;
  bool Empty() const { return _width == 0 || _height == 0; }
  size_t Width() const { return _width; }
  size_t Height() const { return _height; }
  size_t Stride() const { return _stride; }

  float Maximum() const {
    if (Empty()) return 0.0;
    const float* data = Data();
    float value = std::numeric_limits<float>::min();
    for (size_t y = 0; y != _height; ++y) {
      const float* row = &data[y * _stride];
      for (size_t x = 0; x != _width; ++x) {
        if (std::isfinite(row[x]) && value < row[x]) value = row[x];
      }
    }
    return value;
  }

  float Minimum() const {
    if (Empty()) return 0.0;
    const float* data = Data();
    float value = std::numeric_limits<float>::max();
    for (size_t y = 0; y != _height; ++y) {
      const float* row = &data[y * _stride];
      for (size_t x = 0; x != _width; ++x) {
        if (std::isfinite(row[x]) && value > row[x]) value = row[x];
      }
    }
    return value;
  }

 private:
  size_t _width;
  size_t _height;
  size_t _stride;
};

#endif
