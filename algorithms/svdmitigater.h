#ifndef SVDMITIGATER_H
#define SVDMITIGATER_H

#include <iostream>

#include "../structures/image2d.h"
#include "../structures/timefrequencydata.h"

// Needs to be included LAST
#include "../util/f2c.h"

class XYPlot;

namespace algorithms {

class SVDMitigater final {
 public:
  SVDMitigater();
  ~SVDMitigater();

  void Initialize(const TimeFrequencyData& data) {
    Clear();
    _data = data;
    _iteration = 0;
  }
  void PerformFit() {
    _iteration++;
    RemoveSingularValues(_removeCount);
  }
  void PerformFit(unsigned) { PerformFit(); }

  void RemoveSingularValues(unsigned singularValueCount) {
    if (!IsDecomposed()) Decompose();
    for (unsigned i = 0; i < singularValueCount; ++i) SetSingularValue(i, 0.0);
    Compose();
  }

  TimeFrequencyData Background() const { return *_background; }

  enum TimeFrequencyData::ComplexRepresentation ComplexRepresentation() const {
    return TimeFrequencyData::ComplexParts;
  }

  bool IsDecomposed() const throw() { return _singularValues != nullptr; }
  double SingularValue(unsigned index) const throw() {
    return _singularValues[index];
  }
  void SetRemoveCount(unsigned removeCount) throw() {
    _removeCount = removeCount;
  }
  void SetVerbose(bool verbose) throw() { _verbose = verbose; }
  static void CreateSingularValueGraph(const TimeFrequencyData& data,
                                       class XYPlot& plot);

 private:
  void Clear();
  void Decompose();
  void Compose();
  void SetSingularValue(unsigned index, double newValue) throw() {
    _singularValues[index] = newValue;
  }

  TimeFrequencyData _data;
  TimeFrequencyData* _background;
  double* _singularValues;
  doublecomplex* _leftSingularVectors;
  doublecomplex* _rightSingularVectors;
  long int _m, _n;
  unsigned _iteration;
  unsigned _removeCount;
  bool _verbose;
};

}  // namespace algorithms

#endif
