#ifndef PLOT_AXIS_H_
#define PLOT_AXIS_H_

#include <string>
#include <vector>

enum class AxisType { kNumeric, kText, kTime };

class Axis {
 public:
  const std::vector<std::pair<double, std::string>>& TickLabels() const {
    return tick_labels_;
  }
  void SetTickLabels(
      const std::vector<std::pair<double, std::string>>& tick_labels) {
    tick_labels_ = tick_labels;
    type_ = AxisType::kText;
  }
  void SetTickLabels(
      std::vector<std::pair<double, std::string>>&& tick_labels) {
    tick_labels_ = std::move(tick_labels);
    type_ = AxisType::kText;
  }

  bool RotateUnits() const { return rotate_units_; }
  void SetRotateUnits(bool rotate_units) { rotate_units_ = rotate_units; }

  AxisType Type() const { return type_; }
  void SetType(AxisType type) { type_ = type; }

 private:
  AxisType type_ = AxisType::kNumeric;
  std::vector<std::pair<double, std::string>> tick_labels_;
  bool rotate_units_ = false;
};

#endif
