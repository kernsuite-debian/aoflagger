#ifndef PLOT_H
#define PLOT_H

#include <unistd.h>

#include <stdexcept>
#include <string>
#include <vector>

class Plot {
 public:
  //[[ deprecated("Superceded by plot2d class") ]]
  explicit Plot(const std::string& pdfFile);
  ~Plot();
  void StartLine() { StartLine(""); }
  void StartLine(const std::string& lineTitle);
  void StartScatter() { StartScatter(""); }
  void StartScatter(const std::string& lineTitle);
  void StartBoxes() { StartScatter(""); }
  void StartBoxes(const std::string& lineTitle);
  void StartGrid() { StartGrid(""); }
  void StartGrid(const std::string& gridTitle);
  void PushDataPoint(long double x, long double y);
  void PushDataPoint(long double x, long double y, long double z);
  void PushUnknownDataPoint(long double x, long double y);
  void PushDataBlockEnd();
  void Close();
  void AddRectangle(long double x1, double y1, double x2, double y2);
  void SetTitle(const std::string& title) throw() { _title = title; }
  void SetXAxisText(const std::string& text) throw() { _xAxisText = text; }
  void SetYAxisText(const std::string& text) throw() { _yAxisText = text; }
  void SetZAxisText(const std::string& text) throw() { _zAxisText = text; }
  void SetXRange(long double min, long double max) throw() {
    _xRangeHasMin = true;
    _xRangeHasMax = true;
    _xRangeMin = min;
    _xRangeMax = max;
  }
  void SetXRangeAutoMax(long double min) throw() {
    _xRangeHasMin = true;
    _xRangeHasMax = false;
    _xRangeMin = min;
  }
  void SetXRangeAutoMin(long double max) throw() {
    _xRangeHasMin = false;
    _xRangeHasMax = true;
    _xRangeMax = max;
  }
  void SetXRangeAuto() throw() {
    _xRangeHasMin = false;
    _xRangeHasMax = false;
  }
  void SetYRange(long double min, long double max) throw() {
    if (_yRangeHasMin) {
      if (min < _yRangeMin) _yRangeMin = min;
    } else {
      _yRangeHasMin = true;
      _yRangeMin = min;
    }
    if (_yRangeHasMax) {
      if (max > _yRangeMax) _yRangeMax = max;
    } else {
      _yRangeHasMax = true;
      _yRangeMax = max;
    }
  }
  void SetYRangeAutoMax(long double min) throw() {
    _yRangeHasMin = true;
    _yRangeHasMax = false;
    _yRangeMin = min;
  }
  void SetYRangeAutoMin(long double max) throw() {
    _yRangeHasMin = false;
    _yRangeHasMax = true;
    _yRangeMax = max;
  }
  void SetYRangeAuto() throw() {
    _yRangeHasMin = false;
    _yRangeHasMax = false;
  }
  void SetZRange(long double min, long double max) throw() {
    _zRangeHasMin = true;
    _zRangeHasMax = true;
    _zRangeMin = min;
    _zRangeMax = max;
  }
  void SetCBRange(long double min, long double max) throw() {
    _cbRangeHasMin = true;
    _cbRangeHasMax = true;
    _cbRangeMin = min;
    _cbRangeMax = max;
  }
  void SetLogScale(bool x, bool y, bool z = false) {
    _logX = x;
    _logY = y;
    _logZ = z;
  }
  void SetFontSize(size_t newSize) { _fontSize = newSize; }
  void Show();

 private:
  enum Type { Line, Scatter, Boxes, Grid } _curType;
  void RunGnuplot();
  char _tmpPlotFile[16];
  void Write(int fd, const std::string& str) {
    if (write(fd, str.c_str(), str.length()) != (int)str.length())
      throw std::runtime_error("write() reported an error");
  }
  void CloseCurFd();
  void ExecuteCmd(const std::string& cmd) const;
  std::vector<std::string> _lineFiles, _lineTitles;
  std::vector<Type> _lineTypes;

  int _curLineFd;
  const std::string _pdfFile;
  bool _open;
  std::string _title, _xAxisText, _yAxisText, _zAxisText;
  bool _xRangeHasMin, _xRangeHasMax;
  bool _yRangeHasMin, _yRangeHasMax;
  bool _zRangeHasMin, _zRangeHasMax;
  bool _cbRangeHasMin, _cbRangeHasMax;
  long double _xRangeMin, _xRangeMax;
  long double _yRangeMin, _yRangeMax;
  long double _zRangeMin, _zRangeMax;
  long double _cbRangeMin, _cbRangeMax;
  bool _clipZ;
  std::vector<std::string> _extraHeaders;

  bool _logX, _logY, _logZ;
  bool _hasBoxes;
  size_t _fontSize;
};

#endif
