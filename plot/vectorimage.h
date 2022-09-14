#ifndef VECTOR_IMAGE_H
#define VECTOR_IMAGE_H

#include "imageinterface.h"

#include <memory>

class VectorImage final : public ImageInterface {
 public:
  VectorImage() : ImageInterface() {}

  VectorImage(std::vector<float>&& image, size_t width)
      : ImageInterface(width, image.size() / width, width),
        _image(std::move(image)) {}

  const float* Data() const override { return _image.data(); }

 private:
  std::vector<float> _image;
};

std::unique_ptr<VectorImage> ShrinkHorizontally(const ImageInterface& source,
                                                size_t shrinkFactor);
std::unique_ptr<VectorImage> ShrinkVertically(const ImageInterface& source,
                                              size_t shrinkFactor);

#endif
