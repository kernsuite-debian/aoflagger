#ifndef TIME_TICK_SET_H
#define TIME_TICK_SET_H

#include "tickset.h"

#include "../../structures/date.h"

class TimeTickSet final : public TickSet {
 public:
  TimeTickSet(double minTime, double maxTime, unsigned sizeRequest)
      : _min(minTime), _max(maxTime), _sizeRequest(sizeRequest) {
    if (!std::isfinite(minTime) || !std::isfinite(maxTime))
      throw std::runtime_error("Invalid (non-finite) range in TimeTickSet");
    set(sizeRequest);
  }

  std::unique_ptr<TickSet> Clone() const override {
    return std::unique_ptr<TickSet>(new TimeTickSet(*this));
  }

  size_t Size() const override { return _ticks.size(); }

  Tick GetTick(size_t i) const override {
    double val = _ticks[i];
    return Tick((val - _min) / (_max - _min), Date::AipsMJDToTimeString(val));
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
    if (_max == _min)
      _ticks.push_back(_min);
    else {
      if (sizeRequest == 0) return;
      double tickWidth =
          calculateTickWidth((_max - _min) / (double)sizeRequest);
      if (tickWidth == 0.0 || !std::isfinite(tickWidth)) tickWidth = 1.0;
      double pos = roundUpToNiceNumber(_min, tickWidth);
      while (pos < _max) {
        _ticks.push_back(pos);
        pos += tickWidth;
      }
      while (_ticks.size() > sizeRequest) _ticks.pop_back();
    }
  }

  double calculateTickWidth(double lowerLimit) const {
    if (!std::isfinite(lowerLimit)) return lowerLimit;

    // number is in units of seconds

    // In days?
    if (lowerLimit >= 60.0 * 60.0 * 24.0) {
      double width = 60.0 * 60.0 * 24.0;
      while (width < lowerLimit) width *= 2.0;
      return width;
    }
    // in hours?
    else if (lowerLimit > 60.0 * 30.0) {
      if (lowerLimit <= 60.0 * 60.0)
        return 60.0 * 60.0;  // hours
      else if (lowerLimit <= 60.0 * 60.0 * 2.0)
        return 60.0 * 60.0 * 2.0;  // two hours
      else if (lowerLimit <= 60.0 * 60.0 * 3.0)
        return 60.0 * 60.0 * 3.0;  // three hours
      else if (lowerLimit <= 60.0 * 60.0 * 4.0)
        return 60.0 * 60.0 * 4.0;  // four hours
      else if (lowerLimit <= 60.0 * 60.0 * 6.0)
        return 60.0 * 60.0 * 6.0;  // six hours
      else
        return 60.0 * 60.0 * 12.0;  // twelve hours
    }
    // in minutes?
    else if (lowerLimit > 30.0) {
      if (lowerLimit <= 60.0)
        return 60.0;  // in minutes
      else if (lowerLimit <= 60.0 * 2.0)
        return 60.0 * 2.0;  // two minutes
      else if (lowerLimit <= 60.0 * 5.0)
        return 60.0 * 5.0;  // five minutes
      else if (lowerLimit <= 60.0 * 10.0)
        return 60.0 * 10.0;  // ten minutes
      else if (lowerLimit <= 60.0 * 15.0)
        return 60.0 * 15.0;  // quarter hours
      else
        return 60.0 * 30.0;  // half hours
    }
    // in seconds?
    else if (lowerLimit > 0.5) {
      if (lowerLimit <= 1.0)
        return 1.0;  // in seconds
      else if (lowerLimit <= 2.0)
        return 2.0;  // two seconds
      else if (lowerLimit <= 5.0)
        return 5.0;  // five seconds
      else if (lowerLimit <= 10.0)
        return 10.0;  // ten seconds
      else if (lowerLimit <= 15.0)
        return 15.0;  // quarter minute
      else
        return 30.0;  // half a minute
    } else if (lowerLimit == 0.0)
      return 0.0;
    // in 10th of seconds or lower?
    else {
      double factor = 1.0;
      while (lowerLimit <= 0.1 && std::isfinite(lowerLimit)) {
        factor *= 0.1;
        lowerLimit *= 10.0;
      }
      if (lowerLimit <= 0.2)
        return 0.2 * factor;
      else if (lowerLimit <= 0.5)
        return 0.5 * factor;
      else
        return factor;
    }
  }

  double roundUpToNiceNumber(double number, double roundUnit) {
    return roundUnit * std::ceil(number / roundUnit);
  }

  double _min, _max;
  size_t _sizeRequest;
  std::vector<double> _ticks;
};

#endif
