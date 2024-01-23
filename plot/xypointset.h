#ifndef PLOT2DPOINTSET_H
#define PLOT2DPOINTSET_H

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

class XYPointSet {
 public:
  explicit XYPointSet(size_t colorIndex) : color_index_(colorIndex) {}

  enum DrawingStyle { DrawLines, DrawPoints, DrawColumns };

  void SetLabel(const std::string& label) { label_ = label; }
  const std::string& Label() const { return label_; }

  const std::string XUnits() const { return x_desc_; }
  const std::string YUnits() const { return y_desc_; }

  bool UseSecondXAxis() const { return use_second_x_axis_; }
  void SetUseSecondXAxis(bool use_second_x_axis) {
    use_second_x_axis_ = use_second_x_axis;
  }

  bool UseSecondYAxis() const { return use_second_y_axis_; }
  void SetUseSecondYAxis(bool use_second_y_axis) {
    use_second_y_axis_ = use_second_y_axis;
  }

  const std::string& XDesc() const { return x_desc_; }
  void SetXDesc(std::string xDesc) { x_desc_ = xDesc; }

  const std::string& YDesc() const { return y_desc_; }
  void SetYDesc(std::string yDesc) { y_desc_ = yDesc; }

  DrawingStyle GetDrawingStyle() const { return drawing_style_; }
  void SetDrawingStyle(enum DrawingStyle drawingStyle) {
    drawing_style_ = drawingStyle;
  }

  void Clear() { points_.clear(); }

  void PushDataPoint(double x, double y) {
    if (std::isfinite(x)) points_.emplace_back(x, y);
  }
  double GetX(size_t index) const { return points_[index].x; }
  double GetY(size_t index) const { return points_[index].y; }
  size_t Size() const { return points_.size(); }

  double MaxX() const {
    if (points_.empty()) return std::numeric_limits<double>::quiet_NaN();
    double max = std::numeric_limits<double>::quiet_NaN();
    for (const Point2D& p : points_) {
      if (std::isfinite(p.x) && (p.x > max || !std::isfinite(max))) {
        max = p.x;
      }
    }
    return max;
  }

  double MinX() const {
    if (points_.empty()) return std::numeric_limits<double>::quiet_NaN();
    double min = std::numeric_limits<double>::quiet_NaN();
    for (const Point2D& p : points_) {
      if (std::isfinite(p.x) && (p.x < min || !std::isfinite(min))) {
        min = p.x;
      }
    }
    return min;
  }

  double MaxY() const {
    if (points_.empty()) return std::numeric_limits<double>::quiet_NaN();
    double max = std::numeric_limits<double>::quiet_NaN();
    for (const Point2D& p : points_) {
      if (std::isfinite(p.y) && (p.y > max || !std::isfinite(max))) {
        max = p.y;
      }
    }
    return max;
  }

  double MaxPositiveY() const {
    double max = 0.0;
    for (const Point2D& p : points_) {
      if (std::isfinite(p.y) && p.y > max) max = p.y;
    }
    if (max == 0.0)
      return std::numeric_limits<double>::quiet_NaN();
    else
      return max;
  }

  double MinY() const {
    if (points_.empty()) return std::numeric_limits<double>::quiet_NaN();
    double min = std::numeric_limits<double>::quiet_NaN();
    for (const Point2D& p : points_) {
      if (std::isfinite(p.y) && (p.y < min || !std::isfinite(min))) {
        min = p.y;
      }
    }
    return min;
  }

  double MinPositiveY() const {
    bool hasValue = false;
    double min = 0.0;
    // Find lowest positive element
    for (const Point2D& p : points_) {
      if (std::isfinite(p.y) && p.y > 0.0 && (p.y < min || !hasValue)) {
        min = p.y;
        hasValue = true;
      }
    }
    if (!hasValue)
      return std::numeric_limits<double>::quiet_NaN();
    else
      return min;
  }

  void Sort() { std::sort(points_.begin(), points_.end()); }

  double XRangeMin() const {
    if (points_.empty())
      return 0.0;
    else
      return points_.begin()->x;
  }

  double XRangePositiveMin() const {
    if (points_.empty()) {
      return 1.0;
    } else {
      std::vector<Point2D>::const_iterator iter =
          std::upper_bound(points_.begin(), points_.end(), Point2D(0.0, 0.0));
      if (iter == points_.end() || iter->x < 0.0 || !std::isfinite(iter->x))
        return 1.0;
      else
        return iter->x;
    }
  }
  double XRangeMax() const {
    if (points_.empty())
      return 1.0;
    else
      return points_.rbegin()->x;
  }
  double XRangePositiveMax() const {
    if (points_.empty())
      return 1.0;
    else if (points_.rbegin()->x < 0.0 || !std::isfinite(points_.rbegin()->x))
      return 1.0;
    else
      return points_.rbegin()->x;
  }
  double YRangeMin() const { return MinY(); }
  double YRangePositiveMin() const { return MinPositiveY(); }
  double YRangeMax() const { return MaxY(); }
  double YRangePositiveMax() const { return MaxPositiveY(); }

  struct Color {
    Color(double r_, double g_, double b_, double a_)
        : r(r_), g(g_), b(b_), a(a_) {}
    double r, g, b, a;
  };

  Color GetColor() const {
    switch (color_index_ % 12) {
      default:
      case 0:
        return Color(1, 0, 0, 1);
      case 1:
        return Color(0, 1, 0, 1);
      case 2:
        return Color(0, 0, 1, 1);
      case 3:
        return Color(0, 0, 0, 1);
      case 4:
        return Color(1, 1, 0, 1);
      case 5:
        return Color(1, 0, 1, 1);
      case 6:
        return Color(0, 1, 1, 1);
      case 7:
        return Color(0.5, 0.5, 0.5, 1);
      case 8:
        return Color(1.0, 0.5, 0.0, 1);
      case 9:
        return Color(0.8, 0.3, 0.8, 1);
      case 10:
        return Color(0.15, 0.5, 0.15, 1);
      case 11:
        return Color(1, 1, 0.4, 1);
    }
  }

 private:
  struct Point2D {
    Point2D(double _x, double _y) : x(_x), y(_y) {}
    double x, y;
    bool operator<(const Point2D& other) const {
      if (!std::isfinite(x))
        return false;
      else if (!std::isfinite(other.x))
        return true;
      else if (std::isnormal(x) && std::isnormal(other.x))
        return x < other.x;
      else if (!std::isnormal(x) && !std::isnormal(other.x))
        return false;
      else if (!std::isnormal(x))
        return 0 < other.x;
      else
        return other.x < 0;
    }
  };

  std::vector<Point2D> points_;
  std::string label_;
  std::string x_desc_;
  std::string y_desc_;
  DrawingStyle drawing_style_ = DrawLines;
  size_t color_index_ = 0;
  bool use_second_x_axis_ = false;
  bool use_second_y_axis_ = false;
};

#endif
