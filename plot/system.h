#ifndef SYSTEM_H
#define SYSTEM_H

#include <map>
#include <string>

#include "dimension.h"

class XYPointSet;

class System {
 public:
  System() : include_zero_y_axis_(false) {}

  ~System() { Clear(); }

  bool Empty() const { return y_dimension_.Empty() && y2_dimension_.Empty(); }

  void AddToSystem(XYPointSet& pointSet) {
    if (pointSet.UseSecondXAxis())
      x2_dimension_.AdjustRanges(pointSet, true);
    else
      x_dimension_.AdjustRanges(pointSet, true);
    if (pointSet.UseSecondYAxis())
      y2_dimension_.AdjustRanges(pointSet, false);
    else
      y_dimension_.AdjustRanges(pointSet, false);
  }

  double XRangeMin(bool second_axis) const {
    return second_axis ? x2_dimension_.Min() : x_dimension_.Min();
  }
  double XRangePositiveMin(bool second_axis) const {
    return second_axis ? x2_dimension_.PositiveMin()
                       : x_dimension_.PositiveMin();
  }
  double XRangeMax(bool second_axis) const {
    return second_axis ? x2_dimension_.Max() : x_dimension_.Max();
  }
  double XRangePositiveMax(bool second_axis) const {
    return second_axis ? x2_dimension_.PositiveMax()
                       : x_dimension_.PositiveMax();
  }
  double YRangeMin(bool second_axis) const {
    const double yMin = second_axis ? y2_dimension_.Min() : y_dimension_.Min();
    if (yMin > 0.0 && include_zero_y_axis_)
      return 0.0;
    else
      return yMin;
  }
  double YRangePositiveMin(bool second_axis) const {
    return second_axis ? y2_dimension_.PositiveMin()
                       : y_dimension_.PositiveMin();
  }
  double YRangeMax(bool second_axis) const {
    const double yMax = second_axis ? y2_dimension_.Max() : y_dimension_.Max();
    if (yMax < 0.0 && include_zero_y_axis_)
      return 0.0;
    else
      return yMax;
  }
  double YRangePositiveMax(bool second_axis) const {
    return second_axis ? y2_dimension_.PositiveMax()
                       : y_dimension_.PositiveMax();
  }
  void Clear() {
    x_dimension_.Clear();
    x2_dimension_.Clear();
    y_dimension_.Clear();
    y2_dimension_.Clear();
  }
  void SetIncludeZeroYAxis(bool includeZeroYAxis) {
    include_zero_y_axis_ = includeZeroYAxis;
  }

 private:
  Dimension x_dimension_;
  Dimension x2_dimension_;
  Dimension y_dimension_;
  Dimension y2_dimension_;
  bool include_zero_y_axis_;
};

#endif
