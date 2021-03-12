#include "aoflagger.h"
#include "structures.h"

namespace aoflagger {

FlagMask::FlagMask() : _data(nullptr) {}

FlagMask::FlagMask(size_t width, size_t height)
    : _data(new FlagMaskData(Mask2D::CreateUnsetMaskPtr(width, height))) {}

FlagMask::FlagMask(size_t width, size_t height, bool initialValue)
    : _data(new FlagMaskData(Mask2D::CreateUnsetMaskPtr(width, height))) {
  if (initialValue)
    _data->mask->SetAll<true>();
  else
    _data->mask->SetAll<false>();
}

FlagMask::FlagMask(const FlagMask& sourceMask)
    : _data(sourceMask._data == nullptr ? nullptr
                                        : new FlagMaskData(*sourceMask._data)) {
}

FlagMask::FlagMask(FlagMask&& sourceMask)
    : _data(std::move(sourceMask._data)) {}

FlagMask& FlagMask::operator=(const FlagMask& flagMask) {
  if (flagMask._data == nullptr) {
    _data.reset();
  } else if (_data == nullptr) {
    _data.reset(new FlagMaskData(*flagMask._data));
  } else {
    *_data = *flagMask._data;
  }
  return *this;
}

FlagMask& FlagMask::operator=(FlagMask&& flagMask) {
  std::swap(_data, flagMask._data);
  return *this;
}

FlagMask::~FlagMask() {}

size_t FlagMask::Width() const { return _data->mask->Width(); }

size_t FlagMask::Height() const { return _data->mask->Height(); }

size_t FlagMask::HorizontalStride() const { return _data->mask->Stride(); }

bool* FlagMask::Buffer() { return _data->mask->ValuePtr(0, 0); }

const bool* FlagMask::Buffer() const { return _data->mask->ValuePtr(0, 0); }

}  // namespace aoflagger
