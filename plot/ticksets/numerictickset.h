#ifndef NUMERIC_TICK_SET_H
#define NUMERIC_TICK_SET_H

#include "tickset.h"

#include <vector>
#include <cmath>
#include <sstream>

class NumericTickSet final : public TickSet {
 public:
  NumericTickSet(double min, double max, unsigned sizeRequest)
      : _min(min), _max(max), _sizeRequest(sizeRequest) {
    set(sizeRequest);
  }

  std::unique_ptr<TickSet> Clone() const override {
    return std::unique_ptr<TickSet>(new NumericTickSet(*this));
  }

  size_t Size() const override { return _ticks.size(); }

  Tick GetTick(size_t i) const override {
    std::stringstream tickStr;
    tickStr << _ticks[i];
    return Tick((_ticks[i] - _min) / (_max - _min), tickStr.str());
  }

  void Reset() override {
    _ticks.clear();
    set(_sizeRequest);
  }

  void Set(size_t maxSize) override {
    _ticks.clear();
    set(maxSize);
  }
  double UnitToAxis(double unitValue) const override {
    return (unitValue - _min) / (_max - _min);
  }
  double AxisToUnit(double axisValue) const override {
    return axisValue * (_max - _min) + _min;
  }

 private:
  void set(size_t sizeRequest) {
    if (std::isfinite(_min) && std::isfinite(_max)) {
      if (_max == _min)
        _ticks.push_back(_min);
      else {
        if (sizeRequest == 0) return;
        double tickWidth =
            roundUpToNiceNumber(fabs(_max - _min) / (double)sizeRequest);
        if (tickWidth == 0.0) tickWidth = 1.0;
        if (_min < _max) {
          double pos = roundUpToNiceNumber(_min, tickWidth);
          while (pos <= _max) {
            if (fabs(pos) < tickWidth / 100.0)
              _ticks.push_back(0.0);
            else
              _ticks.push_back(pos);
            pos += tickWidth;
          }
        } else {
          double pos = -roundUpToNiceNumber(-_min, tickWidth);
          while (pos >= _max) {
            if (fabs(pos) < tickWidth / 100.0)
              _ticks.push_back(0.0);
            else
              _ticks.push_back(pos);
            pos -= tickWidth;
          }
        }
        while (_ticks.size() > sizeRequest) _ticks.pop_back();
      }
    }
  }

  double roundUpToNiceNumber(double number) {
    if (!std::isfinite(number)) return number;
    double roundedNumber = 1.0;
    if (number <= 0.0) {
      if (number == 0.0)
        return 0.0;
      else {
        roundedNumber = -1.0;
        number *= -1.0;
      }
    }
    while (number > 10) {
      number /= 10;
      roundedNumber *= 10;
    }
    while (number <= 1) {
      number *= 10;
      roundedNumber /= 10;
    }
    if (number <= 2)
      return roundedNumber * 2;
    else if (number <= 5)
      return roundedNumber * 5;
    else
      return roundedNumber * 10;
  }
  double roundUpToNiceNumber(double number, double roundUnit) {
    return roundUnit * ceil(number / roundUnit);
  }

  double _min, _max;
  size_t _sizeRequest;
  std::vector<double> _ticks;
};

#endif
