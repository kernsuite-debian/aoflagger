#ifndef DIMENSION_H
#define DIMENSION_H

#include <limits>

#include "xypointset.h"

class Dimension {
 public:
  Dimension() = default;

  bool Empty() const { return n_sets_ == 0; }

  void Clear() {
    n_sets_ = 0;

    min_ = std::numeric_limits<double>::quiet_NaN();
    max_ = std::numeric_limits<double>::quiet_NaN();
    positive_min_ = std::numeric_limits<double>::quiet_NaN();
    positive_max_ = std::numeric_limits<double>::quiet_NaN();
  }

  void AdjustRanges(XYPointSet& pointSet, bool use_x) {
    const double set_min = use_x ? pointSet.XRangeMin() : pointSet.YRangeMin();
    const double set_max = use_x ? pointSet.XRangeMax() : pointSet.YRangeMax();
    const double set_positive_min =
        use_x ? pointSet.XRangePositiveMin() : pointSet.YRangePositiveMin();
    const double set_positive_max =
        use_x ? pointSet.XRangePositiveMax() : pointSet.YRangePositiveMax();

    if (n_sets_ == 0) {
      min_ = set_min;
      max_ = set_max;
      positive_min_ = set_positive_min;
      positive_max_ = set_positive_max;
    } else {
      if (min_ > set_min && std::isfinite(set_min)) min_ = set_min;
      if (positive_min_ > set_positive_min && std::isfinite(set_positive_min))
        positive_min_ = set_positive_min;

      if (max_ < set_max && std::isfinite(set_max)) max_ = set_max;
      if (positive_max_ < set_positive_max && std::isfinite(set_positive_max))
        positive_min_ = set_positive_max;
    }
    ++n_sets_;
  }

  double Min() const { return min_; }
  double Max() const { return max_; }
  double PositiveMin() const { return positive_min_; }
  double PositiveMax() const { return positive_max_; }

 private:
  size_t n_sets_ = 0;

  double min_ = std::numeric_limits<double>::quiet_NaN();
  double max_ = std::numeric_limits<double>::quiet_NaN();
  double positive_min_ = std::numeric_limits<double>::quiet_NaN();
  double positive_max_ = std::numeric_limits<double>::quiet_NaN();
};

#endif
