#include "vectorimage.h"

std::unique_ptr<VectorImage> ShrinkHorizontally(const ImageInterface& source,
                                                size_t shrinkFactor) {
  if (source.Width() == 0)
    return std::unique_ptr<VectorImage>(new VectorImage());
  else {
    const size_t newWidth = (source.Width() + shrinkFactor - 1) / shrinkFactor;
    const size_t height = source.Height();

    std::vector<float> newImage;
    newImage.reserve(newWidth * height);

    for (size_t x = 0; x != newWidth; ++x) {
      size_t binSize = shrinkFactor;
      if (binSize + x * shrinkFactor > source.Width())
        binSize = source.Width() - x * shrinkFactor;

      const float* sourceData = source.Data();
      for (size_t y = 0; y != height; ++y) {
        float sum = 0.0;
        const float* sourceRow = &sourceData[y * source.Stride()];
        for (size_t binX = 0; binX < binSize; ++binX) {
          size_t curX = x * shrinkFactor + binX;
          sum += sourceRow[curX];
        }
        newImage.emplace_back(sum / float(binSize));
      }
    }
    return std::unique_ptr<VectorImage>(
        new VectorImage(std::move(newImage), newWidth));
  }
}

std::unique_ptr<VectorImage> ShrinkVertically(const ImageInterface& source,
                                              size_t shrinkFactor) {
  const size_t sourceHeight = source.Height();
  const size_t newHeight = (sourceHeight + shrinkFactor - 1) / shrinkFactor;

  std::vector<float> newImage;
  newImage.reserve(source.Width() * newHeight);

  const float* sourceData = source.Data();
  for (size_t y = 0; y != newHeight; ++y) {
    size_t binSize = shrinkFactor;
    if (binSize + y * shrinkFactor > sourceHeight)
      binSize = sourceHeight - y * shrinkFactor;

    for (size_t x = 0; x != source.Width(); ++x) {
      float sum = 0.0;
      for (size_t binY = 0; binY < binSize; ++binY) {
        size_t curY = y * shrinkFactor + binY;
        const float* sourceRow = &sourceData[curY * source.Stride()];
        sum += sourceRow[x];
      }
      newImage.emplace_back(sum / float(binSize));
    }
  }
  return std::unique_ptr<VectorImage>(
      new VectorImage(std::move(newImage), source.Width()));
}
