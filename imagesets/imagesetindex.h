#ifndef RFI_STRATEGY_IMAGESETINDEX_H
#define RFI_STRATEGY_IMAGESETINDEX_H

#include <cstring>

namespace imagesets {

class ImageSet;

class ImageSetIndex final {
 public:
  ImageSetIndex() : _n(0), _value(0), _hasWrapped(true) {}

  ImageSetIndex(size_t n, size_t value = 0)
      : _n(n), _value(value), _hasWrapped(false) {}

  void Previous() {
    if (_value == 0) {
      _value = _n - 1;
      _hasWrapped = true;
    } else {
      --_value;
    }
  }

  bool Empty() const { return _n == 0; }

  void Next() {
    ++_value;
    if (_value == _n) {
      _hasWrapped = true;
      _value = 0;
    }
  }

  size_t Value() const { return _value; }

  bool HasWrapped() const { return _hasWrapped; }

 private:
  size_t _n, _value;
  bool _hasWrapped;
};
}  // namespace imagesets

#endif
