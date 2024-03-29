#ifndef MULTIPLOT_H
#define MULTIPLOT_H

#include "../structures/types.h"

#include <vector>

#include "../plot/plotmanager.h"

class MultiPlot {
 public:
  MultiPlot(XYPlot& plot, size_t plotCount);

  void AddPoint(size_t plotIndex, num_t x, num_t y) {
    _points[plotIndex].push_back(Point(x, y));
  }
  void SetLegend(int index, const std::string& title) {
    _legends[index] = title;
  }
  void Finish();
  XYPlot& Plot() { return _plot; }
  void SetXAxisText(const std::string& text) { _xAxisText = text; }
  void SetYAxisText(const std::string& text) { _yAxisText = text; }

 private:
  MultiPlot(const MultiPlot&) = delete;
  MultiPlot& operator=(const MultiPlot&) = delete;
  struct Point {
    Point(num_t _x, num_t _y) : x(_x), y(_y) {}
    num_t x, y;
  };
  typedef std::vector<struct Point> PointList;
  std::vector<std::string> _legends;
  std::vector<PointList> _points;
  size_t _plotCount;
  XYPlot& _plot;
  std::string _xAxisText, _yAxisText;
};

#endif
