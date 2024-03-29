#ifndef RAYLEIGHFITTER_H
#define RAYLEIGHFITTER_H

#include "loghistogram.h"

class RayleighFitter {
 public:
  RayleighFitter()
      : _fitLogarithmic(true), _hist(nullptr), _minVal(0.0), _maxVal(0.0) {}

  void Fit(double minVal, double maxVal, const LogHistogram& hist,
           double& sigma, double& n);
  static double SigmaEstimate(const LogHistogram& hist);
  static double SigmaEstimate(const LogHistogram& hist, double rangeStart,
                              double rangeEnd);
  static double NEstimate(const LogHistogram& hist, double rangeStart,
                          double rangeEnd);
  static void FindFitRangeUnderRFIContamination(double minPositiveAmplitude,
                                                double sigmaEstimate,
                                                double& minValue,
                                                double& maxValue);
  static double RayleighValue(double sigma, double n, double x) {
    double sigmaP2 = sigma * sigma;
    return n * x / (sigmaP2)*exp(-x * x / (2 * sigmaP2));
  }
  static double ErrorOfFit(const LogHistogram& histogram, double rangeStart,
                           double rangeEnd, double sigma, double n);

  bool FitLogarithmic() const { return _fitLogarithmic; }
  void SetFitLogarithmic(bool fitLogarithmic) {
    _fitLogarithmic = fitLogarithmic;
  }

 private:
  bool _fitLogarithmic;

 public:
  const LogHistogram* _hist;
  double _minVal, _maxVal;
};

#endif
