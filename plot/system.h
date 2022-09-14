#ifndef SYSTEM_H
#define SYSTEM_H

#include <map>
#include <string>

#include "dimension.h"

class XYPointSet;

class System {
 public:
  System() : _includeZeroYAxis(false) {}

  ~System() { Clear(); }

  bool Empty() const { return _dimensions.empty(); }

  void AddToSystem(XYPointSet& pointSet) {
    Dimension* dimension;
    auto iter = _dimensions.find(pointSet.YUnits());
    if (iter == _dimensions.end()) {
      dimension =
          &_dimensions.emplace(pointSet.YUnits(), Dimension()).first->second;
    } else {
      dimension = &iter->second;
    }
    dimension->AdjustRanges(pointSet);
  }

  double XRangeMin(XYPointSet& pointSet) const {
    return _dimensions.find(pointSet.YUnits())->second.XRangeMin();
  }
  double XRangePositiveMin(XYPointSet& pointSet) const {
    return _dimensions.find(pointSet.YUnits())->second.XRangePositiveMin();
  }
  double XRangeMax(XYPointSet& pointSet) const {
    return _dimensions.find(pointSet.YUnits())->second.XRangeMax();
  }
  double XRangePositiveMax(XYPointSet& pointSet) const {
    return _dimensions.find(pointSet.YUnits())->second.XRangePositiveMax();
  }
  double YRangeMin(XYPointSet& pointSet) const {
    const double yMin = _dimensions.find(pointSet.YUnits())->second.YRangeMin();
    if (yMin > 0.0 && _includeZeroYAxis)
      return 0.0;
    else
      return yMin;
  }
  double YRangePositiveMin(XYPointSet& pointSet) const {
    return _dimensions.find(pointSet.YUnits())->second.YRangePositiveMin();
  }
  double YRangeMax(XYPointSet& pointSet) const {
    const double yMax = _dimensions.find(pointSet.YUnits())->second.YRangeMax();
    if (yMax < 0.0 && _includeZeroYAxis)
      return 0.0;
    else
      return yMax;
  }
  double YRangePositiveMax(XYPointSet& pointSet) const {
    return _dimensions.find(pointSet.YUnits())->second.YRangePositiveMax();
  }
  void Clear() { _dimensions.clear(); }
  void SetIncludeZeroYAxis(bool includeZeroYAxis) {
    _includeZeroYAxis = includeZeroYAxis;
  }

 private:
  std::map<std::string, Dimension> _dimensions;
  bool _includeZeroYAxis;
};

#endif
