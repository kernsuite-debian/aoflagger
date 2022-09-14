#ifndef TEXT_TICK_SET_H
#define TEXT_TICK_SET_H

#include "tickset.h"

#include <vector>

class TextTickSet : public TickSet {
 public:
  TextTickSet(const std::vector<std::string>& labels, size_t sizeRequest)
      : _sizeRequest(sizeRequest), _labels(labels) {
    set(sizeRequest);
  }

  std::unique_ptr<TickSet> Clone() const override {
    return std::unique_ptr<TickSet>(new TextTickSet(*this));
  }

  size_t Size() const final override { return _ticks.size(); }

  Tick GetTick(size_t i) const final override {
    const size_t labelIndex = _ticks[i];
    const double val = (_labels.size() == 1)
                           ? 0.5
                           : (double)labelIndex / (double)(_labels.size() - 1);
    return Tick(val, _labels[labelIndex]);
  }

  void Reset() final override {
    _ticks.clear();
    set(_sizeRequest);
  }

  void Set(size_t maxSize) final override {
    _ticks.clear();
    set(maxSize);
  }
  double UnitToAxis(double) const final override { return 0.0; }
  double AxisToUnit(double) const final override { return 0.0; }

 private:
  void set(size_t sizeRequest) {
    if (sizeRequest > _labels.size()) sizeRequest = _labels.size();
    const size_t stepSize =
        (size_t)std::ceil((double)_labels.size() / (double)sizeRequest);

    for (size_t tick = 0; tick < _labels.size(); tick += stepSize)
      _ticks.push_back(tick);
  }

  size_t _sizeRequest;
  std::vector<std::string> _labels;
  std::vector<size_t> _ticks;
};

#endif
