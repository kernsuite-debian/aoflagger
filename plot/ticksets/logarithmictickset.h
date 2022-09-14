#ifndef LOGARITHMIC_TICK_SET_H
#define LOGARITHMIC_TICK_SET_H

#include "tickset.h"

class LogarithmicTickSet : public TickSet {
 public:
  LogarithmicTickSet(double min, double max, unsigned sizeRequest)
      : _min(min),
        _minLog10(log10(min)),
        _max(max),
        _maxLog10(log10(max)),
        _sizeRequest(sizeRequest) {
    if (std::isfinite(min) && std::isfinite(max)) {
      set(sizeRequest);
    }
  }

  std::unique_ptr<TickSet> Clone() const override {
    return std::unique_ptr<LogarithmicTickSet>(new LogarithmicTickSet(*this));
  }

  size_t Size() const final override { return _ticks.size(); }

  Tick GetTick(size_t i) const final override { return _ticks[i]; }

  void Reset() final override {
    _ticks.clear();
    set(_sizeRequest);
  }

  void Set(size_t maxSize) final override {
    _ticks.clear();
    set(maxSize);
  }

  /**
   * Returns a value scaled according to a given axis.
   * Values that are within the min and max will have an axis value of
   * 0 to 1.
   */
  static double UnitToAxis(double unitValue, double unitMin, double unitMax) {
    return log(unitValue / unitMin) / log(unitMax / unitMin);
  }

  static double AxisToUnit(double axisValue, double unitMin, double unitMax) {
    return exp(axisValue * log(unitMax / unitMin)) * unitMin;
  }
  double UnitToAxis(double unitValue) const final override {
    return UnitToAxis(unitValue, _min, _max);
  }
  double AxisToUnit(double axisValue) const final override {
    return AxisToUnit(axisValue, _min, _max);
  }

 private:
  static std::string toSuperScript(const std::string& val) {
    const std::string kToSuperTable[10] = {"⁰", "¹", "²", "³", "⁴",
                                           "⁵", "⁶", "⁷", "⁸", "⁹"};
    std::stringstream out;
    for (const char c : val) {
      if (c >= '0' && c <= '9') {
        out << kToSuperTable[c - '0'];
      } else if (c == '-') {
        out << "⁻";
      } else
        out << c;
    }
    return out.str();
  }

  void set(size_t sizeRequest) {
    std::vector<double> vals;
    bool hasTensOnly = true;
    if (_max == _min) {
      vals.emplace_back(_min);
      hasTensOnly = false;
    } else {
      if (sizeRequest == 0) sizeRequest = 1;
      const double tickStart = roundUpToBase10Number(_min * 0.999),
                   tickEnd = roundDownToBase10Number(_max * 1.001);
      vals.emplace_back(tickStart);
      if (sizeRequest == 1) return;
      // Add all steps with factor of ten
      if (tickEnd > tickStart) {
        const unsigned distance =
            (unsigned)std::round(std::log10(tickEnd / tickStart));
        const unsigned step = (distance + sizeRequest - 1) / sizeRequest;
        const double factor = exp10((double)step);
        double pos = tickStart * factor;
        while (pos <= _max && vals.size() < sizeRequest) {
          vals.emplace_back(pos);
          pos *= factor;
        }
      }
      // can we add two to nine?
      // If there are already 3 steps of 10, skip this so the format can be like
      // "10^3" etc
      bool isFull = vals.size() >= sizeRequest;  // || vals.size() >= 3;
      if (!isFull) {
        std::vector<double> tryTickset(vals);
        double base = tickStart / 10.0;
        do {
          for (double i = 2.0; i < 9.5; ++i) {
            double val = base * i;
            if (val >= _min && val <= _max) tryTickset.push_back(val);
          }
          base *= 10.0;
        } while (base < _max);
        if (tryTickset.size() <= sizeRequest) {
          vals = tryTickset;
          hasTensOnly = false;
          isFull = true;

          // can we add 1.5 ?
          base = tickStart / 10.0;
          do {
            double val = base * 1.5;
            if (val >= _min && val <= _max) tryTickset.push_back(val);
            base *= 10.0;
          } while (base < _max);
          if (tryTickset.size() <= sizeRequest) vals = std::move(tryTickset);
        }
      }
      // can we add two, four, ... eight?
      if (!isFull) {
        std::vector<double> tryTickset(vals);
        double base = tickStart / 10.0;
        do {
          for (double i = 2.0; i < 9.0; i += 2.0) {
            double val = base * i;
            if (val >= _min && val <= _max) tryTickset.push_back(val);
          }
          base *= 10.0;
        } while (base < _max);
        if (tryTickset.size() <= sizeRequest) {
          vals = tryTickset;
          hasTensOnly = false;
          isFull = true;

          // can we add 1.5 ?
          base = tickStart / 10.0;
          do {
            double val = base * 1.5;
            if (val >= _min && val <= _max) tryTickset.push_back(val);
            base *= 10.0;
          } while (base < _max);
          if (tryTickset.size() <= sizeRequest) vals = std::move(tryTickset);
        }
      }
      // can we add two and five?
      if (!isFull) {
        std::vector<double> tryTickset(vals);
        double base = tickStart / 10.0;
        do {
          for (double i = 2.0; i < 6.0; i += 3.0) {
            double val = base * i;
            if (val >= _min && val <= _max) tryTickset.push_back(val);
          }
          base *= 10.0;
        } while (base < _max);
        if (tryTickset.size() <= sizeRequest) {
          vals = std::move(tryTickset);
          hasTensOnly = false;
          isFull = true;
        }
      }
      // can we add fives?
      if (!isFull) {
        std::vector<double> tryTickset(vals);
        double base = tickStart / 10.0;
        do {
          double val = base * 5.0;
          if (val >= _min && val <= _max) tryTickset.push_back(val);
          base *= 10.0;
        } while (base < _max);
        if (tryTickset.size() <= sizeRequest) {
          vals = std::move(tryTickset);
          hasTensOnly = false;
          isFull = true;
        }
      }
      std::sort(vals.begin(), vals.end());
    }

    // Should we use "3e-3" syntax?
    bool preferExp = (_max / _min > 100 || _max >= 1000.0 || _min < 0.01);
    // Can we instead use "10^-5" syntax?
    bool preferTens = preferExp && hasTensOnly;

    for (double v : vals) {
      if (preferTens) {
        int eVal = int(std::round(std::log10(v)));
        _ticks.emplace_back(
            (std::log10(v) - _minLog10) / (_maxLog10 - _minLog10),
            "10" + toSuperScript(std::to_string(eVal)));
      } else if (preferExp) {
        int eVal = int(std::floor(std::log10(v)));
        double mantissa = v / exp10(eVal);
        if (std::fabs(mantissa - 10.0) < 1e-6) {
          eVal++;
          mantissa /= 10.0;
        }
        std::ostringstream str;
        str << (v / exp10(eVal)) << "⋅10"
            << toSuperScript(std::to_string(eVal));
        _ticks.emplace_back(
            (std::log10(v) - _minLog10) / (_maxLog10 - _minLog10), str.str());
      } else {
        std::ostringstream str;
        str << v;
        _ticks.emplace_back(
            (std::log10(v) - _minLog10) / (_maxLog10 - _minLog10), str.str());
      }
    }
  }

  double roundUpToBase10Number(double number) const {
    if (!std::isfinite(number)) return number;
    const double l = log10(number);
    return exp10(ceil(l));
  }

  double roundDownToBase10Number(double number) const {
    if (!std::isfinite(number)) return number;
    const double l = log10(number);
    return exp10(floor(l));
  }

  double _min, _minLog10, _max, _maxLog10;
  size_t _sizeRequest;
  std::vector<Tick> _ticks;
};

#endif
