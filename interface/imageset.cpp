#include "aoflagger.h"
#include "structures.h"

#include "../structures/image2d.h"

#include <stdexcept>

namespace aoflagger {

ImageSet::ImageSet() : _data(nullptr) {}

ImageSet::ImageSet(size_t width, size_t height, size_t count)
    : _data(new ImageSetData(count)) {
  assertValidCount(count);
  for (size_t i = 0; i != count; ++i)
    _data->images[i] = Image2D::CreateUnsetImagePtr(width, height);
}

ImageSet::ImageSet(size_t width, size_t height, size_t count,
                   float initialValue)
    : _data(new ImageSetData(count)) {
  assertValidCount(count);
  for (size_t i = 0; i != count; ++i)
    _data->images[i] = Image2D::CreateSetImagePtr(width, height, initialValue);
}

ImageSet::ImageSet(size_t width, size_t height, size_t count,
                   size_t widthCapacity)
    : _data(new ImageSetData(count)) {
  assertValidCount(count);
  for (size_t i = 0; i != count; ++i)
    _data->images[i] =
        Image2D::CreateUnsetImagePtr(width, height, widthCapacity);
}

ImageSet::ImageSet(size_t width, size_t height, size_t count,
                   float initialValue, size_t widthCapacity)
    : _data(new ImageSetData(count)) {
  assertValidCount(count);
  for (size_t i = 0; i != count; ++i)
    _data->images[i] =
        Image2D::CreateSetImagePtr(width, height, initialValue, widthCapacity);
}

ImageSet::ImageSet(const ImageSet& sourceImageSet)
    : _data(sourceImageSet._data != nullptr
                ? new ImageSetData(*sourceImageSet._data)
                : nullptr) {}

ImageSet::ImageSet::ImageSet(aoflagger::ImageSet&& sourceImageSet)
    : _data(std::move(sourceImageSet._data)) {}

ImageSet::~ImageSet() {}

ImageSet& ImageSet::operator=(const ImageSet& sourceImageSet) {
  if (sourceImageSet._data == nullptr) {
    _data.reset();
  } else if (_data == nullptr) {
    _data.reset(new ImageSetData(*sourceImageSet._data));
  } else {
    *_data = *sourceImageSet._data;
  }
  return *this;
}

ImageSet& ImageSet::operator=(ImageSet&& sourceImageSet) {
  std::swap(_data, sourceImageSet._data);
  return *this;
}

void ImageSet::assertValidCount(size_t count) {
  if (count != 1 && count != 2 && count != 4 && count != 8)
    throw std::runtime_error(
        "Invalid count specified when creating image set for aoflagger; should "
        "be 1, 2, 4 or 8.");
}

float* ImageSet::ImageBuffer(size_t imageIndex) {
  return _data->images[imageIndex]->Data();
}

const float* ImageSet::ImageBuffer(size_t imageIndex) const {
  return _data->images[imageIndex]->Data();
}

size_t ImageSet::Width() const { return _data->images[0]->Width(); }

size_t ImageSet::Height() const { return _data->images[0]->Height(); }

size_t ImageSet::ImageCount() const { return _data->images.size(); }

size_t ImageSet::HorizontalStride() const { return _data->images[0]->Stride(); }

void ImageSet::Set(float newValue) {
  for (Image2DPtr& image : _data->images) {
    image->SetAll(newValue);
  }
}

void ImageSet::ResizeWithoutReallocation(size_t newWidth) const {
  for (Image2DPtr& image : _data->images) {
    image->ResizeWithoutReallocation(newWidth);
  }
}

void ImageSet::SetAntennas(size_t antenna1, size_t antenna2) {
  _data->hasAntennas = true;
  _data->antenna1 = antenna1;
  _data->antenna2 = antenna2;
}

bool ImageSet::HasAntennas() const { return _data->hasAntennas; }

size_t ImageSet::Antenna1() const { return _data->antenna1; }

size_t ImageSet::Antenna2() const { return _data->antenna2; }

void ImageSet::SetInterval(size_t index) {
  _data->interval = index;
  _data->hasInterval = true;
}

bool ImageSet::HasInterval() const { return _data->hasInterval; }

size_t ImageSet::Interval() const { return _data->interval; }

void ImageSet::SetBand(size_t index) {
  _data->band = index;
  _data->hasBand = true;
}

bool ImageSet::HasBand() const { return _data->hasBand; }

size_t ImageSet::Band() const { return _data->band; }

}  // namespace aoflagger
