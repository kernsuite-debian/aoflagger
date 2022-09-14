#ifndef PLOT_IMAGE_H
#define PLOT_IMAGE_H

#include "../plot/imageinterface.h"
#include "../structures/image2d.h"

class PlotImage final : public ImageInterface {
 public:
  PlotImage() : ImageInterface() {}

  PlotImage(const Image2DCPtr& image)
      : ImageInterface(image->Width(), image->Height(), image->Stride()),
        _image(image) {}

  PlotImage(Image2DCPtr&& image)
      : ImageInterface(image->Width(), image->Height(), image->Stride()),
        _image(std::move(image)) {}

  const float* Data() const override { return _image->Data(); }

  const Image2DCPtr& Get() const { return _image; }

 private:
  Image2DCPtr _image;
};

#endif
